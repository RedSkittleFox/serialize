#include <fox/serialize.hpp>

#include <iostream>
#include <format>
#include <string>

void sample_containers()
{
	fox::serialize::bit_writer writer;

	std::unordered_map<std::string, int> a{ { "Fox", 1 }, {"Capybara", 2 } };
	writer | a;

	fox::serialize::bit_reader reader(std::from_range, writer.data());
	std::unordered_map<std::string, int> b;
	reader | b;

	std::cout << std::format("=== Aggregate Types Sample ===\n");

	std::cout << "A = [";
	for (auto&& [k, v] : a)
		std::cout << std::format(" [ {}, {} ] ", v, k);

	std::cout << "]\nB = [";
	for (auto&& [k, v] : b)
		std::cout << std::format(" [ {}, {} ] ", v, k);
	std::cout << "]\n";
}