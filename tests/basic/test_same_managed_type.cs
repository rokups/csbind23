using System;
using System.Numerics;
using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E;

public class SameManagedTypeTests
{
    [Fact]
    public void DifferentNativeTypes_Can_Share_SystemNumericsVector2()
    {
        Vector2 fromMethods = SameManagedTypeApi.make_vector2(1.0f, 2.0f);
        Vector2 fromFields = SameManagedTypeApi.make_vec2(3.0f, 4.0f);

        Assert.Equal(new Vector2(1.0f, 2.0f), fromMethods);
        Assert.Equal(new Vector2(3.0f, 4.0f), fromFields);
        Assert.Equal(17.0f, SameManagedTypeApi.dot_vector2(fromMethods, new Vector2(5.0f, 6.0f)));
        Assert.Equal(53.0f, SameManagedTypeApi.dot_vec2(fromFields, new Vector2(7.0f, 8.0f)));

        Vector2 sum = SameManagedTypeApi.add_mixed(fromMethods, fromFields);
        Assert.Equal(new Vector2(4.0f, 6.0f), sum);

        Vector2 scaled = SameManagedTypeApi.scale_vec2(sum, 0.5f);
        Assert.Equal(new Vector2(2.0f, 3.0f), scaled);
    }
}
