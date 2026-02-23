#pragma once

namespace csbind23
{
class BindingsGenerator;
}

namespace example::domain
{

int add(int left, int right);
double scale(double value, double factor);

class Counter
{
public:
    virtual ~Counter() = default;
    explicit Counter(int start);

    virtual int increment(int delta);
    virtual int read() const;
    int increment_through_native(int delta);
    int read_through_native() const;

private:
    int value_;
};

class FancyCounter : public Counter
{
public:
    explicit FancyCounter(int start)
        : Counter(start)
    {
    }

    int increment(int delta) override;
};

Counter* make_polymorphic_counter(bool derived);

} // namespace example::domain

namespace example
{

void register_bindings(csbind23::BindingsGenerator& generator);

} // namespace example
