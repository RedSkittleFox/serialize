#include <fox/serialize.hpp>

#include <iostream>
#include <format>
#include <string>

class custom_type_3
{
	friend void sample_custom_3();

	std::string v0_;
	int v1_;

public:
	custom_type_3() = default;

	custom_type_3(fox::serialize::from_bit_reader_t, fox::serialize::bit_reader& reader)
	{
		reader | v0_ | v1_;
	}

	custom_type_3(const std::string& v0, int v1)
		: v0_(v0), v1_(v1) {}

	static void serialize(fox::serialize::bit_writer& writer, const custom_type_3& obj)
	{
		writer | obj.v0_ | obj.v1_;
	}

};

void sample_custom_3()
{
	fox::serialize::bit_writer writer;

	custom_type_3 a = custom_type_3("Fox", 1);
	writer | a;

	fox::serialize::bit_reader reader(std::from_range, writer.data());

	custom_type_3 b;
	reader | b;

	std::cout << std::format("=== Custom Variant 3 Sample ===\nA = [{} ; {}]\nB = [{} ; {}]\n",
		a.v0_, a.v1_, b.v0_, b.v1_);
}