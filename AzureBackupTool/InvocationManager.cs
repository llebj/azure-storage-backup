namespace AzureBackupTool;

public class InvocationManager
{
    private readonly PriorityQueue<Invocation, DateTimeOffset> _queue = new();

    public void AddInvocation(Invocation invocation)
    {
        _queue.Enqueue(invocation, invocation.InvokeAt);
    }

    public List<Invocation> GetPendingInvocations(DateTimeOffset time)
    {
        List<Invocation> result = [];
        while (_queue.Peek().InvokeAt <= time)
        {
            result.Add(_queue.Dequeue());
        }
        return result;
    }
}

public readonly record struct Invocation(string ProfileId, DateTimeOffset InvokeAt);