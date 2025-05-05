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
                // TODO: Perform basic validation on the profile name.
                var outputFileName = $"{profile.Name}.tar.gz";
                var outputFile = Path.Join(profile.OutputDirectory, outputFileName);
                _logger.LogInformation("Writing to \"{Output}\".", outputFile);
                // This 'FileMode' causes the output file to be overwritten if it already exists.
                // TODO: Use a temporary file for each backup run that is then removed after it has been uploaded.
                using FileStream fs = new(outputFile, FileMode.Create, FileAccess.Write);
                using GZipStream gz = new(fs, CompressionMode.Compress, leaveOpen: true);
                await TarFile.CreateFromDirectoryAsync(profile.SearchPath, gz, includeBaseDirectory: false, stoppingToken);
                gz.Close();
                fs.Close();

                // Push to Azure
                var blobContainerClient = _blobServiceClient.GetBlobContainerClient(_blobContainerSettings.Value.Name);
                var blobClient = blobContainerClient.GetBlobClient(outputFileName);
                _logger.LogInformation("Reading from \"{Output}\" for upload.", outputFile);
                await blobClient.UploadAsync(outputFile, cancellationToken: stoppingToken);

                File.Delete(outputFile);
            }

            await Task.Delay(1_000_000, stoppingToken);
        }
    }
}
