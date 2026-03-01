using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E;

public class BasicTests
{
	[Fact]
	public void Bool_RoundTrips()
	{
		const bool value = true;
		Assert.Equal(value, BasicApi.get_and_return_bool(value));
	}

	[Fact]
	public void Int8_RoundTrips()
	{
		const sbyte value = 100;
		Assert.Equal(value, BasicApi.get_and_return_int8(value));
	}

	[Fact]
	public void UInt8_RoundTrips()
	{
		const byte value = 200;
		Assert.Equal(value, BasicApi.get_and_return_uint8(value));
	}

	[Fact]
	public void Int16_RoundTrips()
	{
		const short value = 1234;
		Assert.Equal(value, BasicApi.get_and_return_int16(value));
	}

	[Fact]
	public void Int32_RoundTrips()
	{
		const int value = 123456;
		Assert.Equal(value, BasicApi.get_and_return_int32(value));
	}

	[Fact]
	public void Int64_RoundTrips()
	{
		nint value = 123456789;
		Assert.Equal(value, BasicApi.get_and_return_int64(value));
	}

	[Fact]
	public void UInt16_RoundTrips()
	{
		const ushort value = 60000;
		Assert.Equal(value, BasicApi.get_and_return_uint16(value));
	}

	[Fact]
	public void UInt32_RoundTrips()
	{
		const uint value = 4_000_000_000;
		Assert.Equal(value, BasicApi.get_and_return_uint32(value));
	}

	[Fact]
	public void UInt64_RoundTrips()
	{
		nuint value = 987654321;
		Assert.Equal(value, BasicApi.get_and_return_uint64(value));
	}

	[Fact]
	public void Float_RoundTrips()
	{
		const float value = 123.5f;
		Assert.Equal(value, BasicApi.get_and_return_float(value));
	}

	[Fact]
	public void Double_RoundTrips()
	{
		const double value = 123.5;
		Assert.Equal(value, BasicApi.get_and_return_double(value));
	}

	[Fact]
	public void Char16_RoundTrips()
	{
		const char value = 'A';
		Assert.Equal(value, BasicApi.get_and_return_char16(value));
	}

	[Fact]
	public void Int32_RefParameter_MutatesManagedValue()
	{
		var value = 10;
		BasicApi.add_assign_ref_int32(ref value, 5);
		Assert.Equal(15, value);
	}

	[Fact]
	public void Double_RefParameter_MutatesManagedValue()
	{
		var value = 10.5;
		BasicApi.add_assign_ref_double(ref value, 0.25);
		Assert.Equal(10.75, value);
	}

	[Fact]
	public void Int32_PointerParameter_MutatesManagedValue()
	{
		var value = 7;
		BasicApi.add_assign_ptr_int32(ref value, 8);
		Assert.Equal(15, value);
	}

	[Fact]
	public void Double_PointerParameter_MutatesManagedValue()
	{
		var value = 1.5;
		BasicApi.add_assign_ptr_double(ref value, 2.0);
		Assert.Equal(3.5, value);
	}

	[Fact]
	public void Bool_RefAssign_Works()
	{
		var boolValue = false;
		BasicApi.assign_ref_bool(ref boolValue, true);
		Assert.True(boolValue);
	}

	[Fact]
	public void Int8_RefAssign_Works()
	{
		sbyte int8Value = 1;
		BasicApi.assign_ref_int8(ref int8Value, -11);
		Assert.Equal((sbyte)-11, int8Value);
	}

	[Fact]
	public void UInt8_RefAssign_Works()
	{
		byte uint8Value = 1;
		BasicApi.assign_ref_uint8(ref uint8Value, 201);
		Assert.Equal((byte)201, uint8Value);
	}

	[Fact]
	public void Int16_RefAssign_Works()
	{
		short int16Value = 1;
		BasicApi.assign_ref_int16(ref int16Value, -1234);
		Assert.Equal((short)-1234, int16Value);
	}

	[Fact]
	public void Int32_RefAssign_Works()
	{
		int int32Value = 1;
		BasicApi.assign_ref_int32(ref int32Value, -123456);
		Assert.Equal(-123456, int32Value);
	}

	[Fact]
	public void Int64_RefAssign_Works()
	{
		nint int64Value = 1;
		BasicApi.assign_ref_int64(ref int64Value, -123456789);
		Assert.Equal((nint)(-123456789), int64Value);
	}

	[Fact]
	public void UInt16_RefAssign_Works()
	{
		ushort uint16Value = 1;
		BasicApi.assign_ref_uint16(ref uint16Value, 54321);
		Assert.Equal((ushort)54321, uint16Value);
	}

