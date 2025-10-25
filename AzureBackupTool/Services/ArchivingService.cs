using System.Collections.Immutable;
using System.Formats.Tar;
using System.IO.Compression;
using AzureBackupTool.Models;
using Dapper;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Options;

namespace AzureBackupTool.Services;

public class ArchivingService
{
    private readonly ILogger<ArchivingService> _logger;
    private readonly string _connectionString;

    public ArchivingService(
        ILogger<ArchivingService> logger,
        IOptions<ArchivingOptions> options)
    {
        _logger = logger;
        SqliteConnectionStringBuilder connectionStringBuilder = new()
        {
            DataSource = options.Value.DatabasePath
        };
        _connectionString = connectionStringBuilder.ToString();
    }

    public async ValueTask BuildArchives(CancellationToken cancellationToken)
    {
        var snapshots = GetRegisteredSnapshots(_connectionString);
        using SqliteConnection connection = new(_connectionString);
        connection.Open();
        foreach (var snapshot in snapshots)
        {
            var archiveName = $"{snapshot.Name}.tar.gz";
            // If the status is BuildingArchive then a previous attempt to build an archive was
            // interrupted either by cancellation or by program termination. We are able to recover
            // from this state by cleaning up any existing archive and proceeding as normal.
            if (snapshot.Status == Status.BuildingArchive && File.Exists(archiveName))
            {
                File.Delete(archiveName);
            }

            _logger.LogDebug(
                "Updating snapshot {SnapshotName} to be in the {Status} state.",
                snapshot.Name, Status.BuildingArchive);
            connection.Execute(
                "UPDATE snapshots SET status = @status WHERE name = @name;",
                new
                {
                    status = Status.BuildingArchive,
                    name = snapshot.Name
                });

            archiveName = await BuildArchive(snapshot.Name, archiveName, cancellationToken);

            _logger.LogDebug(
                "Updating snapshot {SnapshotName} to be in the {Status} state.",
                snapshot.Name, Status.ArchiveBuilt);
            connection.Execute(
                "UPDATE snapshots SET status = @status, archive_name = @archive WHERE name = @name;",
                new
                {
                    status = Status.ArchiveBuilt,
                    archive = archiveName,
                    name = snapshot.Name
                });
        }
    }

    private ImmutableArray<Snapshot> GetRegisteredSnapshots(string connectionString)
    {
        using SqliteConnection connection = new(connectionString);
        connection.Open();
        _logger.LogDebug("Getting snapshots in the {Registered} or {BuildingArchive} state in database {Database}.",
            Status.Registered,
            Status.BuildingArchive,
            connection.Database);

        var query = @"
            SELECT name, status, archive_name as ArchiveName 
            FROM snapshots 
            WHERE status = @registered OR status = @building;";
        ImmutableArray<Snapshot> snapshots = [..
            connection.Query<Snapshot>(query, new { registered = Status.Registered, building = Status.BuildingArchive })];
        _logger.LogDebug("Retrieved {SnapshotCount} snapshots.",
            snapshots.Length);

        return snapshots;
    }

    private async ValueTask<string> BuildArchive(string snapshot, string archiveName, CancellationToken cancellationToken)
    {
        _logger.LogDebug("Building archive for snapshot {Snapshot}.", snapshot);
        if (File.Exists(archiveName))
        {
            throw new InvalidOperationException($"Unable to process snapshot {snapshot}: archive {archiveName} already exists");
        }

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
}
