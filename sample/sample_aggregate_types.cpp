#include <fox/serialize.hpp>

#include <iostream>
#include <format>
#include <string>

#ifdef FOX_SERIALIZE_HAS_REFLEXPR

struct aggregate_type
{
	std::string v0;
	int v1;
	float v2;
};

void sample_aggregate_types()
{
	fox::serialize::bit_writer writer;

	aggregate_type a
	{
		.v0 = "Fox Fox",
		.v1 = 1,
		.v2 = 12.34f
	};

	writer | a;

	fox::serialize::bit_reader reader(std::from_range, writer.data());

	aggregate_type b;
	reader | b;

	std::cout << std::format("=== Aggregate Types Sample ===\nA = [ {}, {}, {} ]\nB = [ {}, {}, {} ]\n",
		a.v0, a.v1, a.v2, b.v0, b.v1, b.v2);
}

#endif