	[Fact]
	public void UInt32_RefAssign_Works()
	{
		uint uint32Value = 1;
		BasicApi.assign_ref_uint32(ref uint32Value, 3000000001);
		Assert.Equal(3000000001u, uint32Value);
	}

	[Fact]
	public void UInt64_RefAssign_Works()
	{
		nuint uint64Value = 1;
		BasicApi.assign_ref_uint64(ref uint64Value, 987654321);
		Assert.Equal((nuint)987654321, uint64Value);
	}

	[Fact]
	public void Float_RefAssign_Works()
	{
		float floatValue = 1.0f;
		BasicApi.assign_ref_float(ref floatValue, 12.25f);
		Assert.Equal(12.25f, floatValue);
	}

	[Fact]
	public void Double_RefAssign_Works()
	{
		double doubleValue = 1.0;
		BasicApi.assign_ref_double(ref doubleValue, 34.5);
		Assert.Equal(34.5, doubleValue);
	}

	[Fact]
	public void Char16_RefAssign_Works()
	{
		char char16Value = 'A';
		BasicApi.assign_ref_char16(ref char16Value, 'Z');
		Assert.Equal('Z', char16Value);
	}

	[Fact]
	public void Bool_PtrAssign_Works()
	{
		var boolValue = false;
		BasicApi.assign_ptr_bool(ref boolValue, true);
		Assert.True(boolValue);
	}

	[Fact]
	public void Int8_PtrAssign_Works()
	{
		sbyte int8Value = 1;
		BasicApi.assign_ptr_int8(ref int8Value, -12);
		Assert.Equal((sbyte)-12, int8Value);
	}

	[Fact]
	public void UInt8_PtrAssign_Works()
	{
		byte uint8Value = 1;
		BasicApi.assign_ptr_uint8(ref uint8Value, 202);
		Assert.Equal((byte)202, uint8Value);
	}

	[Fact]
	public void Int16_PtrAssign_Works()
	{
		short int16Value = 1;
		BasicApi.assign_ptr_int16(ref int16Value, -2345);
		Assert.Equal((short)-2345, int16Value);
	}

	[Fact]
	public void Int32_PtrAssign_Works()
	{
		int int32Value = 1;
		BasicApi.assign_ptr_int32(ref int32Value, -234567);
		Assert.Equal(-234567, int32Value);
	}

	[Fact]
	public void Int64_PtrAssign_Works()
	{
		nint int64Value = 1;
		BasicApi.assign_ptr_int64(ref int64Value, -987654321);
		Assert.Equal((nint)(-987654321), int64Value);
	}

	[Fact]
	public void UInt16_PtrAssign_Works()
	{
		ushort uint16Value = 1;
		BasicApi.assign_ptr_uint16(ref uint16Value, 12345);
		Assert.Equal((ushort)12345, uint16Value);
	}

	[Fact]
	public void UInt32_PtrAssign_Works()
	{
		uint uint32Value = 1;
		BasicApi.assign_ptr_uint32(ref uint32Value, 4000000001);
		Assert.Equal(4000000001u, uint32Value);
	}

	[Fact]
	public void UInt64_PtrAssign_Works()
	{
		nuint uint64Value = 1;
		BasicApi.assign_ptr_uint64(ref uint64Value, 123456789);
		Assert.Equal((nuint)123456789, uint64Value);
	}

	[Fact]
	public void Float_PtrAssign_Works()
	{
		float floatValue = 1.0f;
		BasicApi.assign_ptr_float(ref floatValue, 22.75f);
		Assert.Equal(22.75f, floatValue);
	}

	[Fact]
	public void Double_PtrAssign_Works()
	{
		double doubleValue = 1.0;
		BasicApi.assign_ptr_double(ref doubleValue, 44.25);
		Assert.Equal(44.25, doubleValue);
	}

	[Fact]
	public void Char16_PtrAssign_Works()
	{
		char char16Value = 'B';
		BasicApi.assign_ptr_char16(ref char16Value, 'Y');
		Assert.Equal('Y', char16Value);
	}

	[Fact]
	public void ArgOption_Output_UsesOutAndWritesValues()
	{
		BasicApi.write_pair(out var left, out var right);
		Assert.Equal(11, left);
		Assert.Equal(22, right);
	}

	[Fact]
	public void ArgOption_Name_PropagatesToManagedSignature()
	{
		var method = typeof(BasicApi).GetMethod("write_pair");
		Assert.NotNull(method);
		var parameters = method!.GetParameters();
		Assert.Equal(2, parameters.Length);
		Assert.Equal("left", parameters[0].Name);
		Assert.Equal("right", parameters[1].Name);
		Assert.True(parameters[0].IsOut);
		Assert.True(parameters[1].IsOut);
	}
}
