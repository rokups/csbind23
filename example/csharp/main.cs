using CsBind23.Generated;

Console.WriteLine($"add(2, 3) = {playgroundApi.add(2, 3)}");
Console.WriteLine($"scale(2.0, 4.5) = {playgroundApi.scale(2.0, 4.5)}");

using var counter = new Counter(10);
Console.WriteLine($"counter.increment(5) = {counter.increment(5)}");
Console.WriteLine($"counter.read() = {counter.read()}");
