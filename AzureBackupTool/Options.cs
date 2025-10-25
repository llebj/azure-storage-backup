namespace AzureBackupTool;

public record SnapshotRegistrationOptions(string DatabasePath, string TargetDirectoryPath);

public record ArchivingOptions(string DatabasePath);