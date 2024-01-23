#ifndef FOX_SERIALIZE_BENCHMARK_BENCHMARK_COMMON_H_
#define FOX_SERIALIZE_BENCHMARK_BENCHMARK_COMMON_H_
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <valarray>
#include <string>
#include <tuple>
#include <random>

namespace fox::serialize::benchmark
{
	inline std::mt19937_64 gen64;

	template<class T>
	struct make_random
	{
		[[nodiscard]] T operator()()
		{
			static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

			if constexpr (std::is_integral_v<T>)
			{
				if constexpr(sizeof(T) < 2)
				{
					static std::uniform_int_distribution<int> dist(-100, 100);
					return static_cast<T>(dist(gen64));
				}
				else
				{
					static std::uniform_int_distribution<T> dist(-100, 100);
					return dist(gen64);
				}
			}
			else if constexpr (std::is_floating_point_v<T>)
			{
				static std::uniform_real_distribution<T> dist(static_cast<T>(-100.0), static_cast<T>(100.0));
				return dist(gen64);
			}
		}
	};

	enum animals : std::uint8_t
	{
		capybara,
		fox,
		cat
	};

	template<>
	struct make_random<animals>
	{
		[[nodiscard]] animals operator()()
		{
			static std::uniform_int_distribution<int> dist(0, 3);
			int v = dist(gen64);
			return static_cast<animals>(v);
		}
	};
	

	template<class T>
	struct make_random<std::vector<T>>
	{
		[[nodiscard]] std::vector<T> operator()(std::size_t count = std::numeric_limits<std::size_t>::max())
		{
			if(count == std::numeric_limits<std::size_t>::max())
			{
				std::uniform_int_distribution<std::size_t> dist(1, 100);
				count = dist(gen64);
			}

			std::vector<T> out;

			for(std::size_t i = 0; i < count; ++i)
			{
				out.push_back(make_random<T>{}());
			}

			return out;
		}
	};

	template<class Char>
	struct make_random<std::basic_string<Char>>
	{
		[[nodiscard]] std::basic_string<Char> operator()(std::size_t count = std::numeric_limits<std::size_t>::max())
		{
			if (count == std::numeric_limits<std::size_t>::max())
			{
				std::uniform_int_distribution<std::size_t> dist(1, 100);
				count = dist(gen64);
			}

			std::basic_string<Char> out;

			std::uniform_int_distribution<int> dist(32, 126);
			for (std::size_t i = 0; i < count; ++i)
			{
				out.push_back(static_cast<Char>(dist(gen64)));
			}

			return out;
		}
	};

	struct vec4
	{
		float x;
		float y;
		float z;
		float w;
	};

	[[nodiscard]] inline bool operator==(const vec4& lhs, const vec4& rhs)
	{
		const std::valarray<float> v_lhs{ lhs.x, lhs.y, lhs.z, lhs.w };
		const std::valarray<float> v_rhs{ rhs.x, rhs.y, rhs.z, rhs.w };
		return (std::abs(v_lhs - v_rhs) < 0.10f).min();
	}

	template<>
	struct make_random<vec4>
	{
		[[nodiscard]] vec4 operator()()
		{
			return
			{
				make_random<float>{}(),
				make_random<float>{}(),
				make_random<float>{}(),
				make_random<float>{}()
			};
		}
	};

	struct engine
	{
		std::string brand;
		float volume = 0.f;
		int max_rpm = 1;
	};

	[[nodiscard]] inline bool operator==(const engine& lhs, const engine& rhs)
	{
		return std::tie(lhs.brand, lhs.volume, lhs.max_rpm) == std::tie(rhs.brand, rhs.volume, rhs.max_rpm);
	}

	template<>
	struct make_random<engine>
	{
		[[nodiscard]] engine operator()()
		{
			return
			{
				make_random<std::string>{}(),
				make_random<float>{}(),
				make_random<int>{}()
			};
		}
	};
	
	// Doesn't make sense 
	struct test_struct
	{
		vec4 v0;
		std::int16_t v1;
		std::int16_t v2;
		std::string v3;
		animals v4;
		std::vector<engine> v5;
		std::vector<uint8_t> v6;
		engine v7;
		std::vector<vec4> v8;
	};

	template<>
	struct make_random<test_struct>
	{
		[[nodiscard]] test_struct operator()()
		{
			return
			{
				make_random<decltype(test_struct::v0)>{}(),
				make_random<decltype(test_struct::v1)>{}(),
				make_random<decltype(test_struct::v2)>{}(),
				make_random<decltype(test_struct::v3)>{}(),
				make_random<decltype(test_struct::v4)>{}(),
				make_random<decltype(test_struct::v5)>{}(),
				make_random<decltype(test_struct::v6)>{}(),
				make_random<decltype(test_struct::v7)>{}(),
				make_random<decltype(test_struct::v8)>{}()
			};
		}
	};

	[[nodiscard]] inline bool operator==(const test_struct& lhs, const test_struct& rhs)
	{
		return std::tie(lhs.v0, lhs.v1, lhs.v2, lhs.v3, lhs.v4, lhs.v5, lhs.v6, lhs.v7, lhs.v8)
			== std::tie(rhs.v0, rhs.v1, rhs.v2, rhs.v3, rhs.v4, rhs.v5, rhs.v6, rhs.v7, rhs.v8);
	}
}

#endif