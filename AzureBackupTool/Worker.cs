using AzureBackupTool.Models;
using AzureBackupTool.Options;
using Dapper;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.FileSystemGlobbing;
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

    public Worker(
        ILogger<Worker> logger,
        IOptions<ProgramOptions> options)
    {
        _logger = logger;
        _options = options.Value;
        SqliteConnectionStringBuilder connectionStringBuilder = new()
        {
            DataSource = options.Value.DatabasePath
        };
        _connectionString = connectionStringBuilder.ToString();
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        PeriodicTimer timer = new(TimeSpan.FromSeconds(10));
        while (await timer.WaitForNextTickAsync(stoppingToken))
        {
            _logger.LogInformation("Backing up files in {Path}", _options.TargetDirectoryPath);
            RegisterSnapshots(_options.TargetDirectoryPath, _connectionString);
        } 
    }

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
            parameters.Add($"status{i}", (int)snapshots[i].Status);
        }
        var query = $@"
            INSERT INTO snapshots (name, status) 
            VALUES {string.Join(", ", values)};";

        var count = connection.Execute(query, parameters);
        _logger.LogInformation("Registered {Count} new snapshots.", count);
    }

    private async ValueTask BuildArchive(Stream stream, ReadOnlySearchDefinition searchDefinition, CancellationToken cancellationToken)
    {
        Matcher matcher = new();
        matcher.AddIncludePatterns(searchDefinition.IncludePatterns);
        matcher.AddExcludePatterns(searchDefinition.ExcludePatterns);
        // TODO: Filter for only regular files
        IEnumerable<string> matchingFiles = matcher.GetResultsInFullPath(searchDefinition.Directory);
        if (_logger.IsEnabled(LogLevel.Debug))
        {
            _logger.LogDebug("Matched to following files: [{FileNames}]", string.Join(", ", matchingFiles));
        }

        using GZipStream gz = new(stream, CompressionMode.Compress, leaveOpen: true);
        using TarWriter writer = new(gz);
        foreach (var fileName in matchingFiles)
        {
            var relativePath = Path.GetRelativePath(searchDefinition.Directory, fileName);
            using var fileStream = File.OpenRead(fileName);
            PaxTarEntry entry = new(TarEntryType.RegularFile, relativePath)
            {
                DataStream = fileStream
            };
            await writer.WriteEntryAsync(entry, cancellationToken);
        }
    }
}
