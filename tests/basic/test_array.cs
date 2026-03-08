using System;
using System.Runtime.InteropServices;
using CsBind23.Tests.E2E.Basic;
using Xunit;

namespace CsBind23.Tests.E2E;

[StructLayout(LayoutKind.Sequential, Pack = 4)]
public struct BasicArrayRecord : IEquatable<BasicArrayRecord>
{
	public int Left;
	public int Right;

	public BasicArrayRecord(int left, int right)
	{
		Left = left;
		Right = right;
	}

	public bool Equals(BasicArrayRecord other)
	{
		return Left == other.Left && Right == other.Right;
	}

	public override bool Equals(object? obj)
	{
		return obj is BasicArrayRecord other && Equals(other);
	}

	public override int GetHashCode()
	{
		return HashCode.Combine(Left, Right);
	}
}

public partial class BasicTests
{
	[Fact]
	public void CArray_RecordArray_ReadsMultipleObjects()
	{
		var value = new[]
		{
			new BasicArrayRecord(1, 2),
			new BasicArrayRecord(3, 4),
			new BasicArrayRecord(5, 6),
		};

		Assert.Equal(21, BasicApi.sum_record_array(value));
	}

	[Fact]
	public void CArray_RecordArray_CopiesBackObjectMutations()
	{
		var value = new[]
		{
			new BasicArrayRecord(1, 10),
			new BasicArrayRecord(2, 20),
		};

		BasicApi.add_to_record_array(value, 5);

		Assert.Equal(new[]
		{
			new BasicArrayRecord(6, 15),
			new BasicArrayRecord(7, 25),
		}, value);
	}

	[Fact]
	public void CArray_ObjectPointerArray_ReadsMultipleTypes()
	{
		using var first = new BasicArrayItem(4);
		using var second = new BasicArrayItem(7);
		using var third = new BasicArrayItem(9);
		var value = new[] { first, second, third };

		Assert.Equal(20, BasicApi.sum_item_pointer_array(value));
	}

	[Fact]
	public void CArray_ObjectPointerArray_CopiesBackPointerRewrites()
	{
		using var first = new BasicArrayItem(10);
		using var second = new BasicArrayItem(20);
		using var third = new BasicArrayItem(30);
		var value = new[] { first, second, third };

		BasicApi.rotate_item_pointer_array(value);

		Assert.Equal(20, value[0].get());
		Assert.Equal(30, value[1].get());
		Assert.Equal(10, value[2].get());
	}
}