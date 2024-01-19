#include <string_view>
#include <array>
#include <fox/serialize.hpp>

int main()
{
	using pair = std::pair<const std::string, int>;

	fox::serialize::bit_writer writer;
	int a = 5;
	std::string sugoma = "sus\n";
	writer | a | sugoma;
	fox::serialize::bit_reader reader(writer.data());
	int b;
	std::string out;
	reader | b | out;

	pair p{};
	// fox::serialize::details::builtin_serialize_traits<pair>::deserialize(reader, p);

	assert(a == b && sugoma == out);
	return 0;
}