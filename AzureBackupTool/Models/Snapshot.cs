using System.ComponentModel.DataAnnotations.Schema;

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

    [Column("archive_name")]
    public string? ArchiveName { get; set; }
}

public enum Status
{
    Registered,
    ArchiveBuilding,
    ArchiveBuilt,
    ArchiveUploading,
    ArchiveUploaded,
    ArchiveRemoved
}