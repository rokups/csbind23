using System.Runtime.InteropServices.Marshalling;
using Xunit;

namespace CsBind23.Tests.E2E.CustomMarshaller;

public sealed class CustomText
{
    public CustomText(string value)
    {
        Value = value ?? string.Empty;
    }

    public string Value { get; }

    public override string ToString() => Value;
}

[CustomMarshaller(typeof(CustomText), MarshalMode.ManagedToUnmanagedIn, typeof(CustomTextMarshaller.ManagedToNative))]
[CustomMarshaller(typeof(CustomText), MarshalMode.ManagedToUnmanagedOut, typeof(CustomTextMarshaller.NativeToManaged))]
public static class CustomTextMarshaller
{
    public static class ManagedToNative
    {
        public static nint ConvertToUnmanaged(CustomText managed)
        {
            return global::CsBind23.Generated.CsBind23Utf8Interop.StringToNative(managed?.Value ?? string.Empty);
        }

        public static void Free(nint unmanaged)
        {
            global::CsBind23.Generated.CsBind23Utf8Interop.Free(unmanaged);
        }
    }

    public static class NativeToManaged
    {
        public static CustomText ConvertToManaged(nint unmanaged)
        {
            return new CustomText(global::CsBind23.Generated.CsBind23Utf8Interop.NativeToString(unmanaged));
        }

        public static void Free(nint unmanaged)
        {
            global::CsBind23.Generated.CsBind23Utf8Interop.Free(unmanaged);
        }
    }
}

public class CustomMarshallerTests
{
    [Fact]
    public void UserProvidedMarshaller_IsUsed_ForParameters_And_Returns()
    {
        var value = new CustomText("hello");
        CustomText echoed = CustomMarshallerApi.echo(value);

        Assert.Equal("hello|native", echoed.Value);
    }

    [Fact]
    public void UserProvidedMarshaller_Can_Unmarshal_ReturnOnly_Value()
    {
        CustomText value = CustomMarshallerApi.make_default();
        Assert.Equal("seed", value.Value);
    }
}
