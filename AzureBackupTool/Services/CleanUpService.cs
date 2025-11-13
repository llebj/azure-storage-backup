using System.Collections.Immutable;
using AzureBackupTool.Models;
using Dapper;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Options;

namespace AzureBackupTool.Services;

public class CleanUpService
{
    private readonly ILogger<CleanUpService> _logger;
    private readonly string _connectionString;

    public CleanUpService(
        ILogger<CleanUpService> logger,
        IOptions<CleanupOptions> options)
    {
        _logger = logger;
        SqliteConnectionStringBuilder connectionStringBuilder = new()
        {
            DataSource = options.Value.DatabasePath
        };
        _connectionString = connectionStringBuilder.ToString();
    }

    public void RemoveArchives()
    {
        var snapshots = GetUploadedSnapshots(_connectionString);
        using SqliteConnection connection = new(_connectionString);
        connection.Open();
        foreach (var snapshot in snapshots)
        {
            _logger.LogDebug("Deleting archive {Archive} for snapshot {Snapshot}.", snapshot.ArchiveName, snapshot.Name);
            File.Delete(snapshot.ArchiveName!);
            _logger.LogInformation("Deleted {Archive}", snapshot.ArchiveName);

            _logger.LogDebug(
                "Updating snapshot {SnapshotName} to be in the {Status} state.",
                snapshot.Name, Status.ArchiveRemoved);
            connection.Execute(
                "UPDATE snapshots SET status = @status WHERE name = @name;",
                new
                {
                    status = Status.ArchiveRemoved,
                    name = snapshot.Name
                });
        }
    }

    private ImmutableArray<Snapshot> GetUploadedSnapshots(string connectionString)
    {
        using SqliteConnection connection = new(connectionString);
        connection.Open();
        _logger.LogDebug("Getting snapshots in the {ArchiveUploaded} state in database {Database}.",
            Status.ArchiveUploaded,
            connection.Database);

        var query = @"
            SELECT name, status, archive_name as ArchiveName 
            FROM snapshots 
            WHERE status = @status;";
        ImmutableArray<Snapshot> snapshots = [..
            connection.Query<Snapshot>(query, new { status = Status.ArchiveUploaded })];
        _logger.LogDebug("Retrieved {SnapshotCount} snapshots.",
            snapshots.Length);

        return snapshots;
    }
}