using Azure.Storage.Blobs;
using AzureBackupTool.Models;
using AzureBackupTool.Options;
using Dapper;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Options;
using System.Collections.Immutable;
using System.Formats.Tar;
using System.IO.Compression;

namespace AzureBackupTool;

public class Worker : BackgroundService
{
    private readonly ILogger<Worker> _logger;
    private readonly ProgramOptions _options;
    private readonly string _connectionString;
    private readonly BlobServiceClient _blobServiceClient;

    public Worker(
        ILogger<Worker> logger,
        IOptions<ProgramOptions> options,
        BlobServiceClient blobServiceClient)
    {
        _logger = logger;
        _options = options.Value;
        SqliteConnectionStringBuilder connectionStringBuilder = new()
        {
            DataSource = options.Value.DatabasePath
        };
        _connectionString = connectionStringBuilder.ToString();
        _blobServiceClient = blobServiceClient;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        PeriodicTimer timer = new(TimeSpan.FromSeconds(10));
        while (await timer.WaitForNextTickAsync(stoppingToken))
        {
            _logger.LogInformation("Backing up files in {Path}", _options.TargetDirectoryPath);
            RegisterSnapshots(_options.TargetDirectoryPath, _connectionString);
            await BuildArchives(_connectionString, stoppingToken);
            await UploadArchives(_connectionString, stoppingToken);
        }
    }

    // Stage One //

    private void RegisterSnapshots(string targetDirectoryPath, string connectionString)
    {
        var snapshots = GetNamesOfExistingSnapshots(connectionString);
        var newSnapshotsToRegister = Directory
            .EnumerateDirectories(targetDirectoryPath)
            .Where(d => !snapshots.Contains(d))
            .Select(d => new Snapshot(d))
            .ToImmutableArray();

        if (newSnapshotsToRegister.Length == 0)
        {
            _logger.LogDebug("No new snapshots to register.");
            return;
        }

        InsertNewSnapshots(connectionString, newSnapshotsToRegister);
    }

    private ImmutableHashSet<string> GetNamesOfExistingSnapshots(string connectionString)
    {
        using SqliteConnection connection = new(connectionString);
        connection.Open();
        _logger.LogDebug("Getting existing snapshots in database {Database}.", connection.Database);

        ImmutableHashSet<string> snapshotNames = [.. connection.Query<string>("SELECT name FROM snapshots;")];
        _logger.LogDebug("Retrieved {SnapshotCount} existing snapshots.", snapshotNames.Count);

        return snapshotNames;
    }

    private void InsertNewSnapshots(string connectionString, ImmutableArray<Snapshot> snapshots)
    {
        using SqliteConnection connection = new(connectionString);
        connection.Open();
        _logger.LogDebug("Attempting to write {SnapshotCount} new snapshots into database {Database}.",
            snapshots.Length,
            connection.Database);

        DynamicParameters parameters = new();
        List<string> values = [];
        for (var i = 0; i < snapshots.Length; i++)
        {
            values.Add($"(@name{i}, @status{i})");
            parameters.Add($"name{i}", snapshots[i].Name);
            parameters.Add($"status{i}", snapshots[i].Status);
        }
        var query = $@"
            INSERT INTO snapshots (name, status) 
            VALUES {string.Join(", ", values)};";

        var count = connection.Execute(query, parameters);
        _logger.LogInformation("Registered {Count} new snapshots.", count);
    }

    // Stage Two // 

    private async ValueTask BuildArchives(string connectionString, CancellationToken cancellationToken)
    {
        var snapshots = GetRegisteredSnapshots(connectionString);
        using SqliteConnection connection = new(connectionString);
        connection.Open();
        foreach (var snapshot in snapshots)
        {
            _logger.LogDebug(
                "Updating snapshot {SnapshotName} to be in the {Status} state.",
                snapshot, Status.BuildingArchive);
            connection.Execute(
                "UPDATE snapshots SET status = @status WHERE name = @name;",
                new
                {
                    status = Status.BuildingArchive,
                    name = snapshot
                });
            var archive = await BuildArchive(snapshot, cancellationToken);
            _logger.LogDebug(
                "Updating snapshot {SnapshotName} to be in the {Status} state.",
                snapshot, Status.ArchiveBuilt);
            connection.Execute(
                "UPDATE snapshots SET status = @status, archive_name = @archive WHERE name = @name;",
                new
                {
                    status = Status.ArchiveBuilt,
                    archive,
                    name = snapshot
                });
        }
    }

