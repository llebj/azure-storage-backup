using Cronos;

namespace AzureBackupTool;

public class BackupProfile
{
    private string _cron = string.Empty;

    private CronExpression _cronExpression = CronExpression.Daily;

    public string Cron 
    { 
        get => _cron;
        set 
        {
            _cron = value;
            _cronExpression = CronExpression.Parse(value);
        }
    }

    public string Name { get; set; } = string.Empty;

    public string SearchPath { get; set; } = string.Empty;

    public ProfileInvocation GetNextInvocation(DateTimeOffset currentTime) 
    {
        var nextOccurrence = _cronExpression.GetNextOccurrence(currentTime, TimeZoneInfo.Utc) ?? 
            throw new InvalidOperationException($"The cron expression '{_cron}' does not produce a valid next occurrence at '{currentTime}'.");
        return new(Name, nextOccurrence, SearchPath);
    }
}