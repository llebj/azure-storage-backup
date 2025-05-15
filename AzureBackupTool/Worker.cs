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
    private readonly ProfileInvocationSchedule _invocationSchedule = new();

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
            var currentTime = DateTimeOffset.Now;
            _logger.LogInformation("Evaluating {ProfileCount} backup profiles.", _profiles.Value.Count);
            foreach (var profile in _profiles.Value)
            {
                var invocation = profile.GetNextInvocation(currentTime);
                _invocationSchedule.ScheduleInvocation(invocation);
            }

            var pendingInvocations = _invocationSchedule.GetPendingInvocations(currentTime);
            _logger.LogInformation("Processing {InvocationCount} pending profile invocations.",pendingInvocations);
            foreach (var invocation in pendingInvocations) 
            {
                _logger.LogInformation("Executing the '{InvocationTime}' scheduled invocation of profile '{ProfileName}'.",
                    invocation.ProfileId,
                    invocation.InvokeAt);
                // Read profile file
                if (!Directory.Exists(invocation.SearchPath))
                {
                    _logger.LogInformation("Directory '{Directory}' does not exist.", invocation.SearchPath);
                    continue;
                }
                _logger.LogInformation("Found directory '{Directory}'.", invocation.SearchPath);

                // Archive and zip file
                // TODO: Perform benchmarking and profiling to determine performance of uploading
                // from memory vs uploading from file.
                using MemoryStream ms = new();
                using GZipStream gz = new(ms, CompressionMode.Compress, leaveOpen: true);
                await TarFile.CreateFromDirectoryAsync(invocation.SearchPath, gz, includeBaseDirectory: false, stoppingToken);
                gz.Close();
                ms.Position = 0;

                // Push to Azure
                // TODO: Perform basic validation on the profile name.
                var blobName = $"{invocation.ProfileId}.tar.gz";
                var blobClient= _blobServiceClient
                    .GetBlobContainerClient(_blobContainerSettings.Value.Name)
                    .GetBlobClient(blobName);
                _logger.LogInformation("Writing blob '{Output}'.", blobName);
                // TODO: Allow blobs to be overwritten. 
                await blobClient.UploadAsync(ms, cancellationToken: stoppingToken);
            }

            await Task.Delay(1_000, stoppingToken);
        }
    }
}
