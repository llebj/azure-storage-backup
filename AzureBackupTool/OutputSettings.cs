namespace AzureBackupTool;

public class OutputSettings
{
    public const string Key = "Output";

    public string Path { get; set; } = string.Empty;

    public string Type { get; set; } = "azure";
}