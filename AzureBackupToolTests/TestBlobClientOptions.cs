public sealed class TestBlobClientOptions
{
    public const string Key = "BlobServiceClient";

    public string BlobEndpoint { get; set; } = string.Empty;

    public string AccountName { get; set; } = string.Empty;

    public string AccountKey { get; set; } = string.Empty;
}