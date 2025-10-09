using System.Formats.Tar;
using System.IO.Compression;
using AzureBackupTool.Options;
using Microsoft.Extensions.FileSystemGlobbing;
using Microsoft.Extensions.Options;

namespace AzureBackupTool;

public class Worker : BackgroundService
{
    private readonly ILogger<Worker> _logger;
    private readonly ProgramOptions _options;

    public Worker(
        ILogger<Worker> logger,
        IOptions<ProgramOptions> options)
    {
        _logger = logger;
        _options = options.Value;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        PeriodicTimer timer = new(TimeSpan.FromSeconds(10));
        do
        {
            _logger.LogInformation("Backing up files in {Path}", _options.Path);
        } while (await timer.WaitForNextTickAsync(stoppingToken));
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
