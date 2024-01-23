#pragma once
#include <fox/serialize.hpp>

struct fox_serialize_benchmark_traits
{
	template<class U>
	[[nodiscard]] std::vector<std::byte> serialize(const U& data)
	{
		fox::serialize::bit_writer writer;
		writer | data;
		return std::vector<std::byte>(std::from_range, writer.data());
	}

	template<class U>
	[[nodiscard]] U deserialize(std::span<const std::byte> data)
	{
		fox::serialize::bit_reader reader(std::from_range, data);
		U out;
		reader | out;
		return out;
	}

	
};