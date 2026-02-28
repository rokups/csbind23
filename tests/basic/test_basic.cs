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
}
