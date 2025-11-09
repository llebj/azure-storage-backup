using System.Reflection.Metadata;
using System.Security.Cryptography;
using Azure.Storage.Blobs;
using AzureBackupTool.Models;
using AzureBackupToolTests.Fixtures;
using Dapper;
using Microsoft.Data.Sqlite;

namespace AzureBackupTool.Services.Tests;

public class UploadServiceTests : IClassFixture<UploadServiceFixture>
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
        _blobContainerClient = _fixture.BlobServiceClient.CreateBlobContainer(containerName).Value;

        _dbPath = Path.Combine(Path.GetTempPath(), $"test_{id}.db");
        _connection = new SqliteConnection($"Data Source={_dbPath}");
        _connection.Open();

        InitialiseSchema();

        _service = new UploadService();
    }

    [Fact]
    public async Task UploadsArchivesForArchivedSnapshots()
    {
        // Arrange
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        var batchPath = Path.Combine(batchDir, "file1.txt");
        File.WriteAllText(batchPath, "content 1");
        var contentHash = ComputeMD5HashBase64(batchPath);

        var query = @"
            INSERT INTO snapshots (name, status, archive_name)
            VALUES (@name, @status, @archive);";
        _connection.Execute(query, new { name = batchDir, status = Status.ArchiveBuilt, archive = batchPath });

        // Act
        await _service.UploadArchives(CancellationToken.None);

        // Assert
        var blobName = Path.GetFileName(batchPath);
        var blobClient = _blobContainerClient
            .GetBlobClient(blobName);

        var blobExists = await blobClient
            .ExistsAsync();
        Assert.True(blobExists.Value);

        var blobProperties = await blobClient.GetPropertiesAsync();
        Assert.Equal(contentHash, Convert.ToBase64String(blobProperties.Value.ContentHash));
    }

    // IgnoresUploadedArchives

    // ThrowsIfABlobAlreadyExistsForAnArchivedSnapshot

    // LeavesSystemInARecoverableStateUponCancellation

    // CanRecoverFromInterruptedArchiveUpload

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