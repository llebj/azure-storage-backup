using System.Collections.Immutable;
using AzureBackupTool.Extensions;
using Microsoft.Extensions.Options;

namespace AzureBackupTool;

public class BackupProfileService : IDisposable
{
    private bool _disposed = false;
    private readonly IOptionsMonitor<List<BackupProfile>> _optionsMonitor;
    private ImmutableDictionary<string, BackupProfileController> _readonlyBackupProfiles;
    private readonly Lock _readonlyBackupProfilesLock = new();

    public BackupProfileService(
        IOptionsMonitor<List<BackupProfile>> optionsMonitor)
    {
        _optionsMonitor = optionsMonitor;
        _readonlyBackupProfiles = _optionsMonitor.CurrentValue
            .ToImmutableDictionary(p => p.Name, p => new BackupProfileController(p.GetReadOnlyBackupProfile(), new CancellationTokenSource()));
        _optionsMonitor.OnChange(OnBackupProfileOptionsChanged);
    }

    public void Dispose()
    {
        if (_disposed) return;
        
        lock (_readonlyBackupProfilesLock)
        {
            foreach (var controller in _readonlyBackupProfiles.Values)
            {
                controller.CancellationTokenSource.Cancel();
                controller.CancellationTokenSource.Dispose();
            }
            _readonlyBackupProfiles = ImmutableDictionary<string, BackupProfileController>.Empty;
        }
        
        _disposed = true;
    }

    public ImmutableArray<ReadOnlyBackupProfileInvocation> GetInvocations(DateTimeOffset currentTime)
    {
        lock (_readonlyBackupProfilesLock)
        {
            return [.. _readonlyBackupProfiles.Select(item => item.Value.State.GetNextInvocation(currentTime, item.Value.CancellationTokenSource.Token))];
        }
    }

    private void OnBackupProfileOptionsChanged(List<BackupProfile> options, string? thing)
    {
        lock (_readonlyBackupProfilesLock)
        {
            var newReadOnlyProfiles = options.ToImmutableDictionary(p => p.Name, p => p.GetReadOnlyBackupProfile());
            var profilesForCancellation = new List<string>();
            var oldProfileKeys = _readonlyBackupProfiles.Keys.ToHashSet();
            var currentProfileKeys = newReadOnlyProfiles.Keys.ToHashSet();

            // Add all of the profiles that have been remove completely.
            profilesForCancellation.AddRange(oldProfileKeys.Except(currentProfileKeys));

            var finalProfiles = new Dictionary<string, BackupProfileController>();
            var newProfileKeys = currentProfileKeys.Except(oldProfileKeys);
            var changedProfiles = new List<string>();
            // Profiles that exist in both but may have changed
            var potentiallyChangedProfiles = oldProfileKeys.Intersect(currentProfileKeys);

            // Register all new profiles.
            foreach (var key in newProfileKeys)
            {
                finalProfiles.Add(key, new BackupProfileController(newReadOnlyProfiles[key], new CancellationTokenSource()));
            }

            // Check which existing profiles have actually changed
            foreach (var key in potentiallyChangedProfiles)
            {
                var oldProfile = _readonlyBackupProfiles[key].State;
                var newProfile = newReadOnlyProfiles[key];

                if (oldProfile.Equals(newProfile))
                {
                    finalProfiles[key] = _readonlyBackupProfiles[key];
                }
                else
                {
                    changedProfiles.Add(key);
                }
            }

            // Add changed profiles (with new CancellationTokenSource)
            foreach (var key in changedProfiles)
            {
                finalProfiles.Add(key, new BackupProfileController(newReadOnlyProfiles[key], new CancellationTokenSource()));
            }
            profilesForCancellation.AddRange(changedProfiles);

            // Cancel and dispose old cancellation token sources
            foreach (var key in profilesForCancellation)
            {
                var (_, cts) = _readonlyBackupProfiles[key];
                cts.Cancel();
                cts.Dispose();
            }

            _readonlyBackupProfiles = finalProfiles.ToImmutableDictionary();
        }
    }

    private readonly record struct BackupProfileController(ReadOnlyBackupProfile State, CancellationTokenSource CancellationTokenSource);
}
