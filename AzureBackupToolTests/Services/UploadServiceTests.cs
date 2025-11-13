using Azure.Storage.Blobs;
using AzureBackupTool.Models;
using AzureBackupToolTests.Fixtures;
using Dapper;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Logging.Abstractions;
using MsOptions = Microsoft.Extensions.Options;
using System.Security.Cryptography;
using System.Collections.Immutable;
using Azure;

namespace AzureBackupTool.Services.Tests;

public class UploadServiceTests : IClassFixture<UploadServiceFixture>, IDisposable
{
    private readonly UploadServiceFixture _fixture;
    private readonly string _directory;
    private readonly BlobContainerClient _blobContainerClient;
    private readonly string _dbPath;
    private readonly SqliteConnection _connection;
    private readonly UploadService _service;

    public UploadServiceTests(UploadServiceFixture fixture)
    {
        _fixture = fixture;

        var id = Guid.NewGuid();

        _directory = Path.Combine(Path.GetTempPath(), $"test_{id}");
        Directory.CreateDirectory(_directory);

        var containerName = $"test{id:N}".ToLowerInvariant();
        var blobServiceClient = _fixture.BlobServiceClient;
        _blobContainerClient = blobServiceClient.CreateBlobContainer(containerName).Value;

        _dbPath = Path.Combine(Path.GetTempPath(), $"test_{id}.db");
        _connection = new SqliteConnection($"Data Source={_dbPath}");
        _connection.Open();

        InitialiseSchema();

        _service = new UploadService(
            new NullLogger<UploadService>(),
            MsOptions.Options.Create(new UploadOptions(_dbPath, containerName)),
            blobServiceClient);
    }

    [Fact]
    public async Task UploadsArchivesForArchivedSnapshots()
    {
        // Arrange
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        var batchPath = $"{batchDir}.txt";
        File.WriteAllText(batchPath, "content 1");
        var contentHash = ComputeMD5HashBase64(batchPath);

        var query = @"
            INSERT INTO snapshots (name, status, archive_name)
            VALUES (@name, @status, @archive);";
        _connection.Execute(query, new { name = batchDir, status = Status.ArchiveBuilt, archive = batchPath });

        // Act
        await _service.UploadArchives(CancellationToken.None);

        // Assert
        var outputQuery = "SELECT name, status, archive_name as ArchiveName FROM snapshots WHERE status = @status;";
        ImmutableArray<Snapshot> archives = [.. _connection.Query<Snapshot>(outputQuery, new { status = Status.ArchiveUploaded })];
        Assert.Single(archives);
        Assert.Equal(batchDir, archives[0].Name);
        Assert.Equal(batchPath, archives[0].ArchiveName);
        Assert.Equal(Status.ArchiveUploaded, archives[0].Status);

        var blobName = Path.GetFileName(batchPath);
        var blobClient = _blobContainerClient
            .GetBlobClient(blobName);

        var blobExists = await blobClient
            .ExistsAsync();
        Assert.True(blobExists.Value);

        var blobProperties = await blobClient.GetPropertiesAsync();
        Assert.Equal(contentHash, Convert.ToBase64String(blobProperties.Value.ContentHash));
    }

    [Fact]
    public async Task IgnoresUploadedArchives()
    {
        // Arrange
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        var batchPath = $"{batchDir}.txt";
        File.WriteAllText(batchPath, "content 1");
        var contentHash = ComputeMD5HashBase64(batchPath);

        var query = @"
            INSERT INTO snapshots (name, status, archive_name)
            VALUES (@name, @status, @archive);";
        _connection.Execute(query, new { name = batchDir, status = Status.ArchiveBuilt, archive = batchPath });

        // Act
        await _service.UploadArchives(CancellationToken.None);
        await _service.UploadArchives(CancellationToken.None);

        // Assert
        var outputQuery = "SELECT name, status, archive_name as ArchiveName FROM snapshots WHERE status = @status;";
        ImmutableArray<Snapshot> archives = [.. _connection.Query<Snapshot>(outputQuery, new { status = Status.ArchiveUploaded })];
        Assert.Single(archives);
        Assert.Equal(batchDir, archives[0].Name);
        Assert.Equal(batchPath, archives[0].ArchiveName);
        Assert.Equal(Status.ArchiveUploaded, archives[0].Status);

        var blobName = Path.GetFileName(batchPath);
        var blobClient = _blobContainerClient
            .GetBlobClient(blobName);

        var blobExists = await blobClient
            .ExistsAsync();
        Assert.True(blobExists.Value);

        var blobProperties = await blobClient.GetPropertiesAsync();
        Assert.Equal(contentHash, Convert.ToBase64String(blobProperties.Value.ContentHash));
    }

