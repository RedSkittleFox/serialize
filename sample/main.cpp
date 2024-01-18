#include <string_view>
#include <array>
#include <fox/serialize.hpp>

int main()
{
	fox::serialize::bit_writer writer;
	int a = 5;
	std::string sugoma = "sus\n";
	writer | a | sugoma;
	fox::serialize::bit_reader reader(writer.data());
	int b;
	std::string out;
	reader | b | out;
	assert(a == b && sugoma == out);
	return 0;
}