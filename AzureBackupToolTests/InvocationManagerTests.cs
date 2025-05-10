using AzureBackupTool;

namespace AzureBackupToolTests;

public class InvocationManagerTests
{
    public class GivenAnInvocationManagerWithASingleInvocation
    {
        private readonly Invocation _invocation = new("test-profile", DateTimeOffset.UtcNow);

        [Fact]
        public void WhenTheTimeIsBeforeTheInvocation_ThenReturnNothing()
        {
            // Arrange
            InvocationManager invocationManager = new();
            invocationManager.AddInvocation(_invocation);

            // Act
            var invocations = invocationManager.GetPendingInvocations(_invocation.InvokeAt.AddHours(1));

            // Assert
            Assert.Empty(invocations);
        }
    }
}