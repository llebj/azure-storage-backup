using System.Collections.Immutable;
using System.Formats.Tar;
using System.IO.Compression;
using System.Text;
using AzureBackupTool.Models;
using AzureBackupTool.Options;
using Dapper;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Logging.Abstractions;
using MsOptions = Microsoft.Extensions.Options;

namespace AzureBackupTool.Services.Tests;

public class ArchivingServiceTests : IDisposable
{
    private readonly string _directory;
    private readonly string _dbPath;
    private readonly SqliteConnection _connection;
    private readonly ArchivingService _service;

    public ArchivingServiceTests()
    {
        var id = Guid.NewGuid();
        _directory = Path.Combine(Path.GetTempPath(), $"test_{id}");
        Directory.CreateDirectory(_directory);

        _dbPath = Path.Combine(Path.GetTempPath(), $"test_{id}.db");
        _connection = new SqliteConnection($"Data Source={_dbPath}");
        _connection.Open();

        InitialiseSchema();

        _service = new ArchivingService(
            new NullLogger<ArchivingService>(),
            MsOptions.Options.Create(new ArchivingOptions(_dbPath)));
    }

    [Fact]
    public async Task CreatesArchivesForRegisteredSnapshots()
    {
        // Arrange
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        File.WriteAllText(Path.Combine(batchDir, "file1.txt"), "content 1");
        File.WriteAllText(Path.Combine(batchDir, "file2.txt"), "content 2");

        var subDir = Path.Combine(batchDir, "subdir");
        Directory.CreateDirectory(subDir);
        File.WriteAllText(Path.Combine(subDir, "file3.txt"), "content 3");

        var deepDir = Path.Combine(subDir, "nested");
        Directory.CreateDirectory(deepDir);
        File.WriteAllText(Path.Combine(deepDir, "file4.txt"), "content 4");

        var hiddenDir = Path.Combine(batchDir, ".hidden");
        Directory.CreateDirectory(hiddenDir);
        File.WriteAllText(Path.Combine(hiddenDir, "file5.txt"), "content 5");

        var query = @"
            INSERT INTO snapshots (name, status)
            VALUES (@name, @status);";
        _connection.Execute(query, new { name = batchDir, status = Status.Registered });

        // Act
        await _service.BuildArchives(CancellationToken.None);

        // Assert
        var archivePath = Path.Join(_directory, "batch_001.tar.gz");
        var outputQuery = "SELECT name, status, archive_name as ArchiveName FROM snapshots WHERE status = @status;";
        ImmutableArray<Snapshot> archives = [.. _connection.Query<Snapshot>(outputQuery, new { status = Status.ArchiveBuilt })];
        Assert.Single(archives);
        Assert.Equal(batchDir, archives[0].Name);
        Assert.Equal($"{batchDir}.tar.gz", archives[0].ArchiveName);
        Assert.Equal(Status.ArchiveBuilt, archives[0].Status);

        Assert.True(File.Exists(archivePath));
        var (files, directories) = await ExtractArchiveContents(archivePath);

        // Verify all files are present with correct content
        Assert.Equal(5, files.Count);
        Assert.Contains(files, kvp => kvp.Key.EndsWith("file1.txt") && kvp.Value == "content 1");
        Assert.Contains(files, kvp => kvp.Key.EndsWith("file2.txt") && kvp.Value == "content 2");
        Assert.Contains(files, kvp => kvp.Key.EndsWith("subdir/file3.txt") && kvp.Value == "content 3");
        Assert.Contains(files, kvp => kvp.Key.EndsWith("nested/file4.txt") && kvp.Value == "content 4");
        Assert.Contains(files, kvp => kvp.Key.EndsWith(".hidden/file5.txt") && kvp.Value == "content 5");

        // Verify directory structure is preserved
        Assert.True(directories.Count > 0);
        Assert.Contains(directories, name => name.Contains("subdir"));
        Assert.Contains(directories, name => name.Contains("nested"));
        Assert.Contains(directories, name => name.Contains(".hidden"));
    }

    [Fact]
    public async Task IgnoresArchivedSnapshots()
    {
        // Arrange
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        File.WriteAllText(Path.Combine(batchDir, "file1.txt"), "content 1");

        var query = @"
            INSERT INTO snapshots (name, status) 
            VALUES (@name, @status)";
        _connection.Execute(query, new { name = batchDir, status = Status.Registered });

        // Act
        await _service.BuildArchives(CancellationToken.None);
        await _service.BuildArchives(CancellationToken.None);

        // Assert: Only one archive exists, status correct
        var archivePath = Path.Join(_directory, "batch_001.tar.gz");
        var outputQuery = "SELECT name, status, archive_name as ArchiveName FROM snapshots WHERE status = @status;";
        ImmutableArray<Snapshot> archives = [.. _connection.Query<Snapshot>(outputQuery, new { status = Status.ArchiveBuilt })];
        Assert.Single(archives);
        Assert.Equal(batchDir, archives[0].Name);
        Assert.Equal(archivePath, archives[0].ArchiveName);
        Assert.Equal(Status.ArchiveBuilt, archives[0].Status);

        Assert.True(File.Exists(archivePath));
        var (files, directories) = await ExtractArchiveContents(archivePath);

        // Verify all files are present with correct content
        Assert.Single(files);
        Assert.Contains(files, kvp => kvp.Key.EndsWith("file1.txt") && kvp.Value == "content 1");
    }

