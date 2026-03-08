using System.Runtime.InteropServices;
using CsBind23.Generated;
using CsBind23.Tests.E2E.InstanceCache;
using Xunit;

namespace CsBind23.Tests.E2E;

public sealed class NativeGcHandleInstanceCache<T> : IInstanceCache<T>
    where T : class
{
    public void Register(System.IntPtr handle, T instance)
    {
        if (handle == System.IntPtr.Zero)
        {
            return;
        }

        var existing = InstanceCacheNative.instance_cache_NativeHandleBackedCounter_get_managed_handle(handle);
        if (existing != System.IntPtr.Zero)
        {
            var previous = GCHandle.FromIntPtr(existing);
            if (previous.IsAllocated)
            {
                previous.Free();
            }
        }

        var gcHandle = GCHandle.Alloc(instance, GCHandleType.Weak);
        InstanceCacheNative.instance_cache_NativeHandleBackedCounter_set_managed_handle(
            handle,
            GCHandle.ToIntPtr(gcHandle));
    }

    public void Unregister(System.IntPtr handle)
    {
        if (handle == System.IntPtr.Zero)
        {
            return;
        }

        var value = InstanceCacheNative.instance_cache_NativeHandleBackedCounter_get_managed_handle(handle);
        if (value == System.IntPtr.Zero)
        {
            return;
        }

        var gcHandle = GCHandle.FromIntPtr(value);
        if (gcHandle.IsAllocated)
        {
            gcHandle.Free();
        }
        InstanceCacheNative.instance_cache_NativeHandleBackedCounter_set_managed_handle(handle, System.IntPtr.Zero);
    }

    public bool TryGet(System.IntPtr handle, out T instance)
    {
        instance = null!;
        if (handle == System.IntPtr.Zero)
        {
            return false;
        }

        var value = InstanceCacheNative.instance_cache_NativeHandleBackedCounter_get_managed_handle(handle);
        if (value == System.IntPtr.Zero)
        {
            return false;
        }

        var gcHandle = GCHandle.FromIntPtr(value);
        if (!gcHandle.IsAllocated)
        {
            return false;
        }

        if (gcHandle.Target is T typed)
        {
            instance = typed;
            return true;
        }

        return false;
    }
}

public sealed class ManagedNativeHandleBackedCounter : NativeHandleBackedCounter
{
    public ManagedNativeHandleBackedCounter(int value)
        : base(value)
    {
    }

    public System.IntPtr RawHandle => _cPtr.Handle;

    public override int add(int arg0)
    {
        return base.add(arg0) + 1000;
    }
}

public class InstanceCacheTests
{
    [Fact]
    public void CustomInstanceCache_StoresGCHandleInNativeObject_AndVirtualDispatchUsesIt()
    {
        using var counter = new ManagedNativeHandleBackedCounter(10);

        Assert.NotEqual(
            System.IntPtr.Zero,
            InstanceCacheNative.instance_cache_NativeHandleBackedCounter_get_managed_handle(counter.RawHandle));
        Assert.Equal(1013, counter.add_through_native(3));
    }
}
