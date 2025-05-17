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

    public SearchDefinition SearchDefinition { get; set; } = new();

    public DateTimeOffset? GetNextOccurence(DateTimeOffset currentTime) => _cronExpression.GetNextOccurrence(currentTime, TimeZoneInfo.Local);
}

public class SearchDefinition
{
    public string Directory { get; set; } = string.Empty;

    public List<string> IncludePatterns { get; set; } = [];

    public List<string> ExcludePatterns { get; set; } = [];
}