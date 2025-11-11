using System.Collections.Immutable;
using Azure.Storage.Blobs;
using AzureBackupTool.Models;
using Dapper;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Options;

namespace AzureBackupTool.Services;

public class UploadService
{
    private readonly ILogger<UploadService> _logger;
    private readonly string _connectionString;
    private readonly BlobServiceClient _blobServiceClient;
    private readonly string _output;

    public UploadService(
        ILogger<UploadService> logger,
        IOptions<UploadOptions> options,
        BlobServiceClient blobServiceClient)
    {
        _logger = logger;
        SqliteConnectionStringBuilder connectionStringBuilder = new()
        {
            DataSource = options.Value.DatabasePath
        };
        _connectionString = connectionStringBuilder.ToString();
        _blobServiceClient = blobServiceClient;
        _output = options.Value.Output;
    }

    public async ValueTask UploadArchives(CancellationToken cancellationToken)
    {
        var snapshots = GetArchivedSnapshots(_connectionString);
        using SqliteConnection connection = new(_connectionString);
        connection.Open();
        foreach (var snapshot in snapshots)
        {
            await UploadArchive(snapshot, cancellationToken);

            _logger.LogDebug(
                "Updating snapshot {SnapshotName} to be in the {Status} state.",
                snapshot.Name, Status.ArchiveUploaded);
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
        
        // We can use the null-forgiving operator here as only valid snapshots should exist, but if they don't
        // then we want to throw as that signals corrupted state.
        using FileStream stream = File.OpenRead(snapshot.ArchiveName!);
        var blobClient = _blobServiceClient
            .GetBlobContainerClient(_output)
            .GetBlobClient(Path.GetFileName(snapshot.ArchiveName));
        await blobClient.UploadAsync(stream, cancellationToken);
        _logger.LogInformation("Successfully uploaded archive for snapshot {Snapshot}.", snapshot.Name);
    }
}