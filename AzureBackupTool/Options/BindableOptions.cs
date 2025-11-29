namespace AzureBackupTool.Options;

// TODO: Implement a way of validating these options

public class ProgramOptions
{
    public const string Key = "Program";

    public string ArchiveOutputDirectory { get; set; } = string.Empty;

    public string DatabasePath { get; set; } = string.Empty;

    public string SourceDirectory { get; set; } = string.Empty;

    public string DestinationContainer { get; set; } = string.Empty;

    public string StorageHosting { get; set; } = string.Empty;
}

public class CloudStorageCredentials
{
    public const string Key = "CloudStorageCredentials";

    public string BlobEndpoint { get; set; } = string.Empty;

    public string ClientId { get; set; } = string.Empty;

    public string ClientSecret { get; set; } = string.Empty;

    public string TenantId { get; set; } = string.Empty;
}

public class LocalStorageCredentials
{
    public const string Key = "LocalStorageCredentials";

    public string BlobEndpoint { get; set; } = string.Empty;

    public string AccountName { get; set; } = string.Empty;

    public string AccountKey { get; set; } = string.Empty;
}