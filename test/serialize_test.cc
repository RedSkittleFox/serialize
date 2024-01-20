#include <gtest/gtest.h>
#include <fox/serialize.hpp>

#include <random>
#include <ranges>
#include <concepts>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <array>
#include <string>
#include <string_view>
#include <span>

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
			fox::serialize::bit_reader reader(std::from_range, writer.data());
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
			fox::serialize::bit_reader reader(std::from_range, writer.data());
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
			std::remove_const_t<T> v;
			std::span<uint8_t> data_span(reinterpret_cast<uint8_t*>(std::addressof(v)), sizeof(v));

			for( auto&& [to, from] : std::views::zip(data_span, std::views::repeat(data) | std::views::join ))
			{
				to = from;
			}

			return v;
		}
	};

	struct udt_trivial
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

	template<class T, std::size_t I>
	struct test_trait<std::array<T, I>>
	{
		[[nodiscard]] static std::array<T, I> construct()
		{
			std::array<T, I> out {};
			for (auto& e : out)
				e = test_trait<T>::construct();

			return out;
		}
	};

	template<std::ranges::range T>
		requires ( !std::is_trivial_v<T> && !std::ranges::borrowed_range<T> )
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

			return values | std::ranges::to<std::remove_const_t<T>>();
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

	template<class... Args>
	struct test_trait<std::variant<Args...>>
	{
		[[nodiscard]] static std::variant<Args...> construct()
		{
			using last = std::tuple_element_t<sizeof...(Args) - 1, std::tuple<Args...>>;
			return std::variant<Args...>(std::in_place_type<last>, test_trait<last>::construct());
		}
	};

	template<class T>
	struct test_trait<std::optional<T>>
	{
		[[nodiscard]] static std::optional<T> construct()
		{
			return std::optional<T>(test_trait<T>::construct());
		}
	};

	template<>
	struct test_trait<std::optional<std::nullptr_t>>
	{
		[[nodiscard]] static std::optional<std::nullptr_t> construct()
		{
			return {};
		}
	};

#ifdef FOX_SERIALIZE_HAS_REFLEXPR
	struct udt_aggregate_type
	{
		std::string v0_;
		int v1_;
		float v2_;
		std::vector<int> v3_;
		std::vector<std::string> v4_;

		[[nodiscard]] bool operator==(const udt_aggregate_type& rhs) const
		{
			return std::tie(v0_, v1_, v2_, v3_, v4_) ==
				std::tie(rhs.v0_, rhs.v1_, rhs.v2_, rhs.v3_, rhs.v4_);
		}

		[[nodiscard]] bool operator!=(const udt_aggregate_type& rhs) const
		{
			return !(*this == rhs);
		}
	};

	template<>
	struct test_trait<udt_aggregate_type>
	{
		[[nodiscard]] static udt_aggregate_type construct()
		{
			return udt_aggregate_type
			{
				.v0_ = "Foxes are great!",
				.v1_ = 1,
				.v2_ = 12.345f,
				.v3_ = {1, 2, 3, 4, 5},
				.v4_ = { "Foxes", "are", "great" }
			};
		}
	};

#endif

	template<std::size_t I>
	class udt_serialize_from_members
	{
		std::string v0_;
		int v1_ = 0;
		float v2_ = 0.f;
		std::vector<int> v3_;
		std::vector<std::string> v4_;

	public:
		udt_serialize_from_members() = default;

		udt_serialize_from_members(std::string v0, int v1, float v2, std::vector<int> v3, std::vector<std::string> v4)
			: v0_(v0), v1_(v1), v2_(v2), v3_(v3), v4_(v4) {}

		template<class> friend struct test_trait;

		template<class> friend struct serialize_traits;

		[[nodiscard]] bool operator==(const udt_serialize_from_members& rhs) const
		{
			return std::tie(v0_, v1_, v2_, v3_, v4_) == 
				std::tie(rhs.v0_, rhs.v1_, rhs.v2_, rhs.v3_, rhs.v4_);
		}

		[[nodiscard]] bool operator!=(const udt_serialize_from_members& rhs) const
		{
			return !(*this == rhs);
		}

		using serialize_traits = std::conditional_t<
			I == 1,
			serialize_from_members<
			udt_serialize_from_members,
			&udt_serialize_from_members::v0_,
			&udt_serialize_from_members::v1_,
			&udt_serialize_from_members::v2_,
			&udt_serialize_from_members::v3_,
			&udt_serialize_from_members::v4_
			>,
			void
		>;

		void serialize(bit_writer& writer) const requires (I == 2)
		{
			writer | v0_ | v1_ | v2_ | v3_ | v4_;
		}

		void deserialize(bit_reader& reader) requires (I == 2)
		{
			reader | v0_ | v1_ | v2_ | v3_ | v4_;
		}

		static void serialize(bit_writer& writer, const udt_serialize_from_members& o) requires (I == 3 || I == 4)
		{
			writer | o.v0_ | o.v1_ | o.v2_ | o.v3_ | o.v4_;
		}

		static void deserialize(bit_reader& reader, udt_serialize_from_members& o) requires (I == 3)
		{
			reader | o.v0_ | o.v1_ | o.v2_ | o.v3_ | o.v4_;
		}

		udt_serialize_from_members(from_bit_reader_t, bit_reader& reader) requires (I == 4)
		{
			reader | v0_ | v1_ | v2_ | v3_ | v4_;
		}
	};

	template<std::size_t I>
	struct test_trait<udt_serialize_from_members<I>>
	{
		[[nodiscard]] static udt_serialize_from_members<I> construct()
		{
			return udt_serialize_from_members<I>
			(
				"Foxes are great!",
				1,
				12.345f,
				{ 1, 2, 3, 4, 5 },
				{ "Foxes", "are", "great" }
			);
		}
	};

	template<>
	struct serialize_traits<udt_serialize_from_members<0>> :
		serialize_from_members<
			udt_serialize_from_members<0>,
			&udt_serialize_from_members<0>::v0_,
			&udt_serialize_from_members<0>::v1_,
			&udt_serialize_from_members<0>::v2_,
			&udt_serialize_from_members<0>::v3_,
			&udt_serialize_from_members<0>::v4_
		>
	{};

	using types = ::testing::Types <
#ifdef FOX_SERIALIZE_HAS_REFLEXPR
		udt_aggregate_type,
#endif
		char,
		int,
		unsigned int,
		udt_trivial,
		std::array<int, 10>,
		std::array<std::string, 10>,
		std::string_view,
		std::span<int>,
		std::vector<int>,
		std::string,
		std::vector<std::string>,
		std::tuple<int, float>,
		std::tuple<int, float, std::string>,
		std::pair<int, int>,
		std::pair<int, std::string>,
		std::unordered_map<int, std::string>,
		std::unordered_map<std::string, int>,
		std::map<std::string, int>,
		std::unordered_set<std::string>,
		std::set<std::string>,
		std::variant<std::string, int, std::vector<int>>,
		std::optional<std::string>,
		std::optional<int>,
		std::optional<std::nullptr_t>,
		udt_serialize_from_members<0>,
		udt_serialize_from_members<1>,
		udt_serialize_from_members<2>,
		udt_serialize_from_members<3>,
		udt_serialize_from_members<4>
	>;

	INSTANTIATE_TYPED_TEST_SUITE_P(fundamental, serialize_test, types);
}