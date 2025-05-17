namespace AzureBackupTool;

public class BlobServiceClientSettings
{
    public const string Key = "BlobServiceClient";

    public string BlobEndpoint { get; set; } = string.Empty;

    public string ClientId { get; set; } = string.Empty;

    public string ClientSecret { get; set; } = string.Empty;

    public string TenantId { get; set; } = string.Empty;
}