namespace Std;

public interface IStringUtf8View
{
    StringView AsView();
}

[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
public readonly unsafe struct StringView :
    IStringUtf8View,
    global::System.IEquatable<StringView>,
    global::System.IEquatable<IStringUtf8View>,
    global::System.IComparable<StringView>,
    global::System.IComparable<IStringUtf8View>
{
    public readonly System.IntPtr str;
    public readonly nuint length;

    public StringView(System.IntPtr str, nuint length)
    {
        this.str = str;
        this.length = length;
    }

    public bool IsNull => str == System.IntPtr.Zero;

    public int ByteLength => checked((int)length);

    public StringView AsView() => this;

    public ReadOnlySpan<byte> AsBytes()
    {
        return str == System.IntPtr.Zero || length == 0
            ? ReadOnlySpan<byte>.Empty
            : new ReadOnlySpan<byte>(str.ToPointer(), checked((int)length));
    }

    public bool Equals(ReadOnlySpan<byte> other)
    {
        return AsBytes().SequenceEqual(other);
    }

    public bool Equals(Span<byte> other)
    {
        return AsBytes().SequenceEqual(other);
    }

    public bool Equals(StringView other)
    {
        return AsBytes().SequenceEqual(other.AsBytes());
    }

    public bool Equals(IStringUtf8View other)
    {
        return other is not null && Equals(other.AsView());
    }

    public int CompareTo(StringView other)
    {
        return AsBytes().SequenceCompareTo(other.AsBytes());
    }

    public int CompareTo(IStringUtf8View other)
    {
        return other is null ? (IsNull ? 0 : 1) : CompareTo(other.AsView());
    }

    public override bool Equals(object obj)
    {
        return obj is StringView other && Equals(other);
    }

    public override int GetHashCode()
    {
        var hash = new global::System.HashCode();
        hash.AddBytes(AsBytes());
        return hash.ToHashCode();
    }

    public static StringView Borrow(global::Std.String value)
    {
        return value is null ? default : value.AsView();
    }

    public static bool operator ==(StringView left, StringView right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(StringView left, StringView right)
    {
        return !left.Equals(right);
    }

    public static bool operator ==(StringView left, IStringUtf8View right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(StringView left, IStringUtf8View right)
    {
        return !left.Equals(right);
    }

    public static bool operator ==(IStringUtf8View left, StringView right)
    {
        return right.Equals(left);
    }

    public static bool operator !=(IStringUtf8View left, StringView right)
    {
        return !right.Equals(left);
    }

    public static bool operator <(StringView left, StringView right)
    {
        return left.CompareTo(right) < 0;
    }

    public static bool operator <=(StringView left, StringView right)
    {
        return left.CompareTo(right) <= 0;
    }

    public static bool operator >(StringView left, StringView right)
    {
        return left.CompareTo(right) > 0;
    }

    public static bool operator >=(StringView left, StringView right)
    {
        return left.CompareTo(right) >= 0;
    }

    public static bool operator <(StringView left, IStringUtf8View right)
    {
        return left.CompareTo(right) < 0;
    }

    public static bool operator <=(StringView left, IStringUtf8View right)
    {
        return left.CompareTo(right) <= 0;
    }

    public static bool operator >(StringView left, IStringUtf8View right)
    {
        return left.CompareTo(right) > 0;
    }

    public static bool operator >=(StringView left, IStringUtf8View right)
    {
        return left.CompareTo(right) >= 0;
    }

    public static bool operator <(IStringUtf8View left, StringView right)
    {
        return right.CompareTo(left) > 0;
    }

    public static bool operator <=(IStringUtf8View left, StringView right)
    {
        return right.CompareTo(left) >= 0;
    }

    public static bool operator >(IStringUtf8View left, StringView right)
    {
        return right.CompareTo(left) < 0;
    }

    public static bool operator >=(IStringUtf8View left, StringView right)
    {
        return right.CompareTo(left) <= 0;
    }

    public override string ToString()
    {
        return global::CsBind23.Generated.CsBind23Utf8Interop.NativeToString(str, length);
    }
}
