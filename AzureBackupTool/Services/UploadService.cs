using System.Collections.Immutable;
using Azure.Storage;
using Azure.Storage.Blobs;
using Azure.Storage.Blobs.Models;
using AzureBackupTool.Models;
using Dapper;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Options;

namespace AzureBackupTool.Services;

public class UploadService
{

    private readonly string _connectionString;
    private readonly string _output;
    private readonly ILogger<UploadService> _logger;
    private readonly BlobServiceClient _blobServiceClient;
    private readonly BlobUploadOptions _blobUploadOptions = new()
    {
        TransferValidation = new()
        {
            ChecksumAlgorithm = StorageChecksumAlgorithm.MD5
        },
        Conditions = new()
        {
            IfNoneMatch = new("*")
        }
    };

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
            var blobClient = _blobServiceClient
                .GetBlobContainerClient(_output)
                .GetBlobClient(Path.GetFileName(snapshot.ArchiveName));
            // If the status is UploadingArchive then a previous attempt to upload an archive was
            // interrupted either by cancellation or by program termination. We are able to recover
            // from this state by cleaning up any existing blob and proceeding as normal.
            if (snapshot.Status == Status.ArchiveUploading)
            {
                // We only need to delete the blob if the checksum does not match the local copy.
                // TODO: implement checksum validation
                await blobClient.DeleteIfExistsAsync(cancellationToken: cancellationToken);
            }

            _logger.LogDebug(
                "Updating snapshot {SnapshotName} to be in the {Status} state.",
                snapshot.Name, Status.ArchiveUploading);
            connection.Execute(
                "UPDATE snapshots SET status = @status WHERE name = @name;",
                new
                {
                    status = Status.ArchiveUploading,
                    name = snapshot.Name
                });

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
        _logger.LogDebug("Getting snapshots in the {Status} or {Status} state in database {Database}.",
            Status.ArchiveBuilt,
            Status.ArchiveUploading,
            connection.Database);

        var query = @"
            SELECT name, status, archive_name AS ArchiveName
            FROM snapshots
            WHERE status = @built OR status = @uploading;";
        ImmutableArray<Snapshot> snapshotNames = [..
            connection.Query<Snapshot>(query, new { built = Status.ArchiveBuilt, uploading = Status.ArchiveUploading })];
        _logger.LogDebug("Retrieved {Count} snapshots.",
            snapshotNames.Length);

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
        await blobClient.UploadAsync(stream, _blobUploadOptions, cancellationToken);
        _logger.LogInformation("Successfully uploaded archive for snapshot {Snapshot}.", snapshot.Name);
    }
}