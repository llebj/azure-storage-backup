namespace AzureBackupTool;

public class ProfileInvocationSchedule
{
    private readonly Dictionary<string, HashSet<DateTimeOffset>> _dictionary = [];

    private readonly PriorityQueue<ReadOnlyBackupProfileInvocation, DateTimeOffset> _queue = new();

    public void ScheduleInvocation(ReadOnlyBackupProfileInvocation invocation)
    {
        if (_dictionary.TryGetValue(invocation.ProfileId, out var invocationTimes) 
            && invocationTimes.Contains(invocation.InvokeAt))
        {
            return;
        }

        if (!_dictionary.TryGetValue(invocation.ProfileId, out HashSet<DateTimeOffset>? value))
        {
            value = [];
            _dictionary.Add(invocation.ProfileId, value);
        }
        value.Add(invocation.InvokeAt);

        _queue.Enqueue(invocation, invocation.InvokeAt);
    }

    public List<ReadOnlyBackupProfileInvocation> GetPendingInvocations(DateTimeOffset time)
    {
        List<ReadOnlyBackupProfileInvocation> result = [];
        while (_queue.TryPeek(out ReadOnlyBackupProfileInvocation invocation, out _) && invocation.InvokeAt <= time)
        {
            var profile = _queue.Dequeue();
            if (profile.CancellationToken.IsCancellationRequested)
            {
                continue;
            }

            result.Add(profile);
            _dictionary[invocation.ProfileId].Remove(invocation.InvokeAt);

            if (_dictionary[invocation.ProfileId].Count == 0)
            {
                _dictionary.Remove(invocation.ProfileId);
            }
        }
        return result;
    }
}
