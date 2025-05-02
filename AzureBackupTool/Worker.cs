using Microsoft.Extensions.Options;

namespace AzureBackupTool;

public class Worker : BackgroundService
{
    private readonly ILogger<Worker> _logger;
    private readonly IOptions<List<BackupProfile>> _profiles;

    public Worker(
        ILogger<Worker> logger,
        IOptions<List<BackupProfile>> profiles)
    {
        _logger = logger;
        _profiles = profiles;
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
            await Task.Delay(1000, stoppingToken);
        }
    }
}
