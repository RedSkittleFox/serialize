#include "benchmark_common.hpp"
#include "fox_serialize_benchmark.hpp"

#include <benchmark/benchmark.h>

#include <span>
#include <cassert>

struct benchmark_traits
{
	template<class U>
	[[nodiscard]] std::vector<std::byte> serialize(const U& data);

	template<class U>
	[[nodiscard]] U deserialize(std::span<const std::byte> data);
};

template<class T>
struct test_data
{
	static T value;
};

template<class T>
T test_data<T>::value = fox::serialize::benchmark::make_random<T>{}();

template<class BenchmarkTraits, class U>
void serialize_benchmark(benchmark::State& state)
{
	auto data = test_data<U>::value;

	for(auto _ : state)
	{
		BenchmarkTraits traits;
		auto out = traits.serialize(data);
		benchmark::DoNotOptimize(out);
	}
}

template<class BenchmarkTraits, class U>
void deserialize_benchmark(benchmark::State& state)
{
	auto data = test_data<U>::value;

	BenchmarkTraits traits;
	auto out = traits.serialize(data);

#ifndef NDEBUG
	auto data_des = traits.template deserialize<U>(out);
	assert(data_des == data);
#endif

	for (auto _ : state)
	{
		U v;
		v = traits.template deserialize<U>(out);
		benchmark::DoNotOptimize(v);
	}
}

#define REGISTER_BENCHMARK_FOR(NAME)\
	BENCHMARK(serialize_benchmark<NAME, std::vector<int>>); \
	BENCHMARK(serialize_benchmark<NAME, std::vector<std::string>>); \
	BENCHMARK(serialize_benchmark<NAME, std::vector<fox::serialize::benchmark::test_struct>>); \
	BENCHMARK(deserialize_benchmark<NAME, std::vector<int>>); \
	BENCHMARK(deserialize_benchmark<NAME, std::vector<std::string>>); \
	BENCHMARK(deserialize_benchmark<NAME, std::vector<fox::serialize::benchmark::test_struct>>); 

REGISTER_BENCHMARK_FOR(fox_serialize_benchmark_traits)

BENCHMARK_MAIN();