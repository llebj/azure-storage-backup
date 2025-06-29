using System.Collections.Immutable;
using Cronos;

namespace AzureBackupTool;

// Can these two settings classes be records?
public class BackupProfile
{
    // TODO: Validate that this cron expression is valid
    public string Cron { get; set; } = string.Empty;

    // TODO: Validate that this value is unique
    public string Name { get; set; } = string.Empty;

    public SearchDefinition SearchDefinition { get; set; } = new();

    
}

public class SearchDefinition
{
    public string Directory { get; set; } = string.Empty;

    public List<string> IncludePatterns { get; set; } = [];

    public List<string> ExcludePatterns { get; set; } = [];
}

public readonly record struct ReadOnlyBackupProfile(
    string ProfileId, 
    CronExpression CronExpression, 
    ReadOnlySearchDefinition SearchDefinition)
{
    public bool Equals(ReadOnlyBackupProfile other) =>
        ProfileId == other.ProfileId &&
        CronExpression.ToString() == other.CronExpression.ToString() &&
        SearchDefinition.Equals(other.SearchDefinition);

    public override int GetHashCode() => HashCode.Combine(ProfileId, CronExpression.ToString(), SearchDefinition);
}

public readonly record struct ReadOnlySearchDefinition(
    string Directory, 
    ImmutableArray<string> IncludePatterns, 
    ImmutableArray<string> ExcludePatterns)
{
    public bool Equals(ReadOnlySearchDefinition other) =>
        Directory == other.Directory &&
        IncludePatterns.SequenceEqual(other.IncludePatterns) &&
        ExcludePatterns.SequenceEqual(other.ExcludePatterns);

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(Directory);
        foreach (var pattern in IncludePatterns)
        {
            hash.Add(pattern);
        }
        foreach (var pattern in ExcludePatterns)
        {
            hash.Add(pattern);
        }
        return hash.ToHashCode();
    }
}

public readonly record struct ReadOnlyBackupProfileInvocation(
    string ProfileId,
    DateTimeOffset InvokeAt,
    ReadOnlySearchDefinition SearchDefinition,
    CancellationToken CancellationToken);
