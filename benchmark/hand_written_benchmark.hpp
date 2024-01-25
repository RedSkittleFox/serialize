#pragma once
#include <fox/serialize.hpp>
#include "benchmark_common.hpp"

struct hand_written_benchmark_traits
{
	template<class T>
		requires std::is_trivial_v<T>
	static void serialize(std::pmr::vector<std::byte>& out, const T& obj)
	{
		const std::size_t offset = std::size(out);
		out.resize(offset + sizeof(T));
		*reinterpret_cast<T*>(std::data(out) + offset) = obj;
	}

	template<class T>
		requires std::is_trivial_v<T>
	static void deserialize(std::span<const std::byte> in, std::size_t& offset, T& obj)
	{
		obj = *reinterpret_cast<const T*>(std::data(in) + offset);
		offset += sizeof(T);
	}

	template<class T>
		requires std::is_trivial_v<T>
	static void serialize(std::pmr::vector<std::byte>& out, const std::vector<T>& obj)
	{
		const std::size_t offset = std::size(out);
		out.resize(offset + std::size(obj) * sizeof(T) + sizeof(std::size_t));
		*reinterpret_cast<std::size_t*>(std::data(out) + offset) = std::size(obj);
		std::memcpy(std::data(out) + offset + sizeof(std::size_t), std::data(obj), std::size(obj) * sizeof(T));
	}

	template<class T>
		requires std::is_trivial_v<T>
	static void deserialize(std::span<const std::byte> in, std::size_t& offset, std::vector<T>& obj)
	{
		obj.resize(*reinterpret_cast<const std::size_t*>(std::data(in) + offset));
		offset += sizeof(std::size_t);
		
		std::memcpy(std::data(obj), std::data(in) + offset, std::size(obj) * sizeof(T));
		offset += std::size(obj) * sizeof(T);
	}

	template<class T>
		requires std::is_trivial_v<T>
	static void serialize(std::pmr::vector<std::byte>& out, const std::basic_string<T>& obj)
	{
		const std::size_t offset = std::size(out);
		out.resize(offset + std::size(obj) * sizeof(T) + sizeof(std::size_t));
		*reinterpret_cast<std::size_t*>(std::data(out) + offset) = std::size(obj);
		std::memcpy(std::data(out) + offset + sizeof(std::size_t), std::data(obj), std::size(obj) * sizeof(T));
	}

	template<class T>
		requires std::is_trivial_v<T>
	static void deserialize(std::span<const std::byte> in, std::size_t& offset, std::basic_string<T>& obj)
	{
		obj.resize(*reinterpret_cast<const std::size_t*>(std::data(in) + offset));
		offset += sizeof(std::size_t);

		std::memcpy(std::data(obj), std::data(in) + offset, std::size(obj) * sizeof(T));
		offset += std::size(obj) * sizeof(T);
	}

	template<class T>
		requires !std::is_trivial_v<T>
	static void serialize(std::pmr::vector<std::byte>& out, const std::vector<T>& obj)
	{
		const std::size_t offset = std::size(out);
		out.resize(offset + sizeof(std::size_t));
		*reinterpret_cast<std::size_t*>(std::data(out) + offset) = std::size(obj);

		for (auto&& e : obj)
			serialize(out, e);
	}

	template<class T>
		requires !std::is_trivial_v<T>
	static void deserialize(std::span<const std::byte> in, std::size_t& offset, std::vector<T>& obj)
	{
		obj.resize(*reinterpret_cast<const std::size_t*>(std::data(in) + offset));
		offset += sizeof(std::size_t);

		for (auto&& e : obj)
			deserialize(in, offset, e);
	}

	static void serialize(std::pmr::vector<std::byte>& out, const fox::serialize::benchmark::engine& obj)
	{
		serialize(out, obj.brand);
		serialize(out, obj.volume);
		serialize(out, obj.max_rpm);

	}

	static void deserialize(std::span<const std::byte> in, std::size_t& offset, fox::serialize::benchmark::engine& obj)
	{
		deserialize(in, offset, obj.brand);
		deserialize(in, offset, obj.volume);
		deserialize(in, offset, obj.max_rpm);
	}

	static void serialize(std::pmr::vector<std::byte>& out, const fox::serialize::benchmark::test_struct& obj)
	{
		serialize(out, obj.v0);
		serialize(out, obj.v1);
		serialize(out, obj.v2);
		serialize(out, obj.v3);
		serialize(out, obj.v4);
		serialize(out, obj.v5);
		serialize(out, obj.v6);
		serialize(out, obj.v7);
		serialize(out, obj.v8);
	}

	static void deserialize(std::span<const std::byte> in, std::size_t& offset, fox::serialize::benchmark::test_struct& obj)
	{
		deserialize(in, offset, obj.v0);
		deserialize(in, offset, obj.v1);
		deserialize(in, offset, obj.v2);
		deserialize(in, offset, obj.v3);
		deserialize(in, offset, obj.v4);
		deserialize(in, offset, obj.v5);
		deserialize(in, offset, obj.v6);
		deserialize(in, offset, obj.v7);
		deserialize(in, offset, obj.v8);
	}

	template<class U>
	[[nodiscard]] std::vector<std::byte> serialize(const U& data)
	{
		std::pmr::vector<std::byte> out;
		serialize(out, data);
		return std::vector<std::byte>(std::from_range, out);
	}

	template<class U>
	[[nodiscard]] U deserialize(std::span<const std::byte> data)
	{
		std::size_t offset = {};
		U u;
		deserialize(data, offset, u);
		return u;
	}
};