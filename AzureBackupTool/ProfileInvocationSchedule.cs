namespace AzureBackupTool;

public class ProfileInvocationSchedule
{
    private readonly Dictionary<string, HashSet<DateTimeOffset>> _dictionary = [];

    private readonly PriorityQueue<ProfileInvocation, DateTimeOffset> _queue = new();

    public void ScheduleInvocation(ProfileInvocation invocation)
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

    public List<ProfileInvocation> GetPendingInvocations(DateTimeOffset time)
    {
        List<ProfileInvocation> result = [];
        while (_queue.TryPeek(out ProfileInvocation invocation, out _) && invocation.InvokeAt <= time)
        {
            result.Add(_queue.Dequeue());
            _dictionary[invocation.ProfileId].Remove(invocation.InvokeAt);

            if (_dictionary[invocation.ProfileId].Count == 0)
            {
                _dictionary.Remove(invocation.ProfileId);
            }
        }
        return result;
    }
}

public readonly record struct ProfileInvocation(string ProfileId, DateTimeOffset InvokeAt, string SearchPath);