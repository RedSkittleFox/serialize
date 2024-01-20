#include <fox/serialize.hpp>

#include <iostream>
#include <format>
#include <string>

void sample_trivial_types()
{
	fox::serialize::bit_writer writer;

	int a_v0 = 1;
	float a_v1 = 2;

	struct trivial_struct
	{
		int a;
		char b;
	};

	trivial_struct a_v2 { 3, 'f' };

	writer | a_v0 | a_v1 | a_v2;

	fox::serialize::bit_reader reader(std::from_range, writer.data());

	int b_v0;
	float b_v1;
	trivial_struct b_v2;
	reader | b_v0 | b_v1 | b_v2;

	std::cout << std::format("=== Trivial Types Sample ===\nA = [ {}, {}, {}, {} ]\nB = [ {}, {}, {}, {} ]\n",
		a_v0, a_v1, a_v2.a, a_v2.b, b_v0, b_v1, b_v2.a, b_v2.b);
}