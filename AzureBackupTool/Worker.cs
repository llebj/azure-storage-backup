using Microsoft.Extensions.Options;

namespace AzureBackupTool;

public class Worker : BackgroundService
{
    private readonly ILogger<Worker> _logger;
    private readonly IOptionsMonitor<List<BackupProfile>> _profilesMonitor;

    public Worker(
        ILogger<Worker> logger,
        IOptionsMonitor<List<BackupProfile>> profilesMonitor)
    {
        _logger = logger;
        _profilesMonitor = profilesMonitor;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        while (!stoppingToken.IsCancellationRequested)
        {
            if (_logger.IsEnabled(LogLevel.Information))
            {
                _logger.LogInformation("Worker running at: {time}", DateTimeOffset.Now);
                _logger.LogInformation("Processing {ProfileCount} backup profiles.", _profilesMonitor.CurrentValue.Count);
            }
            await Task.Delay(1000, stoppingToken);
        }
    }
}
