using AzureBackupTool;

namespace AzureBackupToolTests;

public class ProfileInvocationScheduleTests
{
    // TODO: Review the naming of tests in this class.
    public class GivenAProfileInvocationScheduleWithASingleInvocation
    {
        public class WhenTheTimeIsBeforeTheInvocation
        {
            [Fact]
            public void ThenReturnNothing()
            {
                // Arrange

                ReadOnlyBackupProfileInvocation invocation = new("test-profile", DateTimeOffset.UtcNow, new(), CancellationToken.None);
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
                ReadOnlyBackupProfileInvocation invocation = new("test-profile", DateTimeOffset.UtcNow, new(), CancellationToken.None);
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
                ReadOnlyBackupProfileInvocation invocation = new("test-profile", DateTimeOffset.UtcNow, new(), CancellationToken.None);
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
                ReadOnlyBackupProfileInvocation invocation = new("test-profile", DateTimeOffset.UtcNow, new(), CancellationToken.None);
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
                ReadOnlyBackupProfileInvocation invocation = new("test-profile", DateTimeOffset.UtcNow, new(), CancellationToken.None);
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
                ReadOnlyBackupProfileInvocation invocation = new("profile-one", DateTimeOffset.UtcNow, new(), CancellationToken.None);
                ProfileInvocationSchedule invocationManager = new();
                invocationManager.ScheduleInvocation(invocation);

                // Act
                ReadOnlyBackupProfileInvocation invocation2 = new("profile-two", invocation.InvokeAt, new(), CancellationToken.None);
                invocationManager.ScheduleInvocation(invocation2);
                var invocations = invocationManager.GetPendingInvocations(invocation.InvokeAt.AddHours(1));

                // Assert
                Assert.Equal(2, invocations.Count);
            }
        }

        public class WhenTheInvocationIsCancelled
        {
            [Fact]
            public void ThenNoInvocationIsReturned()
            {
                // Arrange
                using CancellationTokenSource cancellationTokenSource = new();
                ReadOnlyBackupProfileInvocation invocation = new("profile-one", DateTimeOffset.UtcNow, new(), cancellationTokenSource.Token);
                ProfileInvocationSchedule invocationManager = new();
                invocationManager.ScheduleInvocation(invocation);

                // Act
                cancellationTokenSource.Cancel();
                var invocations = invocationManager.GetPendingInvocations(invocation.InvokeAt.AddHours(1));

                // Assert
                Assert.Empty(invocations);
            }
        }
    }
}