    private ImmutableArray<string> GetRegisteredSnapshots(string connectionString)
    {
        using SqliteConnection connection = new(connectionString);
        connection.Open();
        _logger.LogDebug("Getting snapshots in the {Status} state in database {Database}.",
            Status.Registered,
            connection.Database);

        var query = "SELECT name FROM snapshots WHERE status = @status;";
        ImmutableArray<string> snapshotNames = [.. connection.Query<string>(query, new { status = Status.Registered })];
        _logger.LogDebug("Retrieved {SnapshotCount} snapshots in the {Status} state.",
            snapshotNames.Length,
            Status.Registered);

        return snapshotNames;
    }

    private async ValueTask<string> BuildArchive(string snapshot, CancellationToken cancellationToken)
    {
        _logger.LogDebug("Building archive for snapshot {Snapshot}.", snapshot);
        var archiveName = $"{snapshot}.tar.gz";
        using FileStream stream = File.Create(archiveName);
        using GZipStream gz = new(stream, CompressionMode.Compress);
        await TarFile.CreateFromDirectoryAsync(
            snapshot,
            gz,
            false,
            cancellationToken);
        _logger.LogInformation("Successfully created archive for snapshot {Snapshot}.", snapshot);
        return archiveName;
    }

    // Stage Three //

    private async ValueTask UploadArchives(string connectionString, CancellationToken cancellationToken)
    {
        var snapshots = GetArchivedSnapshots(connectionString);
        using SqliteConnection connection = new(connectionString);
        connection.Open();
        foreach (var snapshot in snapshots)
        {
            _logger.LogDebug("{Name}, {Archive}, {Status}", snapshot.Name, snapshot.ArchiveName, snapshot.Status);
            _logger.LogDebug(
                "Updating snapshot {SnapshotName} to be in the {Status} state.",
                snapshot.Name,
                Status.UploadingArchive);
            connection.Execute(
                "UPDATE snapshots SET status = @status WHERE name = @name;",
                new
                {
                    status = Status.UploadingArchive,
                    name = snapshot.Name
                });
            await UploadArchive(snapshot, cancellationToken);
            _logger.LogDebug(
                "Updating snapshot {SnapshotName} to be in the {Status} state.",
                snapshot.Name,
                Status.ArchiveUploaded);
            connection.Execute(
                "UPDATE snapshots SET status = @status WHERE name = @name;",
                new
                {
                    status = Status.ArchiveUploaded,
                    name = snapshot.Name
                });
        }
    }

    private ImmutableArray<Snapshot> GetArchivedSnapshots(string connectionString)
    {
        using SqliteConnection connection = new(connectionString);
        connection.Open();
        _logger.LogDebug("Getting snapshots in the {Status} state in database {Database}.",
            Status.ArchiveBuilt,
            connection.Database);

        var query = "SELECT name, status, archive_name AS ArchiveName FROM snapshots WHERE status = @status;";
        ImmutableArray<Snapshot> snapshotNames = [.. connection.Query<Snapshot>(query, new { status = Status.ArchiveBuilt })];
        _logger.LogDebug("Retrieved {SnapshotCount} snapshots in the {Status} state.",
            snapshotNames.Length,
            Status.ArchiveBuilt);

        return snapshotNames;
    }

    private async ValueTask UploadArchive(Snapshot snapshot, CancellationToken cancellationToken)
    {
        _logger.LogDebug("Uploading archive for snapshot {Snapshot}.", snapshot.Name);
        using FileStream stream = File.OpenRead(snapshot.ArchiveName);
        var blobClient = _blobServiceClient
            .GetBlobContainerClient(_options.Output)
            .GetBlobClient(Path.GetFileName(snapshot.ArchiveName));
        await blobClient.UploadAsync(stream, cancellationToken);
        _logger.LogInformation("Successfully uploaded archive for snapshot {Snapshot}.", snapshot.Name);
    }
}
