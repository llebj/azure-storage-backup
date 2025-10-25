using AzureBackupTool.Models;
using Dapper;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Logging.Abstractions;
using MsOptions = Microsoft.Extensions.Options;
using System.Collections.Immutable;

namespace AzureBackupTool.Services.Tests;

public class SnapshotRegistrationServiceTests : IDisposable
{
    private readonly string _directory;
    private readonly string _dbPath;
    private readonly SqliteConnection _connection;
    private readonly SnapshotRegistrationService _service;

    public SnapshotRegistrationServiceTests()
    {
        var id = Guid.NewGuid();
        _directory = Path.Combine(Path.GetTempPath(), $"test_{id}");
        Directory.CreateDirectory(_directory);

        _dbPath = Path.Combine(Path.GetTempPath(), $"test_{id}.db");
        _connection = new SqliteConnection($"Data Source={_dbPath}");
        _connection.Open();

        InitialiseSchema();

        _service = new SnapshotRegistrationService(
            new NullLogger<SnapshotRegistrationService>(),
            MsOptions.Options.Create(new SnapshotRegistrationOptions(_dbPath, _directory)));
    }

    [Fact]
    public void CreatesEntriesForNewDirectories()
    {
        // Arrange: Create test directories with some files
        var dir1 = Path.Combine(_directory, "batch_001");
        var dir2 = Path.Combine(_directory, "batch_002");
        Directory.CreateDirectory(dir1);
        Directory.CreateDirectory(dir2);
        
        File.WriteAllText(Path.Combine(dir1, "file1.txt"), "test content");
        File.WriteAllText(Path.Combine(dir2, "file2.txt"), "test content");

        // Act
        _service.RegisterSnapshots();

        // Assert
        var query = "SELECT name FROM snapshots WHERE status = @status;";
        ImmutableArray<string> snapshotNames = [.. _connection.Query<string>(query, new { status = Status.Registered })];
        
        Assert.Equal(2, snapshotNames.Length);
    }

    [Fact]
    public void IgnoresExistingDirectories()
    {
        // Arrange
        var dir1 = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(dir1);
        File.WriteAllText(Path.Combine(dir1, "file1.txt"), "test content");

        // Act
        _service.RegisterSnapshots();
        _service.RegisterSnapshots();

        // Assert
        var query = "SELECT name FROM snapshots WHERE status = @status;";
        ImmutableArray<string> snapshotNames = [.. _connection.Query<string>(query, new { status = Status.Registered })];
        
        Assert.Single(snapshotNames);
    }

    [Fact]
    public void IgnoresFiles()
    {
        // Arrange
        File.WriteAllText(Path.Combine(_directory, "file1.txt"), "test content");

        // Act
        _service.RegisterSnapshots();

        // Assert
        var query = "SELECT name FROM snapshots WHERE status = @status;";
        ImmutableArray<string> snapshotNames = [.. _connection.Query<string>(query, new { status = Status.Registered })];
        
        Assert.Empty(snapshotNames);
    }

    public void Dispose()
    {
        _connection?.Dispose();

        if (File.Exists(_dbPath))
        {
            File.Delete(_dbPath);
        }

        if (Directory.Exists(_directory))
        {
            Directory.Delete(_directory, recursive: true);
        }
    }

    private void InitialiseSchema()
    {
        var schemaPath = Path.Combine(AppContext.BaseDirectory, "create.sql");
        var schemaSql = File.ReadAllText(schemaPath);
        
        using var cmd = _connection.CreateCommand();
        cmd.CommandText = schemaSql;
        cmd.ExecuteNonQuery();
    }
}