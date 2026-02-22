#pragma once

namespace csbind23 {
class BindingsGenerator;
}

namespace example::domain {

int add(int left, int right);
double scale(double value, double factor);

class Counter {
public:
	explicit Counter(int start);

	int increment(int delta);
	int read() const;

private:
	int value_;
};

} // namespace example::domain

namespace example {

void register_bindings(csbind23::BindingsGenerator& generator);

} // namespace example
