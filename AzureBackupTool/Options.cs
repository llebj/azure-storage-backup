namespace AzureBackupTool;

public record SnapshotRegistrationOptions(string DatabasePath, string TargetDirectoryPath);

public record ArchivingOptions(string DatabasePath);

public record UploadOptions(string DatabasePath, string Output);

public record CleanupOptions(string DatabasePath);