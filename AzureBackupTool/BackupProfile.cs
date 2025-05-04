namespace AzureBackupTool;

public class BackupProfile
{
    public string Name { get; set; } = string.Empty;

    public string OutputDirectory { get; set; } = string.Empty;

    public string SearchPath { get; set; } = string.Empty;
}