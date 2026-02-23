using CsBind23.Generated;

public class CounterOverride : Counter
{
	public CounterOverride(int start)
		: base(start)
	{
	}

	public override int increment(int arg0)
	{
		return base.increment(arg0) + 100;
	}

	public override int read()
	{
		return base.read() + 200;
	}
}


public static class MainClass
{
    public static void Main()
    {
        Console.WriteLine($"add(2, 3) = {playgroundApi.add(2, 3)}");
        Console.WriteLine($"scale(2.0, 4.5) = {playgroundApi.scale(2.0, 4.5)}");

        using var counter = new Counter(10);
        Console.WriteLine($"counter.increment(5) = {counter.increment(5)}");
        Console.WriteLine($"counter.read() = {counter.read()}");

        using var derived = new CounterOverride(10);
        Console.WriteLine($"derived.increment(5) = {derived.increment(5)}");
        Console.WriteLine($"derived.read() = {derived.read()}");
        Console.WriteLine($"derived.increment_through_native(5) = {derived.increment_through_native(5)}");
        Console.WriteLine($"derived.read_through_native() = {derived.read_through_native()}");

        var polyBase = playgroundApi.make_polymorphic_counter(false);
        var polyDerived = playgroundApi.make_polymorphic_counter(true);
        Console.WriteLine($"make_polymorphic_counter(false) -> {polyBase?.GetType().Name}");
        Console.WriteLine($"make_polymorphic_counter(true) -> {polyDerived?.GetType().Name}");
    }
}
