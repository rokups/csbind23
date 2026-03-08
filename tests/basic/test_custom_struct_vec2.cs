using System;
using System.Reflection;
using System.Runtime.InteropServices;
using CsBind23.Tests.E2E.CustomStructVec2;
using Xunit;

namespace CsBind23.Tests.E2E;

[StructLayout(LayoutKind.Sequential, Pack = 4)]
public struct SequentialVec2 : IEquatable<SequentialVec2>
{
    public float X;
    public float Y;

    public SequentialVec2(float x, float y)
    {
        X = x;
        Y = y;
    }

    public bool Equals(SequentialVec2 other)
    {
        return X.Equals(other.X) && Y.Equals(other.Y);
    }

    public override bool Equals(object? obj)
    {
        return obj is SequentialVec2 other && Equals(other);
    }

    public override int GetHashCode()
    {
        return HashCode.Combine(X, Y);
    }

    public override string ToString()
    {
        return $"({X}, {Y})";
    }

    public static bool operator ==(SequentialVec2 left, SequentialVec2 right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(SequentialVec2 left, SequentialVec2 right)
    {
        return !left.Equals(right);
    }
}

public class CustomStructVec2Tests
{
    [Fact]
    public void SequentialVec2_HasExpectedBlittableLayout()
    {
        Assert.Equal(0, Marshal.OffsetOf<SequentialVec2>(nameof(SequentialVec2.X)).ToInt32());
        Assert.Equal(4, Marshal.OffsetOf<SequentialVec2>(nameof(SequentialVec2.Y)).ToInt32());
        Assert.Equal(8, Marshal.SizeOf<SequentialVec2>());
    }

    [Fact]
    public void ComplexVec2_UsesPodCAbiStruct_ForCustomSequentialType()
    {
        Func<float, float, SequentialVec2> makeVec2 = CustomStructVec2Api.make_vec2;
        Func<SequentialVec2, SequentialVec2, SequentialVec2> addVec2 = CustomStructVec2Api.add_vec2;
        Func<SequentialVec2, float, SequentialVec2> scaleVec2 = CustomStructVec2Api.scale_vec2;

        SequentialVec2 left = makeVec2(1.25f, 2.5f);
        SequentialVec2 right = new SequentialVec2(3.0f, 4.0f);

        Assert.Equal(new SequentialVec2(1.25f, 2.5f), left);
        Assert.Equal(3.75f, CustomStructVec2Api.component_sum(left));
        Assert.Equal(13.75f, CustomStructVec2Api.dot_vec2(left, right));

        SequentialVec2 combined = addVec2(left, right);
        Assert.Equal(new SequentialVec2(4.25f, 6.5f), combined);

        SequentialVec2 scaled = scaleVec2(combined, 0.5f);
        Assert.Equal(new SequentialVec2(2.125f, 3.25f), scaled);

        SequentialVec2 podCombined = CustomStructVec2Api.add_pod_vec2(left, right);
        Assert.Equal(new SequentialVec2(4.25f, 6.5f), podCombined);
    }

    [Fact]
    public void GeneratedNativeSignature_UsesPointerLikeInputs_AndValueReturns()
    {
        Type nativeType = typeof(CustomStructVec2Api).Assembly.GetType(
            $"{typeof(CustomStructVec2Api).Namespace}.CustomStructVec2Native",
            throwOnError: true)!;

        MethodInfo makeVec2 = nativeType.GetMethod("custom_struct_vec2_make_vec2", BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public)!;
        Assert.Equal(typeof(SequentialVec2), makeVec2.ReturnType);
        Assert.All(makeVec2.GetParameters(), parameter => Assert.False(parameter.ParameterType.IsByRef));

        MethodInfo componentSum = nativeType.GetMethod("custom_struct_vec2_component_sum", BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public)!;
        ParameterInfo componentArg = Assert.Single(componentSum.GetParameters());
        Assert.True(componentArg.ParameterType.IsByRef);
        Assert.Equal(typeof(SequentialVec2).MakeByRefType(), componentArg.ParameterType);
        Assert.Equal(typeof(float), componentSum.ReturnType);

        MethodInfo addVec2 = nativeType.GetMethod("custom_struct_vec2_add_vec2", BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public)!;
        Assert.Equal(typeof(SequentialVec2), addVec2.ReturnType);
        Assert.All(addVec2.GetParameters(), parameter =>
        {
            Assert.True(parameter.ParameterType.IsByRef);
            Assert.Equal(typeof(SequentialVec2).MakeByRefType(), parameter.ParameterType);
        });

        MethodInfo publicAddVec2 = typeof(CustomStructVec2Api).GetMethod(nameof(CustomStructVec2Api.add_vec2), BindingFlags.Static | BindingFlags.Public)!;
        Assert.Equal(typeof(SequentialVec2), publicAddVec2.ReturnType);
        Assert.All(publicAddVec2.GetParameters(), parameter => Assert.False(parameter.ParameterType.IsByRef));
    }
}
