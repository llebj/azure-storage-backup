namespace AzureBackupTool.Options;

public class ProgramOptions
{
    public const string Key = "Program";

    public string DatabasePath { get; set; } = string.Empty;

    public string TargetDirectoryPath { get; set; } = string.Empty;
}