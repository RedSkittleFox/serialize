#include <fox/serialize.hpp>

#include <iostream>
#include <format>
#include <string>

template<>
struct fox::serialize::serialize_traits<std::unique_ptr<int>>
{
	static void serialize(fox::serialize::bit_writer& writer, const std::unique_ptr<int>& ptr)
	{
		writer | (static_cast<bool>(ptr));
		if (ptr)
			writer | (*ptr);
	}

	static void deserialize(fox::serialize::bit_reader& reader, std::unique_ptr<int>& ptr)
	{
		bool b = fox::serialize::deserialize<bool>(reader);
		if(b)
		{
			ptr = std::make_unique<int>(fox::serialize::deserialize<int>(reader));
		}
		else
		{
			ptr.reset();
		}
	}
};

void sample_custom_0()
{
	fox::serialize::bit_writer writer;

	auto a = std::make_unique<int>(2);
	writer | a;

	fox::serialize::bit_reader reader(std::from_range, writer.data());

	std::unique_ptr<int> b;
	reader | b;

	std::cout << std::format("=== Custom Variant 0 Sample ===\nA = {}\nB = {}\n",
		*a, *b);

}