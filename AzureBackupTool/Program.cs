using AzureBackupTool;

var builder = Host.CreateApplicationBuilder(args);
var env = builder.Environment;

builder.Configuration
    .AddJsonFile("appsettings.json", optional: true, reloadOnChange: true)
    .AddJsonFile($"appsettings.{env.EnvironmentName}.json", optional: true, reloadOnChange: true);

List<BackupProfile> profiles = [];
builder.Configuration.GetSection("Profiles").Bind(profiles);

using var factory = LoggerFactory.Create(builder => builder.AddConsole());
var logger = factory.CreateLogger("Program");
logger.LogInformation("Loaded {ProfileCount} backup profiles.", profiles.Count);

builder.Services.Configure<List<BackupProfile>>(builder.Configuration.GetSection(key: "Profiles"));
builder.Services.AddHostedService<Worker>();

var host = builder.Build();
host.Run();
