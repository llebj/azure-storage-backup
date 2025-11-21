namespace AzureBackupTool.Options;

public class SnapshotRegistrationOptions
{
    public SnapshotRegistrationOptions() { }

    public SnapshotRegistrationOptions(string databasePath, string targetDirectoryPath)
    {
        DatabasePath = databasePath;
        TargetDirectoryPath = targetDirectoryPath;
    }

    public string DatabasePath { get; set; } = string.Empty;
    public string TargetDirectoryPath { get; set; } = string.Empty;
}

public class ArchivingOptions
{
    public ArchivingOptions() { }

    public ArchivingOptions(string databasePath)
    {
        DatabasePath = databasePath;
    }

    public string DatabasePath { get; set; } = string.Empty;
}

public class UploadOptions
{
    public UploadOptions() { }

    public UploadOptions(string databasePath, string output)
    {
        DatabasePath = databasePath;
        Output = output;
    }

    public string DatabasePath { get; set; } = string.Empty;
    public string Output { get; set; } = string.Empty;
}

public class CleanUpOptions
{
    public CleanUpOptions() { }

    public CleanUpOptions(string databasePath)
    {
        DatabasePath = databasePath;
    }

    public string DatabasePath { get; set; } = string.Empty;
}

public static class ProgramOptionsExtensions
{
    public static SnapshotRegistrationOptions BuildSnapshotRegistrationOptions(this ProgramOptions options)
        => new(options.DatabasePath, options.TargetDirectoryPath);

    public static ArchivingOptions BuildArchivingOptions(this ProgramOptions options)
        => new(options.DatabasePath);

    public static UploadOptions BuildUploadOptions(this ProgramOptions options)
        => new(options.DatabasePath, options.Output);

    public static CleanUpOptions BuildCleanUpOptions(this ProgramOptions options)
        => new(options.DatabasePath);
}