    [Fact]
    public async Task ThrowsIfABlobAlreadyExistsForAnArchivedSnapshot()
    {
        // Arrange
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        var batchPath = $"{batchDir}.txt";
        File.WriteAllText(batchPath, "content 1");
        var contentHash = ComputeMD5HashBase64(batchPath);

        var query = @"
            INSERT INTO snapshots (name, status, archive_name)
            VALUES (@name, @status, @archive);";
        _connection.Execute(query, new { name = batchDir, status = Status.ArchiveBuilt, archive = batchPath });

        using FileStream stream = File.OpenRead(batchPath);
        var blobClient = _blobContainerClient
            .GetBlobClient(Path.GetFileName(batchPath));
        await blobClient.UploadAsync(stream, CancellationToken.None);

        // Act & Assert
        await Assert.ThrowsAsync<RequestFailedException>(async () => { await _service.UploadArchives(CancellationToken.None); });
    }

    [Fact]
    public async Task LeavesSystemInARecoverableStateUponCancellation()
    {
        // Arrange
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        var batchPath = $"{batchDir}.txt";
        File.WriteAllText(batchPath, "content 1");

        var query = @"
            INSERT INTO snapshots (name, status, archive_name)
            VALUES (@name, @status, @archive);";
        _connection.Execute(query, new { name = batchDir, status = Status.ArchiveBuilt, archive = batchPath });

        CancellationTokenSource cts = new();
        cts.Cancel();

        // Act
        await Assert.ThrowsAnyAsync<OperationCanceledException>(async () => await _service.UploadArchives(cts.Token));

        // Assert
        var outputQuery = "SELECT name, status, archive_name as ArchiveName FROM snapshots WHERE name = @name;";
        var snapshot = _connection.QuerySingle<Snapshot>(outputQuery, new { name = batchDir });
        Assert.Equal(Status.ArchiveUploading, snapshot.Status);
    }

    [Fact]
    public async Task CanRecoverFromInterruptedArchiveUpload()
    {
        // Arrange
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        var batchPath = $"{batchDir}.txt";
        File.WriteAllText(batchPath, "content 1");
        var contentHash = ComputeMD5HashBase64(batchPath);

        // Upload the blob to simulate failure after upload to ensure correct clean-up.
        var blobName = Path.GetFileName(batchPath);
        var blobClient = _blobContainerClient.GetBlobClient(blobName);
        await blobClient.UploadAsync(batchPath);

        // A status of `UploadingArchive` indicates that the process was interrupted. This
        // is a recoverable state.
        var query = @"
            INSERT INTO snapshots (name, status, archive_name)
            VALUES (@name, @status, @archive);";
        _connection.Execute(query, new { name = batchDir, status = Status.ArchiveUploading, archive = batchPath });

        // Act
        await _service.UploadArchives(CancellationToken.None);

        // Assert
        var outputQuery = "SELECT name, status, archive_name as ArchiveName FROM snapshots WHERE status = @status;";
        ImmutableArray<Snapshot> archives = [.. _connection.Query<Snapshot>(outputQuery, new { status = Status.ArchiveUploaded })];
        Assert.Single(archives);
        Assert.Equal(batchDir, archives[0].Name);
        Assert.Equal(batchPath, archives[0].ArchiveName);
        Assert.Equal(Status.ArchiveUploaded, archives[0].Status);

        var blobExists = await blobClient
            .ExistsAsync();
        Assert.True(blobExists.Value);

        var blobProperties = await blobClient.GetPropertiesAsync();
        Assert.Equal(contentHash, Convert.ToBase64String(blobProperties.Value.ContentHash));
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

        if (_blobContainerClient.Exists())
        {
            _blobContainerClient.Delete();
        }
    }

    private string ComputeMD5HashBase64(string filePath)
    {
        using var md5 = MD5.Create();
        using var stream = File.OpenRead(filePath);
        byte[] hash = md5.ComputeHash(stream);
        return Convert.ToBase64String(hash);
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