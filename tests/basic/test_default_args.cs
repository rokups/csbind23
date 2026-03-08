using CsBind23.Tests.E2E.DefaultArgs;
using Xunit;

namespace CsBind23.Tests.E2E;

public class DefaultArgsTests
{
    [Fact]
    public void Sum3_AllArguments_UsesProvidedValues()
    {
        Assert.Equal(6, DefaultArgsApi.sum3(1, 2, 3));
    }

    [Fact]
    public void Sum3_NullTail_UsesCppDefaults()
    {
        Assert.Equal(13, DefaultArgsApi.sum3(1, null, null));
    }

    [Fact]
    public void Sum3_LastNull_UsesLastCppDefault()
    {
        Assert.Equal(10, DefaultArgsApi.sum3(1, 2, null));
    }

    [Fact]
    public void Weighted_NullTail_UsesCppDefaults()
    {
        Assert.Equal(9, DefaultArgsApi.weighted(4, null, null));
    }

    [Fact]
    public void Weighted_SecondProvided_LastNull_UsesSingleCppDefault()
    {
        Assert.Equal(13, DefaultArgsApi.weighted(4, 3, null));
    }

    [Fact]
    public void Affine_NullTail_UsesCppDefaults()
    {
        Assert.Equal(3.25, DefaultArgsApi.affine(2.0, null, null), 10);
    }

    [Fact]
    public void Affine_SecondProvided_LastNull_UsesSingleCppDefault()
    {
        Assert.Equal(5.25, DefaultArgsApi.affine(2.0, 2.5, null), 10);
    }

    [Fact]
    public void FourDefaults_AllNullTail_UsesAllCppDefaults()
    {
        Assert.Equal(61, DefaultArgsApi.four_defaults(1, null, null, null));
    }

    [Fact]
    public void FourDefaults_MixedProvidedSuffix_UsesExpectedDefaults()
    {
        Assert.Equal(57, DefaultArgsApi.four_defaults(1, 6, null, null));
        Assert.Equal(38, DefaultArgsApi.four_defaults(1, 6, 1, null));
    }

    [Fact]
    public void NonTrailingNull_ThrowsArgumentException()
    {
        Assert.Throws<System.ArgumentException>(() => DefaultArgsApi.sum3(1, null, 3));
    }
}
