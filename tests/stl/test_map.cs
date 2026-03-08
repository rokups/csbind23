using System.Collections.Generic;
using System.Linq;
using CsBind23.Generated;
using CsBind23.Tests.E2E.Map;
using Std;
using Xunit;

namespace CsBind23.Tests.E2E;

public class MapTests
{
    [Fact]
    public void Map_Implements_Dictionary_Interfaces()
    {
        using var value = new Map<long, int>();
        Assert.IsAssignableFrom<IDictionary<long, int>>(value);
        Assert.IsAssignableFrom<IReadOnlyDictionary<long, int>>(value);
    }

    [Fact]
    public void Map_Basic_Add_Get_Set_Work()
    {
        using var value = new Map<long, int>();
        value.Add(1, 10);
        value.Add(2, 20);
        value[3] = 30;

        Assert.Equal(3, value.Count);
        Assert.True(value.ContainsKey(2));
        Assert.Equal(20, value[2]);
        Assert.True(value.TryGetValue(3, out var third));
        Assert.Equal(30, third);

        value[2] = 200;
        Assert.Equal(200, value[2]);
    }

    [Fact]
    public void Map_Keys_Values_And_CopyTo_Work()
    {
        using var value = new Map<long, int>();
        value.Add(1, 10);
        value.Add(2, 200);
        value.Add(3, 30);

        var keys = value.Keys.OrderBy(x => x).ToArray();
        var values = value.Values.OrderBy(x => x).ToArray();
        Assert.Equal(new long[] { 1, 2, 3 }, keys);
        Assert.Equal(new[] { 10, 30, 200 }, values);

        var destination = new KeyValuePair<long, int>[5];
        value.CopyTo(destination, 1);
        var copied = destination.Skip(1).Take(3).OrderBy(x => x.Key).ToArray();
        Assert.Equal(new[]
        {
            new KeyValuePair<long, int>(1, 10),
            new KeyValuePair<long, int>(2, 200),
            new KeyValuePair<long, int>(3, 30),
        }, copied);
    }

    [Fact]
    public void Map_Remove_And_Missing_Key_Behavior_Work()
    {
        using var value = new Map<long, int>();
        value.Add(1, 10);
        value.Add(2, 20);

        Assert.True(value.Remove(1));
        Assert.False(value.Remove(99));
        Assert.Throws<KeyNotFoundException>(() => _ = value[99]);
    }

    [Fact]
    public void Map_String_Key_And_Class_Value_Work()
    {
        using var counts = new Map<Std.String, int>();
        counts.Add("one", 1);
        counts["two"] = 2;

        Assert.Equal(2, counts["two"]);
        Assert.Contains(counts, pair => pair.Key == "one" && pair.Value == 1);

        using var items = new Map<long, MapItem>(ItemOwnership.Owned);
        using var source = new MapItem(7);
        items.Add(5, source);

        var item = items[5];
        var typed = Assert.IsType<MapItem>(item);
        Assert.Equal(7, typed.get());

        typed.set(15);
        Assert.Equal(15, items[5].get());

        typed.Dispose();
        Assert.Equal(15, items[5].get());
    }

    [Fact]
    public void UnorderedMap_Operations_Work_Without_Order_Assumptions()
    {
        using var value = new UnorderedMap<long, int>();
        value.Add(1, 10);
        value.Add(2, 20);
        value[3] = 30;

        Assert.Equal(3, value.Count);
        Assert.True(value.TryGetValue(2, out var second));
        Assert.Equal(20, second);

        var entries = value.ToDictionary(pair => pair.Key, pair => pair.Value);
        Assert.Equal(new Dictionary<long, int>
        {
            [1] = 10,
            [2] = 20,
            [3] = 30,
        }, entries);

        value.Clear();
        Assert.Empty(value);
    }

    [Fact]
    public void UnorderedMap_Pointer_Value_Can_Take_Ownership_And_Destroy_Manually()
    {
        using var value = new UnorderedMap<long, MapItem>(ItemOwnership.Borrowed);
        using var source = new MapItem(123);
        source.ReleaseOwnership();
        value.Add(7, source);

        var borrowed = value[7];
        borrowed.Dispose();
        Assert.Equal(123, source.get());

        borrowed = value[7];
        borrowed.TakeOwnership();

        Assert.True(value.Remove(7));
        borrowed.DestroyNative();
        borrowed.Dispose();
    }

    [Fact]
    public void Map_And_UnorderedMap_Throw_For_Unmapped_Generic_Tuple()
    {
        var mapEx = Assert.Throws<NotSupportedException>(() =>
        {
            using var _ = new Map<int, int>();
        });
        Assert.Contains("generic mapping", mapEx.Message, StringComparison.OrdinalIgnoreCase);

        var unorderedEx = Assert.Throws<NotSupportedException>(() =>
        {
            using var _ = new UnorderedMap<int, int>();
        });
        Assert.Contains("generic mapping", unorderedEx.Message, StringComparison.OrdinalIgnoreCase);
    }
}