using Cronos;

namespace AzureBackupTool.Extensions;

public static class BackupProfileExtensions
{
    public static ReadOnlyBackupProfile GetReadOnlyBackupProfile(this BackupProfile backupProfile) => new(
        backupProfile.Name,
        CronExpression.Parse(backupProfile.Cron),
        new ReadOnlySearchDefinition(
            backupProfile.SearchDefinition.Directory,
            [.. backupProfile.SearchDefinition.IncludePatterns],
            [.. backupProfile.SearchDefinition.ExcludePatterns]));

    public static ReadOnlyBackupProfileInvocation GetNextInvocation(
        this ReadOnlyBackupProfile profile,
        DateTimeOffset currentTime,
        CancellationToken cancellationToken)
    {
        var nextOccurrence = profile.GetNextOccurence(currentTime) ??
            throw new InvalidOperationException($"The cron expression '{profile.CronExpression}' does not produce a valid next occurrence at '{currentTime}'.");

        return new(
            profile.ProfileId,
            nextOccurrence,
            new ReadOnlySearchDefinition(
                profile.SearchDefinition.Directory,
                [.. profile.SearchDefinition.IncludePatterns],
                [.. profile.SearchDefinition.ExcludePatterns]),
                cancellationToken);
    }

    private static DateTimeOffset? GetNextOccurence(this ReadOnlyBackupProfile profile, DateTimeOffset currentTime) =>
        profile.CronExpression.GetNextOccurrence(currentTime, TimeZoneInfo.Local);
}