namespace AzureBackupTool.Models;

public class Snapshot
{
    public Snapshot() { }

    public Snapshot(string name)
    {
        Name = name;
    }

    public string Name { get; set; } = string.Empty;

    public Status Status { get; set; } = Status.Registered;

    public string? ArchiveName { get; set; }
}

public enum Status
{
    Registered = 0
}