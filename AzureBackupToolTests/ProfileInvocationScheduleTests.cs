using AzureBackupTool;

namespace AzureBackupToolTests;

public class ProfileInvocationScheduleTests
{
    public class GivenAProfileInvocationScheduleWithASingleInvocation
    {
        public class WhenTheTimeIsBeforeTheInvocation
        {
            [Fact]
            public void ThenReturnNothing()
            {
                // Arrange

                ProfileInvocation invocation = new("test-profile", DateTimeOffset.UtcNow, new());
                ProfileInvocationSchedule invocationManager = new();
                invocationManager.ScheduleInvocation(invocation);

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
                ProfileInvocation invocation = new("test-profile", DateTimeOffset.UtcNow, new());
                ProfileInvocationSchedule invocationManager = new();
                invocationManager.ScheduleInvocation(invocation);

                // Act
                var invocations = invocationManager.GetPendingInvocations(invocation.InvokeAt.AddHours(1));

                // Assert
                Assert.Single(invocations);
            }

            [Fact]
            public void ThenTheManagerDoesNotServeTheSameInvocationTwice()
            {
                // Arrange
                ProfileInvocation invocation = new("test-profile", DateTimeOffset.UtcNow, new());
                ProfileInvocationSchedule invocationManager = new();
                invocationManager.ScheduleInvocation(invocation);

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
                ProfileInvocation invocation = new("test-profile", DateTimeOffset.UtcNow, new());
                ProfileInvocationSchedule invocationManager = new();
                invocationManager.ScheduleInvocation(invocation);
                _ = invocationManager.GetPendingInvocations(invocation.InvokeAt.AddHours(1));

                // Act
                invocationManager.ScheduleInvocation(invocation);
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
                ProfileInvocation invocation = new("test-profile", DateTimeOffset.UtcNow, new());
                ProfileInvocationSchedule invocationManager = new();
                invocationManager.ScheduleInvocation(invocation);
                invocationManager.ScheduleInvocation(invocation);

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
                ProfileInvocation invocation = new("profile-one", DateTimeOffset.UtcNow, new());
                ProfileInvocationSchedule invocationManager = new();
                invocationManager.ScheduleInvocation(invocation);

                // Act
                ProfileInvocation invocation2 = new("profile-two", invocation.InvokeAt, new());
                invocationManager.ScheduleInvocation(invocation2);
                var invocations = invocationManager.GetPendingInvocations(invocation.InvokeAt.AddHours(1));

                // Assert
                Assert.Equal(2, invocations.Count);
            }
        }
    }
}