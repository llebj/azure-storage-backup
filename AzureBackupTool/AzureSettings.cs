namespace AzureBackupTool;

public class AzureSettings
{
    public const string Key = "Azure";

    public string BlobEndpoint { get; set; } = string.Empty;

    public string ClientId { get; set; } = string.Empty;

    public string ClientSecret { get; set; } = string.Empty;

    public string TenantId { get; set; } = string.Empty;
}