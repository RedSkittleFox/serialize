#include <fox/serialize.hpp>

#include <iostream>
#include <format>
#include <string>

class custom_type_4
{
	friend void sample_custom_4();

	std::string v0_;
	int v1_;

public:
	custom_type_4() = default;

	custom_type_4(const std::string& v0, int v1)
		: v0_(v0), v1_(v1) {}

	// This can be a hand written trait like in a sample samples/sample_custom_0.cpp
	using serialize_trait = fox::serialize::serialize_from_members<custom_type_4, &custom_type_4::v0_, &custom_type_4::v1_>;
};

void sample_custom_4()
{
	fox::serialize::bit_writer writer;

	custom_type_4 a = custom_type_4("Fox", 1);
	writer | a;

	fox::serialize::bit_reader reader(std::from_range, writer.data());

	custom_type_4 b;
	reader | b;

	std::cout << std::format("=== Custom Variant 4 Sample ===\nA = [{} ; {}]\nB = [{} ; {}]\n",
		a.v0_, a.v1_, b.v0_, b.v1_);
}