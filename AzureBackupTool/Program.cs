using Azure.Identity;
using AzureBackupTool;
using Microsoft.Extensions.Azure;

var builder = Host.CreateApplicationBuilder(args);
var env = builder.Environment;

builder.Configuration
    .AddJsonFile("appsettings.json", optional: true, reloadOnChange: true)
    .AddJsonFile($"appsettings.{env.EnvironmentName}.json", optional: true, reloadOnChange: true);

builder.Services.Configure<List<BackupProfile>>(builder.Configuration.GetSection(key: "Profiles"));
builder.Services.Configure<BlobContainerSettings>(builder.Configuration.GetSection(key: BlobContainerSettings.Key));

builder.Services.AddAzureClients(clientBuilder => 
{
    AzureSettings azureSettings = new();
    builder.Configuration.GetSection(key: AzureSettings.Key).Bind(azureSettings);
    clientBuilder
        .AddBlobServiceClient(new Uri(azureSettings.BlobEndpoint))
        .WithCredential(
            new ClientSecretCredential(
                azureSettings.TenantId,
                azureSettings.ClientId,
                azureSettings.ClientSecret));
});

builder.Services.AddHostedService<Worker>();

var host = builder.Build();
host.Run();
