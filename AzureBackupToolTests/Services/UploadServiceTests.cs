using Azure.Storage.Blobs;
using AzureBackupToolTests.Fixtures;
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

        _blobContainerClient = _fixture.BlobServiceClient.CreateBlobContainer($"test_{id}").Value;

        _dbPath = Path.Combine(Path.GetTempPath(), $"test_{id}.db");
        _connection = new SqliteConnection($"Data Source={_dbPath}");
        _connection.Open();

        InitialiseSchema();

        _service = new UploadService();
    }

    // UploadsArchivesForArchivedSnapshots

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

    private void InitialiseSchema()
    {
        var schemaPath = Path.Combine(AppContext.BaseDirectory, "create.sql");
        var schemaSql = File.ReadAllText(schemaPath);
        
        using var cmd = _connection.CreateCommand();
        cmd.CommandText = schemaSql;
        cmd.ExecuteNonQuery();
    }
}