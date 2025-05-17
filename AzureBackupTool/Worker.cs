using System.Formats.Tar;
using System.IO.Compression;
using Azure.Storage.Blobs;
using AzureBackupTool.Extensions;
using Microsoft.Extensions.Options;

namespace AzureBackupTool;

public class Worker : BackgroundService
{
    private readonly ILogger<Worker> _logger;
    private readonly IOptions<List<BackupProfile>> _profiles;
    private readonly IOptions<OutputSettings> _outputSettings;
    private readonly BlobServiceClient _blobServiceClient;
    private readonly ProfileInvocationSchedule _invocationSchedule = new();

    public Worker(
        ILogger<Worker> logger,
        IOptions<List<BackupProfile>> profiles,
        IOptions<OutputSettings> blobSettings,
        BlobServiceClient blobServiceClient)
    {
        _logger = logger;
        _profiles = profiles;
        _outputSettings = blobSettings;
        _blobServiceClient = blobServiceClient;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        if (_outputSettings.Value.Type == "fs")
        {
            _logger.LogInformation("Running in file system mode.");
        }

        while (!stoppingToken.IsCancellationRequested)
        {
            var currentTime = DateTimeOffset.Now;
            _logger.LogDebug("Evaluating {ProfileCount} backup profiles.", _profiles.Value.Count);
            foreach (var profile in _profiles.Value)
            {
                var invocation = profile.GetNextInvocation(currentTime);
                _invocationSchedule.ScheduleInvocation(invocation);
            }

            var pendingInvocations = _invocationSchedule.GetPendingInvocations(currentTime);
            _logger.LogDebug("Processing {InvocationCount} pending profile invocations.", pendingInvocations.Count);
            foreach (var invocation in pendingInvocations)
            {
                _logger.LogInformation("Executing the '{InvocationTime}' scheduled invocation of profile '{ProfileName}'.",
                    invocation.InvokeAt,
                    invocation.ProfileId);
                // Read profile file
                if (!Directory.Exists(invocation.SearchDefinition.Directory))
                {
                    _logger.LogInformation("Directory '{Directory}' does not exist.", invocation.SearchDefinition.Directory);
                    continue;
                }
                _logger.LogDebug("Found directory '{Directory}'.", invocation.SearchDefinition.Directory);

                // Archive and zip file
                // TODO: Perform basic validation on the profile name.
                // TODO: Perform benchmarking and profiling to determine performance of uploading
                // from memory vs uploading from file.
                using Stream stream = _outputSettings.Value.Type == "fs" ?
                    new FileStream(Path.Combine(_outputSettings.Value.Path, $"{invocation.ProfileId}.tar.gz"), FileMode.Create) :
                    new MemoryStream();
                using GZipStream gz = new(stream, CompressionMode.Compress, leaveOpen: true);
                await TarFile.CreateFromDirectoryAsync(invocation.SearchDefinition.Directory, gz, includeBaseDirectory: false, stoppingToken);
                gz.Close();

                if (_outputSettings.Value.Type == "fs")
                {
                    // Dump to file system
                    stream.Close();
                }
                else
                {
                    // Push to Azure
                    stream.Position = 0;
                    var blobName = $"{invocation.ProfileId}.tar.gz";
                    var blobClient = _blobServiceClient
                        .GetBlobContainerClient(_outputSettings.Value.Path)
                        .GetBlobClient(blobName);
                    _logger.LogInformation("Writing blob '{Output}'.", blobName);
                    await blobClient.UploadAsync(stream, overwrite: true, cancellationToken: stoppingToken);
                }
            }

            await Task.Delay(1_000, stoppingToken);
        }
    }
}
