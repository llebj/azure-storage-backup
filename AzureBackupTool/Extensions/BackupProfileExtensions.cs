namespace AzureBackupTool.Extensions;

public static class BackupProfileExtensions
{
    public static ProfileInvocation GetNextInvocation(this BackupProfile profile, DateTimeOffset currentTime)
    {
        var nextOccurrence = profile.GetNextOccurence(currentTime) ?? 
            throw new InvalidOperationException($"The cron expression '{profile.Cron}' does not produce a valid next occurrence at '{currentTime}'.");

        return new(
            profile.Name,
            nextOccurrence,
            new InvocationSearchDefinition(
                profile.SearchDefinition.Directory,
                [.. profile.SearchDefinition.IncludePatterns],
                [.. profile.SearchDefinition.ExcludePatterns]));
    }
}