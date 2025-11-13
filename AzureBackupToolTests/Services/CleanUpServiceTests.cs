using System.Collections.Immutable;
using AzureBackupTool.Models;
using Dapper;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Logging.Abstractions;
using MsOptions = Microsoft.Extensions.Options;

namespace AzureBackupTool.Services.Tests;

public class CleanUpServiceTests : IDisposable
{
    private readonly string _directory;
    private readonly string _dbPath;
    private readonly SqliteConnection _connection;
    private readonly CleanUpService _service;

    // TODO: implement an abstract class for all of this boilerplate
    public CleanUpServiceTests()
    {
        var id = Guid.NewGuid();
        _directory = Path.Combine(Path.GetTempPath(), $"test_{id}");
        Directory.CreateDirectory(_directory);

        _dbPath = Path.Combine(Path.GetTempPath(), $"test_{id}.db");
        _connection = new SqliteConnection($"Data Source={_dbPath}");
        _connection.Open();

        InitialiseSchema();

        _service = new CleanUpService(
            new NullLogger<CleanUpService>(),
            MsOptions.Options.Create(new CleanupOptions(_dbPath)));
    }

    [Fact]
    public async Task DeletesArchivesForUploadedBlobs()
    {
        // Arrange
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        var batchPath = $"{batchDir}.txt";
        File.WriteAllText(batchPath, "content 1");

        var query = @"
            INSERT INTO snapshots (name, status, archive_name)
            VALUES (@name, @status, @archive);";
        _connection.Execute(query, new { name = batchDir, status = Status.ArchiveUploaded, archive = batchPath });

        // Act
        _service.RemoveArchives();

        // Assert
        var outputQuery = "SELECT name, status, archive_name as ArchiveName FROM snapshots WHERE name = @name;";
        ImmutableArray<Snapshot> archives = [.. _connection.Query<Snapshot>(outputQuery, new { name = batchDir })];
        Assert.Single(archives);
        Assert.Equal(Status.ArchiveRemoved, archives[0].Status);
        Assert.True(Directory.Exists(archives[0].Name));
        Assert.False(File.Exists(archives[0].ArchiveName));
    }

    [Fact]
    public async Task IgnoresDeletedArchives()
    {
        // Arrange
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        var batchPath = $"{batchDir}.txt";
        File.WriteAllText(batchPath, "content 1");

        var query = @"
            INSERT INTO snapshots (name, status, archive_name)
            VALUES (@name, @status, @archive);";
        _connection.Execute(query, new { name = batchDir, status = Status.ArchiveUploaded, archive = batchPath });

        // Act
        _service.RemoveArchives();
        _service.RemoveArchives();

        // Assert
        var outputQuery = "SELECT name, status, archive_name as ArchiveName FROM snapshots WHERE name = @name;";
        ImmutableArray<Snapshot> archives = [.. _connection.Query<Snapshot>(outputQuery, new { name = batchDir })];
        Assert.Single(archives);
        Assert.Equal(Status.ArchiveRemoved, archives[0].Status);
        Assert.True(Directory.Exists(archives[0].Name));
        Assert.False(File.Exists(archives[0].ArchiveName));
    }

    [Fact]
    public async Task IgnoresNonUploadedArchives()
    {
        // Arrange
        var dirOne = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(dirOne);
        var fileOne = $"{dirOne}.txt";
        File.WriteAllText(fileOne, "content 1");
        var queryOne = @"
            INSERT INTO snapshots (name, status, archive_name)
            VALUES (@name, @status, @archive);";
        _connection.Execute(queryOne, new { name = dirOne, status = Status.ArchiveUploaded, archive = fileOne });

        var dirTwo = Path.Combine(_directory, "batch_002");
        Directory.CreateDirectory(dirTwo);
        var fileTwo = $"{dirTwo}.txt";
        File.WriteAllText(fileTwo, "content 2");
        var queryTwo = @"
            INSERT INTO snapshots (name, status, archive_name)
            VALUES (@name, @status, @archive);";
        _connection.Execute(queryTwo, new { name = dirTwo, status = Status.ArchiveBuilt, archive = fileTwo });

        // Act
        _service.RemoveArchives();

        // Assert
        Assert.True(Directory.Exists(dirOne));
        Assert.False(File.Exists(fileOne));
        Assert.True(Directory.Exists(dirTwo));
        Assert.True(File.Exists(fileTwo));
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