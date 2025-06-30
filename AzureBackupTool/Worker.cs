using System.Formats.Tar;
using System.IO.Compression;
using Azure.Storage.Blobs;
using Microsoft.Extensions.FileSystemGlobbing;
using Microsoft.Extensions.Options;

namespace AzureBackupTool;

public class Worker : BackgroundService
{
    private readonly ILogger<Worker> _logger;
    private readonly IOptions<OutputSettings> _outputSettings;
    private readonly BlobServiceClient _blobServiceClient;
    private readonly ProfileInvocationSource _invocationSource;
    private readonly ProfileInvocationSchedule _invocationSchedule = new();

    public Worker(
        ILogger<Worker> logger,
        IOptions<OutputSettings> blobSettings,
        BlobServiceClient blobServiceClient,
        ProfileInvocationSource backupProfileService)
    {
        _logger = logger;
        _outputSettings = blobSettings;
        _blobServiceClient = blobServiceClient;
        _invocationSource = backupProfileService;
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
            foreach (var invocation in _invocationSource.GetInvocations(currentTime))
            {
                _invocationSchedule.ScheduleInvocation(invocation);
            }

            var pendingInvocations = _invocationSchedule.GetPendingInvocations(currentTime);
            _logger.LogTrace("Processing {InvocationCount} pending profile invocations.", pendingInvocations.Count);
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
                await BuildArchive(stream, invocation.SearchDefinition, stoppingToken);

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

    private async ValueTask BuildArchive(Stream stream, ReadOnlySearchDefinition searchDefinition, CancellationToken cancellationToken)
    {
        Matcher matcher = new();
        matcher.AddIncludePatterns(searchDefinition.IncludePatterns);
        matcher.AddExcludePatterns(searchDefinition.ExcludePatterns);
        // TODO: Filter for only regular files
        IEnumerable<string> matchingFiles = matcher.GetResultsInFullPath(searchDefinition.Directory);
        if (_logger.IsEnabled(LogLevel.Debug))
        {
            _logger.LogDebug("Matched to following files: [{FileNames}]", string.Join(", ", matchingFiles));
        }

        using GZipStream gz = new(stream, CompressionMode.Compress, leaveOpen: true);
        using TarWriter writer = new(gz);
        foreach (var fileName in matchingFiles)
        {
            var relativePath = Path.GetRelativePath(searchDefinition.Directory, fileName);
            using var fileStream = File.OpenRead(fileName);
            PaxTarEntry entry = new(TarEntryType.RegularFile, relativePath)
            {
                DataStream = fileStream
            };
            await writer.WriteEntryAsync(entry, cancellationToken);
        }
    }
}
