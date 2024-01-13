#include <gtest/gtest.h>
#include <fox/serialize.hpp>
#include <random>
#include <ranges>
#include <concepts>

namespace fox::serialize
{
	template<class T>
	struct test_trait
	{
		[[nodiscard]] static T construct();
	};

	template<class Type>
	class serialize_test : public testing::Test {};

	TYPED_TEST_SUITE_P(serialize_test);

	TYPED_TEST_P(serialize_test, serialize_deserialize)
	{
		static_assert(std::ranges::borrowed_range<std::vector<int>> == false);
		using value_type = TypeParam;
		if constexpr(deserializable<value_type>)
		{
			fox::serialize::bit_writer writer;
			value_type a = test_trait<value_type>::construct();
			writer | a;
			fox::serialize::bit_reader reader(writer.data());
			value_type b;
			reader | b;
			if constexpr (std::equality_comparable<value_type> || !std::is_trivial_v<value_type>)
			{
				EXPECT_EQ(a, b);
			}
			else
			{
				EXPECT_TRUE(std::memcmp(std::addressof(a), std::addressof(b), sizeof(a)) == 0);
			}
		}
		else if constexpr (std::ranges::range<value_type>)
		{
			fox::serialize::bit_writer writer;
			value_type a = test_trait<value_type>::construct();
			writer | a;
			fox::serialize::bit_reader reader(writer.data());
			std::vector<std::ranges::range_value_t<value_type>> b;
			reader | b;
			EXPECT_TRUE(std::ranges::equal(a, b));
		}
		else
		{
			static_assert(!deserializable<value_type> && !std::ranges::range<value_type> &&
				"Invalid type for this test.");
		}
	}

	REGISTER_TYPED_TEST_SUITE_P(serialize_test, serialize_deserialize);

	template<class T>
	requires std::is_trivial_v<T>
	struct test_trait<T>
	{
		static constexpr std::array<std::uint8_t, 4> data = {32u, 255u, 22u, 54u};

		[[nodiscard]] static T construct()
		{
			T v;
			std::span<uint8_t> data_span(reinterpret_cast<uint8_t*>(std::addressof(v)), sizeof(v));

			for( auto&& [to, from] : std::views::zip(data_span, std::views::repeat(data) | std::views::join ))
			{
				to = from;
			}

			return v;
		}
	};

	struct test_struct
	{
		int a;
		float b;
		char c;
	};

	template<>
	struct test_trait<std::string_view>
	{
		[[nodiscard]] static std::string_view construct()
		{
			using namespace std::string_view_literals;
			return "Foxes can fly"sv;
		}
	};

	template<>
	struct test_trait<std::span<int>>
	{
		static inline std::vector<int> v{ 1, 2, 3, 4 };

		[[nodiscard]] static std::span<int> construct()
		{
			return std::span<int>(v);
		}
	};

	template<std::ranges::range T>
		requires !std::is_trivial_v<T> && !std::ranges::borrowed_range<T>
	struct test_trait<T>
	{
		[[nodiscard]] static T construct()
		{
			using value_type = std::ranges::range_value_t<T>;

			std::vector<value_type> values;
			for(std::size_t i = 0; i < sizeof(T); ++i)
			{
				values.push_back(test_trait<value_type>::construct());
			}

			return values | std::ranges::to<T>();
		}
	};

	template<class... Args>
	struct test_trait<std::tuple<Args...>>
	{
		[[nodiscard]] static std::tuple<Args...> construct()
		{
			return std::tuple<Args...>(test_trait<Args>::construct()...);
		}
	};

	template<class... Args>
	struct test_trait<std::pair<Args...>>
	{
		[[nodiscard]] static std::pair<Args...> construct()
		{
			return std::pair<Args...>(test_trait<Args>::construct()...);
		}
	};

	using types = ::testing::Types<
		char,
		int,
		unsigned int,
		test_struct,
		std::array<int, 10>,
		std::string_view,
		std::span<int>,
		std::vector<int>,
		std::string,
		std::tuple<int, float>,
		std::tuple<int, float, std::string>,
		std::pair<int, int>,
		std::pair<int, std::string>
	>;

	INSTANTIATE_TYPED_TEST_SUITE_P(fundamental, serialize_test, types);
}