using AzureBackupTool;
using Microsoft.Extensions.Options;

namespace AzureBackupToolTests;

public class BackupProfileServiceTests
{
    [Fact]
    public void EmptyInitialConfiguration_ReturnsNoInvocations()
    {
        // Arrange
        var backupProfileService = new BackupProfileService(new TestOptionsMonitor<List<BackupProfile>>([]));

        // Act & Assert
        var invocations = backupProfileService.GetInvocations(DateTime.UtcNow);
        Assert.Empty(invocations);
    }

    [Fact]
    public void GivenEmptyConfiguration_WhenAProfileIsAdded_ThenInvocationsAreReturned()
    {
        // Arrange
        var optionsMonitor = new TestOptionsMonitor<List<BackupProfile>>([]);
        var backupProfileService = new BackupProfileService(optionsMonitor);

        // Act
        var backupProfile = new BackupProfile
        {
            Cron = "* * * * *",
            Name = "profile-123",
            SearchDefinition = new()
        };
        optionsMonitor.SimulateChange([backupProfile]);

        // Assert
        var invocations = backupProfileService.GetInvocations(DateTime.UtcNow);
        Assert.Single(invocations);
        Assert.False(invocations[0].CancellationToken.IsCancellationRequested);
    }

    [Fact]
    public void GivenAnExistingProfile_WhenTheProfileIsRemoved_ThenAllRelatedInvocationsAreCancelled()
    {
        // Arrange
        var optionsMonitor = new TestOptionsMonitor<List<BackupProfile>>(
            [
                new()
                {
                    Cron = "* * * * *",
                    Name = "profile-123",
                    SearchDefinition = new()
                }
            ]);
        var backupProfileService = new BackupProfileService(optionsMonitor);
        var invocations = backupProfileService.GetInvocations(DateTime.UtcNow);

        // Act
        optionsMonitor.SimulateChange([]);

        // Assert
        Assert.True(invocations[0].CancellationToken.IsCancellationRequested);
    }

    [Fact]
    public void GivenAnExistingProfile_WhenTheProfileIsModified_ThenOldInvocationsAreCancelledAndNewOnesAreAvailable()
    { 
        // Arrange
        var optionsMonitor = new TestOptionsMonitor<List<BackupProfile>>(
            [
                new()
                {
                    Cron = "* * * * *",
                    Name = "profile-123",
                    SearchDefinition = new()
                }
            ]);
        var backupProfileService = new BackupProfileService(optionsMonitor);
        var initialInvocations = backupProfileService.GetInvocations(DateTime.UtcNow);

        // Act
        optionsMonitor.SimulateChange(
            [
                new()
                {
                    Cron = "*/5 * * * * ",
                    Name = "profile-123",
                    SearchDefinition = new()
                }
            ]);
        var subsequentInvocations = backupProfileService.GetInvocations(DateTime.UtcNow);

        // Assert
        Assert.Equal(initialInvocations[0].ProfileId, subsequentInvocations[0].ProfileId);
        Assert.True(initialInvocations[0].CancellationToken.IsCancellationRequested);
        Assert.False(subsequentInvocations[0].CancellationToken.IsCancellationRequested);
    }

    [Fact]
    public void GivenAnExistingProfile_WhenAnotherProfileIsAdded_ThenInvocationsForTheExistingProfileAreNotCancelled()
    {
        // Arrange
        var optionsMonitor = new TestOptionsMonitor<List<BackupProfile>>(
            [
                new()
                {
                    Cron = "* * * * *",
                    Name = "profile-123",
                    SearchDefinition = new()
                }
            ]);
        var backupProfileService = new BackupProfileService(optionsMonitor);
        var initialInvocations = backupProfileService.GetInvocations(DateTime.UtcNow);

        // Act
        optionsMonitor.SimulateChange(
            [
                new()
                {
                    Cron = "* * * * *",
                    Name = "profile-123",
                    SearchDefinition = new()
                },
                new()
                {
                    Cron = "*/5 * * * * ",
                    Name = "profile-987",
                    SearchDefinition = new()
                }
            ]);
        var subsequentInvocations = backupProfileService.GetInvocations(DateTime.UtcNow);

        // Assert
        var subsequenInvocation = subsequentInvocations.Single(i => i.ProfileId == "profile-123");
        Assert.Equal(initialInvocations[0].ProfileId, subsequenInvocation.ProfileId);
        Assert.False(initialInvocations[0].CancellationToken.IsCancellationRequested);
        Assert.False(subsequenInvocation.CancellationToken.IsCancellationRequested); 
    }
}

internal class TestOptionsMonitor<T> : IOptionsMonitor<T>
{
    private T _currentValue;
    private readonly List<Action<T, string?>> _changeCallbacks = [];

    public TestOptionsMonitor(T initialValue)
    {
        _currentValue = initialValue;
    }

    public T CurrentValue => _currentValue;

    public T Get(string? name) => _currentValue;

    public IDisposable OnChange(Action<T, string?> listener)
    {
        _changeCallbacks.Add(listener);
        return new CallbackUnsubscriber(() => _changeCallbacks.Remove(listener));
    }

    public void SimulateChange(T newValue, string? name = null)
    {
        _currentValue = newValue;
        foreach (var callback in _changeCallbacks)
        {
            callback(newValue, name);
        }
    }

    private class CallbackUnsubscriber : IDisposable
    {
        private readonly Action _unsubscribe;
        public CallbackUnsubscriber(Action unsubscribe) => _unsubscribe = unsubscribe;
        public void Dispose() => _unsubscribe();
    }
}