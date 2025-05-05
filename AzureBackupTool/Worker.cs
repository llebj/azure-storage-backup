using System.Formats.Tar;
using System.IO.Compression;
using Azure.Storage.Blobs;
using Microsoft.Extensions.Options;

namespace AzureBackupTool;

public class Worker : BackgroundService
{
    private readonly ILogger<Worker> _logger;
    private readonly IOptions<List<BackupProfile>> _profiles;
    private readonly IOptions<BlobContainerSettings> _blobContainerSettings;
    private readonly BlobServiceClient _blobServiceClient;

    public Worker(
        ILogger<Worker> logger,
        IOptions<List<BackupProfile>> profiles,
        IOptions<BlobContainerSettings> blobContainerSettings,
        BlobServiceClient blobServiceClient)
    {
        _logger = logger;
        _profiles = profiles;
        _blobContainerSettings = blobContainerSettings;
        _blobServiceClient = blobServiceClient;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        while (!stoppingToken.IsCancellationRequested)
        {
            if (_logger.IsEnabled(LogLevel.Information))
            {
                _logger.LogInformation("Worker running at: {time}", DateTimeOffset.Now);
                _logger.LogInformation("Processing {ProfileCount} backup profiles.", _profiles.Value.Count);
            }

            foreach (var profile in _profiles.Value) 
            {
                // Read profile file
                if (!Directory.Exists(profile.SearchPath))
                {
                    _logger.LogInformation("Directory \"{Directory}\" does not exist.", profile.SearchPath);
                    continue;
                }
                _logger.LogInformation("Found directory \"{Directory}\".", profile.SearchPath);

                // Archive and zip file
                // TODO: Perform benchmarking and profiling to determine performance of uploading
                // from memory vs uploading from file.
                using MemoryStream ms = new();
                using GZipStream gz = new(ms, CompressionMode.Compress, leaveOpen: true);
                await TarFile.CreateFromDirectoryAsync(profile.SearchPath, gz, includeBaseDirectory: false, stoppingToken);
                gz.Close();
                ms.Position = 0;

                // Push to Azure
                // TODO: Perform basic validation on the profile name.
                var blobName = $"{profile.Name}.tar.gz";
                var blobClient= _blobServiceClient
                    .GetBlobContainerClient(_blobContainerSettings.Value.Name)
                    .GetBlobClient(blobName);
                _logger.LogInformation("Writing blob \"{Output}\".", blobName);
                await blobClient.UploadAsync(ms, cancellationToken: stoppingToken);
            }

            await Task.Delay(1_000_000, stoppingToken);
        }
    }
}
