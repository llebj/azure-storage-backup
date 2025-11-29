using AzureBackupTool;
using AzureBackupTool.Extensions;
using AzureBackupTool.Options;
using AzureBackupTool.Services;
using Microsoft.Extensions.Hosting.Systemd;

var builder = Host.CreateApplicationBuilder(args);
var env = builder.Environment;
var basePath = AppContext.BaseDirectory;

builder.Configuration
    .SetBasePath(basePath)
    .AddJsonFile("appsettings.json")
    .AddJsonFile($"appsettings.{env.EnvironmentName}.json", optional: true);

if (env.IsProduction())
{
    // Optionally read from the global configuration defined in /etc
    builder.Configuration
        .AddJsonFile("/etc/azure-storage-backup/appsettings.json", optional: true);
}

ProgramOptions programOptions = new();
builder.Configuration.GetSection(ProgramOptions.Key).Bind(programOptions);

// TODO: Tidy up options registration
builder.Services.AddOptions<SnapshotRegistrationOptions>()
    .Configure(options => 
    {
        var built = programOptions.BuildSnapshotRegistrationOptions();
        options.DatabasePath = built.DatabasePath;
        options.SourceDirectory = built.SourceDirectory;
    });
builder.Services.AddSingleton<SnapshotRegistrationService>();

builder.Services.AddOptions<ArchivingOptions>()
    .Configure(options => 
    {
        var built = programOptions.BuildArchivingOptions();
        options.DatabasePath = built.DatabasePath;
        options.ArchiveOutputDirectory = built.ArchiveOutputDirectory;
    });
builder.Services.AddSingleton<ArchivingService>();

builder.Services.AddOptions<UploadOptions>()
    .Configure(options => 
    {
        var built = programOptions.BuildUploadOptions();
        options.DatabasePath = built.DatabasePath;
        options.DestinationContainer = built.DestinationContainer;
    });
builder.Services.AddSingleton<UploadService>();

builder.Services.AddOptions<CleanUpOptions>()
    .Configure(options => 
    {
        var built = programOptions.BuildCleanUpOptions();
        options.DatabasePath = built.DatabasePath;
    });
builder.Services.AddSingleton<CleanUpService>();

builder.Services.ConfigureStorageClient(builder.Configuration, programOptions.StorageHosting);

builder.Services.AddHostedService<Worker>();

builder.Services.AddSystemd();
if (SystemdHelpers.IsSystemdService())
{
    builder.Logging.AddSystemdConsole();
}

var host = builder.Build();
host.Run();
