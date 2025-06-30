using System.Collections.Immutable;
using AzureBackupTool.Extensions;
using Microsoft.Extensions.Options;

namespace AzureBackupTool;

public class BackupProfileService : IDisposable
{
    private bool _disposed = false;
    private ImmutableDictionary<string, ReadOnlyBackupProfile> _readonlyBackupProfiles;

    private readonly IOptionsMonitor<List<BackupProfile>> _optionsMonitor;
    private readonly Lock _readonlyBackupProfilesLock = new();
    private readonly CancellationTokenSourceRegistry _ctsRegistry = new();

    public BackupProfileService(
        IOptionsMonitor<List<BackupProfile>> optionsMonitor)
    {
        _optionsMonitor = optionsMonitor;
        _readonlyBackupProfiles = _optionsMonitor.CurrentValue
            .ToImmutableDictionary(p => p.Name, p => p.GetReadOnlyBackupProfile());
        foreach (var profileId in _readonlyBackupProfiles.Keys)
        {
            _ctsRegistry.Register(profileId);
        }
        _optionsMonitor.OnChange(OnBackupProfileOptionsChanged);
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        lock (_readonlyBackupProfilesLock)
        {
            _ctsRegistry.Dispose();
            _readonlyBackupProfiles = ImmutableDictionary<string, ReadOnlyBackupProfile>.Empty;
        }

        _disposed = true;
    }

    public ImmutableArray<ReadOnlyBackupProfileInvocation> GetInvocations(DateTimeOffset currentTime)
    {
        lock (_readonlyBackupProfilesLock)
        {
            return [.. _readonlyBackupProfiles.Select(item => item.Value.GetNextInvocation(currentTime, _ctsRegistry.GetToken(item.Value.ProfileId)))];
        }
    }

    private void OnBackupProfileOptionsChanged(List<BackupProfile> options, string? thing)
    {
        lock (_readonlyBackupProfilesLock)
        {
            var newReadOnlyProfiles = options.ToImmutableDictionary(p => p.Name, p => p.GetReadOnlyBackupProfile());
            var oldProfileIds = _readonlyBackupProfiles.Keys.ToHashSet();
            var currentProfileIds = newReadOnlyProfiles.Keys.ToHashSet();

            // Cancel all tokens for profiles that no longer exist.
            foreach (var profileId in oldProfileIds.Except(currentProfileIds))
            {
                _ctsRegistry.RequestCancellation(profileId);
            }

            // Check which existing profiles have actually changed and replace their token sources.
            foreach (var profileId in oldProfileIds.Intersect(currentProfileIds))
            {
                var oldProfile = _readonlyBackupProfiles[profileId];
                var newProfile = newReadOnlyProfiles[profileId];
                if (!oldProfile.Equals(newProfile))
                {
                    _ctsRegistry.RegisterOrReplace(profileId);
                }
            }

            // Register all new profiles.
            foreach (var profileId in currentProfileIds.Except(oldProfileIds))
            {
                _ctsRegistry.Register(profileId);
            }

            // Set new readonly profiles.
            _readonlyBackupProfiles = newReadOnlyProfiles;
        }
    }

    private class CancellationTokenSourceRegistry : IDisposable
    {
        private readonly Dictionary<string, CancellationTokenSource> _cancellationTokenSources = [];

        public void Clear()
        {
            foreach (var cts in _cancellationTokenSources.Values)
            {
                cts.Cancel();
                cts.Dispose();
            }
            _cancellationTokenSources.Clear();
        }

        public void Dispose() => Clear(); 

        public CancellationToken GetToken(string profileId)
        {
            if (!_cancellationTokenSources.TryGetValue(profileId, out CancellationTokenSource? cts))
            {
                throw new InvalidOperationException($"The key {profileId} is not registered.");
            }
            return cts.Token;
        }

        public void Register(string profileId)
        {
            if (_cancellationTokenSources.ContainsKey(profileId))
            {
                throw new InvalidOperationException($"The key {profileId} has already been registered.");
            }
            _cancellationTokenSources[profileId] = new CancellationTokenSource();
        }

        public void RegisterOrReplace(string profileId)
        {
            if (_cancellationTokenSources.TryGetValue(profileId, out var existingCts))
            {
                existingCts.Cancel();
                existingCts.Dispose();
            }
            _cancellationTokenSources[profileId] = new CancellationTokenSource();
        }

        public void RequestCancellation(string profileId)
        {
            if (!_cancellationTokenSources.TryGetValue(profileId, out CancellationTokenSource? cts))
            {
                return;
            }
            cts.Cancel();
            cts.Dispose();
            _cancellationTokenSources.Remove(profileId);
        }
    }
}
