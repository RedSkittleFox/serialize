#include <gtest/gtest.h>
#include <fox/serialize.hpp>
#include <random>

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
		using value_type = TypeParam;
		fox::serialize::bit_writer writer;
		value_type a = test_trait<value_type>::construct();;
		writer | a;
		fox::serialize::bit_reader reader(writer.data());
		value_type b;
		reader | b;
		EXPECT_EQ(a, b);
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
		}
	};

	using types = ::testing::Types<
		char,
		int,
		unsigned int
	>;
	INSTANTIATE_TYPED_TEST_SUITE_P(fundamental, serialize_test, types);
}