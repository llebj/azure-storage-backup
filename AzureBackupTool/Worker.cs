using AzureBackupTool.Services;

namespace AzureBackupTool;

public class Worker : BackgroundService
{
    private readonly ILogger<Worker> _logger;
    private readonly SnapshotRegistrationService _snapshotRegistrationService;
    private readonly ArchivingService _archivingService;
    private readonly UploadService _uploadService;
    private readonly CleanUpService _cleanUpService;

    public Worker(
        ILogger<Worker> logger,
        SnapshotRegistrationService snapshotRegistrationService,
        ArchivingService archivingService,
        UploadService uploadService,
        CleanUpService cleanUpService)
    {
        _logger = logger;
        _snapshotRegistrationService = snapshotRegistrationService;
        _archivingService = archivingService;
        _uploadService = uploadService;
        _cleanUpService = cleanUpService;
    }

    // TODO: Perform benchmarking of these operations
    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        PeriodicTimer timer = new(TimeSpan.FromSeconds(10));
        while (await timer.WaitForNextTickAsync(stoppingToken))
        {
            _logger.LogInformation("Executing backup procedure at {Time}", DateTimeOffset.UtcNow);
            _snapshotRegistrationService.RegisterSnapshots();
            await _archivingService.BuildArchives(stoppingToken);
            await _uploadService.UploadArchives(stoppingToken);
            _cleanUpService.RemoveArchives();
        }
    }
}
