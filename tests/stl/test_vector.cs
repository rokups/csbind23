using System.Collections;
using System.Collections.Generic;
using CsBind23.Generated;
using Std;
using CsBind23.Tests.E2E.Vector;
using Xunit;

namespace CsBind23.Tests.E2E;

public class VectorTests
{
    [Fact]
    public void Vector_Implements_List_Interfaces()
    {
        using var value = new Vector<int>();
        Assert.IsAssignableFrom<IList<int>>(value);
        Assert.IsAssignableFrom<ICollection<int>>(value);
        Assert.IsAssignableFrom<IEnumerable<int>>(value);
        Assert.IsAssignableFrom<IReadOnlyList<int>>(value);
    }

    [Fact]
    public void Vector_Int_List_Operations_Work()
    {
        using var value = new Vector<int>();
        value.Add(1);
        value.Add(2);
        value.Add(4);

        value.Insert(2, 3);
        value.Add(5);

        Assert.Equal(5, value.Count);
        Assert.Equal(new[] { 1, 2, 3, 4, 5 }, value);

        value[1] = 20;
        Assert.Equal(20, value[1]);
        Assert.Contains(4, value);
        Assert.Equal(3, value.IndexOf(4));

        Assert.True(value.Remove(3));
        value.RemoveAt(0);
        Assert.Equal(new[] { 20, 4, 5 }, value);
    }

    [Fact]
    public void Vector_Double_CopyTo_And_Clear_Work()
    {
        using var value = new Vector<double>();
        value.Add(1.5);
        value.Add(2.5);
        value.Add(3.5);
        var destination = new double[6];

        value.CopyTo(destination, 2);

        Assert.Equal(new[] { 0.0, 0.0, 1.5, 2.5, 3.5, 0.0 }, destination);
        value.Clear();
        Assert.Empty(value);
    }

    [Fact]
    public void Vector_Throws_On_Invalid_Index()
    {
        using var value = new Vector<int>();
        value.Add(7);
        value.Add(8);

        Assert.Throws<ArgumentOutOfRangeException>(() => _ = value[-1]);
        Assert.Throws<ArgumentOutOfRangeException>(() => _ = value[2]);
        Assert.Throws<ArgumentOutOfRangeException>(() => value.Insert(3, 9));
        Assert.Throws<ArgumentOutOfRangeException>(() => value.RemoveAt(2));
    }

    [Fact]
    public void Vector_Throws_For_Unmapped_Generic_Type()
    {
        var ex = Assert.Throws<NotSupportedException>(() =>
        {
            using var _ = new Vector<float>();
        });

        Assert.Contains("generic mapping", ex.Message, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public void Vector_VectorItem_Uses_NonOwning_Wrapper()
    {
        using var list = new Vector<VectorItem>(ItemOwnership.Owned);
        using var source = new VectorItem(10);
        list.Add(source);

        var item = list[0];
        var typed = Assert.IsType<VectorItem>(item);
        Assert.Equal(10, typed.get());

        typed.set(42);
        Assert.Equal(42, list[0].get());

        typed.Dispose();
        Assert.Equal(42, list[0].get());
    }

    [Fact]
    public void Vector_VectorItemPointer_Can_Take_Ownership_And_Destroy_Manually()
    {
        using var list = new Vector<VectorItem>(ItemOwnership.Borrowed);
        using var source = new VectorItem(123);
        source.SetOwnership(ItemOwnership.Borrowed);
        list.Add(source);

        var borrowed = list[0];
        borrowed.Dispose();
        Assert.Equal(123, source.get());

        borrowed = list[0];
        borrowed.SetOwnership(ItemOwnership.Owned);

        list.RemoveAt(0);
        borrowed.Dispose();
    }
}
