using System.Collections.Immutable;
using AzureBackupTool.Models;
using AzureBackupTool.Options;
using Dapper;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Options;

namespace AzureBackupTool.Services;

public class SnapshotRegistrationService
{
    private readonly ILogger<SnapshotRegistrationService> _logger;
    private readonly SnapshotRegistrationOptions _options;
    private readonly string _connectionString;

    public SnapshotRegistrationService(
        ILogger<SnapshotRegistrationService> logger,
        IOptions<SnapshotRegistrationOptions> options)
    {
        _logger = logger;
        _options = options.Value;
        SqliteConnectionStringBuilder connectionStringBuilder = new()
        {
            DataSource = options.Value.DatabasePath
        };
        _connectionString = connectionStringBuilder.ToString();
    }

    public void RegisterSnapshots()
    {
        var snapshots = GetNamesOfExistingSnapshots(_connectionString);
        var newSnapshotsToRegister = Directory
            .EnumerateDirectories(_options.TargetDirectoryPath)
            .Where(d => !snapshots.Contains(d))
            .Select(d => new Snapshot(d))
            .ToImmutableArray();

        if (newSnapshotsToRegister.Length == 0)
        {
            _logger.LogDebug("No new snapshots to register.");
            return;
        }

        InsertNewSnapshots(_connectionString, newSnapshotsToRegister);
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
}