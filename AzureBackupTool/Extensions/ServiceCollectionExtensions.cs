using Azure.Identity;
using Azure.Storage;
using Azure.Storage.Blobs;
using AzureBackupTool.Options;
using Microsoft.Extensions.Azure;

namespace AzureBackupTool.Extensions;

public static class IServiceCollectionExtensions
{
    public static IServiceCollection ConfigureStorageClient(this IServiceCollection services, ConfigurationManager configuration, string storageHosting)
    {
        if (storageHosting == "local")
        {
            LocalStorageCredentials localStorageCredentials = new();
            configuration.GetSection(key: LocalStorageCredentials.Key).Bind(localStorageCredentials);
            BlobServiceClient serviceClient = new(
                serviceUri: new(localStorageCredentials.BlobEndpoint),
                credential: new StorageSharedKeyCredential(localStorageCredentials.AccountName, localStorageCredentials.AccountKey));
            services.AddSingleton(serviceClient);
        }
        else if (storageHosting == "cloud")
        {
            services.AddAzureClients(clientBuilder => 
            {
                CloudStorageCredentials cloudStorageCredentials = new();
                configuration.GetSection(key: CloudStorageCredentials.Key).Bind(cloudStorageCredentials);
                clientBuilder
                    .AddBlobServiceClient(new Uri(cloudStorageCredentials.BlobEndpoint))
                    .WithCredential(
                        new ClientSecretCredential(
                            cloudStorageCredentials.TenantId,
                            cloudStorageCredentials.ClientId,
                            cloudStorageCredentials.ClientSecret));
            });
        }
        else
        {
            throw new ArgumentOutOfRangeException(nameof(storageHosting), $"'{storageHosting}' is not a valid value");
        }
        return services;
    }
}