#include <fox/serialize.hpp>

#include <iostream>
#include <format>
#include <string>

class custom_type_1
{
	friend void sample_custom_1();

	std::string v0_;
	int v1_;

public:
	custom_type_1() = default;

	custom_type_1(const std::string& v0, int v1)
		: v0_(v0), v1_(v1) {}

	void serialize(fox::serialize::bit_writer& writer) const
	{
		writer | v0_ | v1_;
	}

	void deserialize(fox::serialize::bit_reader& reader)
	{
		reader | v0_ | v1_;
	}
};

void sample_custom_1()
{
	fox::serialize::bit_writer writer;

	custom_type_1 a = custom_type_1("Fox", 1);
	writer | a;

	fox::serialize::bit_reader reader(std::from_range, writer.data());

	custom_type_1 b;
	reader | b;

	std::cout << std::format("=== Custom Variant 1 Sample ===\nA = [{} ; {}]\nB = [{} ; {}]\n",
		a.v0_, a.v1_, b.v0_, b.v1_);
}