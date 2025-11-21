using AzureBackupTool;
using AzureBackupTool.Extensions;
using AzureBackupTool.Options;
using AzureBackupTool.Services;

var builder = Host.CreateApplicationBuilder(args);
var env = builder.Environment;

builder.Configuration
    .AddJsonFile("appsettings.json", optional: true, reloadOnChange: true)
    .AddJsonFile($"appsettings.{env.EnvironmentName}.json", optional: true, reloadOnChange: true);

ProgramOptions programOptions = new();
builder.Configuration.GetSection(ProgramOptions.Key).Bind(programOptions);

// TODO: Tidy up options registration
builder.Services.AddOptions<SnapshotRegistrationOptions>()
    .Configure(options => 
    {
        var built = programOptions.BuildSnapshotRegistrationOptions();
        options.DatabasePath = built.DatabasePath;
        options.TargetDirectoryPath = built.TargetDirectoryPath;
    });
builder.Services.AddSingleton<SnapshotRegistrationService>();

builder.Services.AddOptions<ArchivingOptions>()
    .Configure(options => 
    {
        var built = programOptions.BuildArchivingOptions();
        options.DatabasePath = built.DatabasePath;
    });
builder.Services.AddSingleton<ArchivingService>();

builder.Services.AddOptions<UploadOptions>()
    .Configure(options => 
    {
        var built = programOptions.BuildUploadOptions();
        options.DatabasePath = built.DatabasePath;
        options.Output = built.Output;
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

var host = builder.Build();
host.Run();