    [Fact]
    public async Task ThrowsIfAnArchiveAlreadyExistsForARegisteredSnapshot()
    {
        // Arrange
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        File.WriteAllText(Path.Combine(batchDir, "file1.txt"), "content 1");

        var query = @"
            INSERT INTO snapshots (name, status) 
            VALUES (@name, @status)";
        _connection.Execute(query, new { name = batchDir, status = Status.Registered });

        // Create a file with the output archive name
        var archivePath = Path.Join(_directory, "batch_001.tar.gz");
        File.WriteAllText(archivePath, "g-zip content");

        // Act & Assert
        await Assert.ThrowsAsync<InvalidOperationException>(async () => { await _service.BuildArchives(CancellationToken.None); });
    }

    [Fact]
    public async Task LeavesSystemInRecoverableStateUponCancellation()
    {
        // Arrange
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        File.WriteAllText(Path.Combine(batchDir, "file1.txt"), "content 1");

        var query = @"
            INSERT INTO snapshots (name, status) 
            VALUES (@name, @status)";
        _connection.Execute(query, new { name = batchDir, status = Status.Registered });

        var cts = new CancellationTokenSource();
        cts.Cancel();

        // Act & Assert
        await Assert.ThrowsAnyAsync<OperationCanceledException>(async () => await _service.BuildArchives(cts.Token));

        var outputQuery = "SELECT name, status, archive_name as ArchiveName FROM snapshots WHERE name = @name;";
        var snapshot = _connection.QuerySingle<Snapshot>(outputQuery, new { name = batchDir });
        Assert.Equal(Status.ArchiveBuilding, snapshot.Status);
    }

    [Fact]
    public async Task CanRecoverFromInterruptedArchiveBuild()
    {
        // Arrange
        // Create the snapshot directory and contents
        var batchDir = Path.Combine(_directory, "batch_001");
        Directory.CreateDirectory(batchDir);
        File.WriteAllText(Path.Combine(batchDir, "file1.txt"), "content 1");

        // Create a corrupted g-zip file to emulate a previously failed execution
        var archivePath = Path.Combine(_directory, "batch_001.tar.gz");
        File.WriteAllText(archivePath, "partial g-zip content");

        // A status of `BuildingArchive` indicates that the process was interrupted. This
        // is a recoverable state.
        var query = @"
            INSERT INTO snapshots (name, status, archive_name) 
            VALUES (@name, @status, @archive)";
        _connection.Execute(query, new { name = batchDir, status = Status.ArchiveBuilding, archive = archivePath });

        // Act
        await _service.BuildArchives(CancellationToken.None);

        // Assert
        var outputQuery = @"SELECT name, status, archive_name as ArchiveName FROM snapshots WHERE status = @status;";
        ImmutableArray<Snapshot> snapshots = [.. _connection.Query<Snapshot>(outputQuery, new { status = Status.ArchiveBuilt })];

        // Assert that the snapshot record is valid
        Assert.Single(snapshots);
        Assert.Equal(batchDir, snapshots[0].Name);
        Assert.Equal(archivePath, snapshots[0].ArchiveName);
        Assert.Equal(Status.ArchiveBuilt, snapshots[0].Status);

        // Assert that the archive contents are valid
        Assert.True(File.Exists(archivePath));
        var (files, directories) = await ExtractArchiveContents(archivePath);
        Assert.Single(files);
        Assert.Contains(files, kvp => kvp.Key.EndsWith("file1.txt") && kvp.Value == "content 1");
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

    private static async Task<(Dictionary<string, string> Files, HashSet<string> Directories)> ExtractArchiveContents(string archivePath)
    {
        await using var fileStream = File.OpenRead(archivePath);
        await using var gzipStream = new GZipStream(fileStream, CompressionMode.Decompress);
        var files = new Dictionary<string, string>();
        var directories = new HashSet<string>();
        var reader = new TarReader(gzipStream);
        while (await reader.GetNextEntryAsync() is { } entry)
        {
            if (entry.EntryType == TarEntryType.Directory)
            {
                directories.Add(entry.Name);
            }
            else if (entry.EntryType == TarEntryType.RegularFile && entry.DataStream != null)
            {
                using var contentStream = new MemoryStream();
                await entry.DataStream.CopyToAsync(contentStream);
                var content = Encoding.UTF8.GetString(contentStream.ToArray());
                files.Add(entry.Name, content);
            }
        }

        return (files, directories);
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