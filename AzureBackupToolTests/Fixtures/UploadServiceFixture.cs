using Azure.Storage;
using Azure.Storage.Blobs;
using Microsoft.Extensions.Configuration;

namespace AzureBackupToolTests.Fixtures;

public class UploadServiceFixture
{
    public readonly BlobServiceClient _blobServiceClient;

    public UploadServiceFixture()
    {
        var config = new ConfigurationBuilder()
            .SetBasePath(AppContext.BaseDirectory)
            .AddJsonFile("appsettings.json", true)
            .AddJsonFile("appsettings.Test.json", true)
            .Build();
        TestBlobClientOptions options = new();
        config.GetSection(TestBlobClientOptions.Key).Bind(options);

        _blobServiceClient = new(
            new Uri(options.BlobEndpoint),
            new StorageSharedKeyCredential(options.AccountName, options.AccountKey));
    }

    public BlobServiceClient BlobServiceClient => _blobServiceClient;
}