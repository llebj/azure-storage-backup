using AzureBackupTool;
using Cronos;
using System.Collections.Immutable;

namespace AzureBackupToolTests;

public class ReadOnlyBackupProfileEqualityTests
{
    [Fact]
    public void GivenTwoIdenticalBackupProfiles_WhenComparingForEquality_ThenTheyAreEqual()
    {
        // Arrange
        var cronExpression = CronExpression.Parse("0 0 * * *");
        var searchDefinition = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );
        
        var profile1 = new ReadOnlyBackupProfile("profile-123", cronExpression, searchDefinition);
        var profile2 = new ReadOnlyBackupProfile("profile-123", cronExpression, searchDefinition);

        // Act & Assert
        Assert.True(profile1.Equals(profile2));
        Assert.True(profile1 == profile2);
        Assert.False(profile1 != profile2);
        Assert.Equal(profile1.GetHashCode(), profile2.GetHashCode());
    }

    [Fact]
    public void GivenTwoBackupProfilesWithDifferentProfileIds_WhenComparingForEquality_ThenTheyAreNotEqual()
    {
        // Arrange
        var cronExpression = CronExpression.Parse("0 0 * * *");
        var searchDefinition = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );
        
        var profile1 = new ReadOnlyBackupProfile("profile-123", cronExpression, searchDefinition);
        var profile2 = new ReadOnlyBackupProfile("profile-456", cronExpression, searchDefinition);

        // Act & Assert
        Assert.False(profile1.Equals(profile2));
        Assert.False(profile1 == profile2);
        Assert.True(profile1 != profile2);
    }

    [Fact]
    public void GivenTwoBackupProfilesWithDifferentCronExpressions_WhenComparingForEquality_ThenTheyAreNotEqual()
    {
        // Arrange
        var cronExpression1 = CronExpression.Parse("0 0 * * *");
        var cronExpression2 = CronExpression.Parse("0 12 * * *");
        var searchDefinition = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );
        
        var profile1 = new ReadOnlyBackupProfile("profile-123", cronExpression1, searchDefinition);
        var profile2 = new ReadOnlyBackupProfile("profile-123", cronExpression2, searchDefinition);

        // Act & Assert
        Assert.False(profile1.Equals(profile2));
        Assert.False(profile1 == profile2);
        Assert.True(profile1 != profile2);
    }

    [Fact]
    public void GivenTwoBackupProfilesWithDifferentSearchDefinitions_WhenComparingForEquality_ThenTheyAreNotEqual()
    {
        // Arrange
        var cronExpression = CronExpression.Parse("0 0 * * *");
        var searchDefinition1 = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );
        var searchDefinition2 = new ReadOnlySearchDefinition(
            "/different/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );
        
        var profile1 = new ReadOnlyBackupProfile("profile-123", cronExpression, searchDefinition1);
        var profile2 = new ReadOnlyBackupProfile("profile-123", cronExpression, searchDefinition2);

        // Act & Assert
        Assert.False(profile1.Equals(profile2));
        Assert.False(profile1 == profile2);
        Assert.True(profile1 != profile2);
    }
}

public class ReadOnlySearchDefinitionEqualityTests
{
    [Fact]
    public void GivenTwoIdenticalSearchDefinitions_WhenComparingForEquality_ThenTheyAreEqual()
    {
        // Arrange
        var searchDefinition1 = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );
        var searchDefinition2 = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );

        // Act & Assert
        Assert.True(searchDefinition1.Equals(searchDefinition2));
        Assert.True(searchDefinition1 == searchDefinition2);
        Assert.False(searchDefinition1 != searchDefinition2);
        Assert.Equal(searchDefinition1.GetHashCode(), searchDefinition2.GetHashCode());
    }

    [Fact]
    public void GivenTwoSearchDefinitionsWithDifferentDirectories_WhenComparingForEquality_ThenTheyAreNotEqual()
    {
        // Arrange
        var searchDefinition1 = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );
        var searchDefinition2 = new ReadOnlySearchDefinition(
            "/different/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );

        // Act & Assert
        Assert.False(searchDefinition1.Equals(searchDefinition2));
        Assert.False(searchDefinition1 == searchDefinition2);
        Assert.True(searchDefinition1 != searchDefinition2);
    }

    [Fact]
    public void GivenTwoSearchDefinitionsWithDifferentIncludePatterns_WhenComparingForEquality_ThenTheyAreNotEqual()
    {
        // Arrange
        var searchDefinition1 = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );
        var searchDefinition2 = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.pdf"],
            ["temp/*", "*.log"]
        );

        // Act & Assert
        Assert.False(searchDefinition1.Equals(searchDefinition2));
        Assert.False(searchDefinition1 == searchDefinition2);
        Assert.True(searchDefinition1 != searchDefinition2);
    }

    [Fact]
    public void GivenTwoSearchDefinitionsWithDifferentExcludePatterns_WhenComparingForEquality_ThenTheyAreNotEqual()
    {
        // Arrange
        var searchDefinition1 = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );
        var searchDefinition2 = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.bak"]
        );

        // Act & Assert
        Assert.False(searchDefinition1.Equals(searchDefinition2));
        Assert.False(searchDefinition1 == searchDefinition2);
        Assert.True(searchDefinition1 != searchDefinition2);
    }

    [Fact]
    public void GivenTwoSearchDefinitionsWithSameIncludePatternsInDifferentOrder_WhenComparingForEquality_ThenTheyAreNotEqual()
    {
        // Given
        var searchDefinition1 = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );
        var searchDefinition2 = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.doc", "*.txt"],
            ["temp/*", "*.log"]
        );

        // Act & Assert
        Assert.False(searchDefinition1.Equals(searchDefinition2));
        Assert.False(searchDefinition1 == searchDefinition2);
        Assert.True(searchDefinition1 != searchDefinition2);
    }

    [Fact]
    public void GivenTwoSearchDefinitionsWithSameExcludePatternsInDifferentOrder_WhenComparingForEquality_ThenTheyAreNotEqual()
    {
        // Given
        var searchDefinition1 = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["temp/*", "*.log"]
        );
        var searchDefinition2 = new ReadOnlySearchDefinition(
            "/backup/path",
            ["*.txt", "*.doc"],
            ["*.log", "temp/*"]
        );

        // Act & Assert
        Assert.False(searchDefinition1.Equals(searchDefinition2));
        Assert.False(searchDefinition1 == searchDefinition2);
        Assert.True(searchDefinition1 != searchDefinition2);
    }
}