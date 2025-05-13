using AzureBackupTool;

namespace AzureBackupToolTests;

public class InvocationManagerTests
{
    public class GivenAnInvocationManagerWithASingleInvocation
    {
        public class WhenTheTimeIsBeforeTheInvocation
        {
            [Fact]
            public void ThenReturnNothing()
            {
                // Arrange

                Invocation invocation = new("test-profile", DateTimeOffset.UtcNow);
                InvocationManager invocationManager = new();
                invocationManager.AddInvocation(invocation);

                // Act
                var invocations = invocationManager.GetPendingInvocations(invocation.InvokeAt.AddHours(-1));

                // Assert
                Assert.Empty(invocations);
            }
        }

        public class WhenTheTimeIsAfterTheInvocation
        {
            [Fact]
            public void ThenReturnTheInvocation()
            {
                // Arrange
                Invocation invocation = new("test-profile", DateTimeOffset.UtcNow);
                InvocationManager invocationManager = new();
                invocationManager.AddInvocation(invocation);

                // Act
                var invocations = invocationManager.GetPendingInvocations(invocation.InvokeAt.AddHours(1));

                // Assert
                Assert.Single(invocations);
            }

            [Fact]
            public void ThenTheManagerDoesNotServeTheSameInvocationTwice()
            {
                // Arrange
                Invocation invocation = new("test-profile", DateTimeOffset.UtcNow);
                InvocationManager invocationManager = new();
                invocationManager.AddInvocation(invocation);

                // Act
                var time = invocation.InvokeAt.AddHours(1);
                _ = invocationManager.GetPendingInvocations(time);
                var invocations = invocationManager.GetPendingInvocations(time);

                // Assert
                Assert.Empty(invocations);
            }

            [Fact]
            public void ThenASecondIdenticalInstanceCanBeAddedAndServedAfterTheFirstInstance()
            {
                Invocation invocation = new("test-profile", DateTimeOffset.UtcNow);
                InvocationManager invocationManager = new();
                invocationManager.AddInvocation(invocation);
                _ = invocationManager.GetPendingInvocations(invocation.InvokeAt.AddHours(1));

                // Act
                invocationManager.AddInvocation(invocation);
                var invocations = invocationManager.GetPendingInvocations(invocation.InvokeAt.AddHours(1));

                // Assert
                Assert.Single(invocations);
            }
        }

        public class WhenADuplicateInvocationIsAdded
        {
            [Fact]
            public void ThenOnlyASingleInvocationInstanceIsReturned()
            {
                // Arrange
                Invocation invocation = new("test-profile", DateTimeOffset.UtcNow);
                InvocationManager invocationManager = new();
                invocationManager.AddInvocation(invocation);
                invocationManager.AddInvocation(invocation);

                // Act
                var invocations = invocationManager.GetPendingInvocations(invocation.InvokeAt.AddHours(1));

                // Assert
                Assert.Single(invocations);
            }
        }
    
        public class WhenAnInvocationIsAddedForADifferentProfile
        {
            [Fact]
            public void ThenBothInvocationsCanBeRetrieved()
            {
                // Arrange
                Invocation invocation = new("profile-one", DateTimeOffset.UtcNow);
                InvocationManager invocationManager = new();
                invocationManager.AddInvocation(invocation);

                // Act
                Invocation invocation2 = new("profile-two", invocation.InvokeAt);
                invocationManager.AddInvocation(invocation2);
                var invocations = invocationManager.GetPendingInvocations(invocation.InvokeAt.AddHours(1));

                // Assert
                Assert.Equal(2, invocations.Count);
            }
        }
    }
}