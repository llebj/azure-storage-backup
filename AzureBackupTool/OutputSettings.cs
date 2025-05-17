using System.ComponentModel.DataAnnotations;

namespace AzureBackupTool;

public class OutputSettings
{
    public const string Key = "Output";

    public string Path { get; set; } = string.Empty;

    [AllowedValues("azure", "fs")]
    public string Type { get; set; } = "azure";
}