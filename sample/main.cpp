#include <string_view>
#include <array>
#include <fox/serialize.hpp>

int main()
{
	fox::serialize::bit_writer writer;
	int a = 5;
	writer | a;
	fox::serialize::bit_reader reader(writer.data());
	int b;
	reader | b;
	assert(a == b);
	return 0;
}