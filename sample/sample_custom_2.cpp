#include <fox/serialize.hpp>

#include <iostream>
#include <format>
#include <string>

class custom_type_2
{
	friend void sample_custom_2();

	std::string v0_;
	int v1_;

public:
	custom_type_2() = default;

	custom_type_2(const std::string& v0, int v1)
		: v0_(v0), v1_(v1) {}

	static void serialize(fox::serialize::bit_writer& writer, const custom_type_2& obj) 
	{
		writer | obj.v0_ | obj.v1_;
	}

	static void deserialize(fox::serialize::bit_reader& reader, custom_type_2& obj)
	{
		reader | obj.v0_ | obj.v1_;
	}
};

void sample_custom_2()
{
	fox::serialize::bit_writer writer;

	custom_type_2 a = custom_type_2("Fox", 1);
	writer | a;

	fox::serialize::bit_reader reader(std::from_range, writer.data());

	custom_type_2 b;
	reader | b;

	std::cout << std::format("=== Custom Variant 2 Sample ===\nA = [{} ; {}]\nB = [{} ; {}]\n",
		a.v0_, a.v1_, b.v0_, b.v1_);
}