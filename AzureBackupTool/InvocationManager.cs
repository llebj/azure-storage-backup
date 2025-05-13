namespace AzureBackupTool;

public class InvocationManager
{
    private readonly Dictionary<string, HashSet<DateTimeOffset>> _dictionary = [];

    private readonly PriorityQueue<Invocation, DateTimeOffset> _queue = new();

    public void AddInvocation(Invocation invocation)
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

    public List<Invocation> GetPendingInvocations(DateTimeOffset time)
    {
        List<Invocation> result = [];
        while (_queue.TryPeek(out Invocation invocation, out _) && invocation.InvokeAt <= time)
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

public readonly record struct Invocation(string ProfileId, DateTimeOffset InvokeAt);