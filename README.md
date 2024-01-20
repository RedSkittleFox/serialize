[![CMake MSVC Windows Build and Test](https://github.com/RedSkittleFox/serialize/actions/workflows/cmake-msvc-windows.yml/badge.svg)](https://github.com/RedSkittleFox/serialize/actions/workflows/cmake-msvc-windows.yml)
[![CMake CLANG Windows Build and Test](https://github.com/RedSkittleFox/serialize/actions/workflows/cmake-clang-windows.yml/badge.svg)](https://github.com/RedSkittleFox/serialize/actions/workflows/cmake-clang-windows.yml)

# serialize
is a C++ 23 serialization library. It provides a simple to use interfaces, supports containers, trivial types and aggregates[1] out of the box and provides a simple interface to create custom serialization methods.

[1] [Aggregate](https://en.cppreference.com/w/cpp/language/aggregate_initialization) types are supported via [fox::reflexpr](https://github.com/RedSkittleFox/reflexpr/tree/main) reflections library.

# Examples

## Builtin serializers
Library uses `operator|` to chain together objects to serialize and deserialize. 
Builtin supported types are:
* Trivailly copyable types
* [range-to](https://en.cppreference.com/w/cpp/ranges/to) compatible types
* [tuple-like](https://en.cppreference.com/w/cpp/utility/tuple/tuple-like) types
* `std::optional`

```cpp
#include <fox/serialize.hpp>

namespace sr = fox::serialize;

std::string a = "Capybara";
int b = 123;

sr::bit_writer writer;
writer | a | b;

sr::bit_reader reader(std::from_range, writer.data());
std::string a_out;
int b_out;
reader | a_out | b_out;
```

## Aggregates
Aggregate types are supported through [fox::reflexpr](https://github.com/RedSkittleFox/reflexpr/tree/main) reflections library if available. Library can be disabled by setting `FOX_SERIALIZE_INCLUDE_REFLEXPR` CMAKE flag to OFF.

```cpp
#include <fox/serialize.hpp>

namespace sr = fox::serialize;

struct aggregate
{
	std::string a;
	int b;
};

aggregate a{ "Fox, 123 };

sr::bit_writer writer;
writer | a;

sr::bit_reader reader(std::from_range, writer.data());
aggregate a_out;
reader | a_out;
```

## Custom serialization methods
It's possible to provide custom serialization method for a given type.

### [Custom `serialize_trait<T>`](sample/sample_custom_0.cpp)
```cpp
#include <fox/serialize.hpp>

template<>
struct fox::serialize::serialize_traits<std::unique_ptr<int>>
{
	static void serialize(fox::serialize::bit_writer& writer, const std::unique_ptr<int>& ptr)
	{
		writer | (static_cast<bool>(ptr));
		if (ptr)
			writer | (*ptr);
	}

	static void deserialize(fox::serialize::bit_reader& reader, std::unique_ptr<int>& ptr)
	{
		bool b = fox::serialize::deserialize<bool>(reader);
		if(b)
		{
			ptr = std::make_unique<int>(fox::serialize::deserialize<int>(reader));
		}
		else
		{
			ptr.reset();
		}
	}
};

void sample_custom_0()
{
	fox::serialize::bit_writer writer;

	auto a = std::make_unique<int>(2);
	writer | a;

	fox::serialize::bit_reader reader(std::from_range, writer.data());

	std::unique_ptr<int> b;
	reader | b;
}
```

### [Member functions](sample/sample_custom_1.cpp)
```cpp
#include <fox/serialize.hpp>

class custom_type_1
{
	friend void sample_custom_1();

	std::string v0_;
	int v1_;

public:
	custom_type_1() = default;

	custom_type_1(const std::string& v0, int v1)
		: v0_(v0), v1_(v1) {}

	void serialize(fox::serialize::bit_writer& writer) const
	{
		writer | v0_ | v1_;
	}

	void deserialize(fox::serialize::bit_reader& reader)
	{
		reader | v0_ | v1_;
	}
};

void sample_custom_1()
{
	fox::serialize::bit_writer writer;

	custom_type_1 a = custom_type_1("Fox", 1);
	writer | a;

	fox::serialize::bit_reader reader(std::from_range, writer.data());

	custom_type_1 b;
	reader | b;
}
```

### [Static member methods](sample/sample_custom_2.cpp)
```cpp
#include <fox/serialize.hpp>

class custom_type_2
{
	friend void sample_custom_2();

	std::string v0_;
	int v1_;

public:
	custom_type_2() = default;

	custom_type_2(const std::string& v0, int v1)
		: v0_(v0), v1_(v1) {}

	static void serialize(fox::serialize::bit_writer& writer, const custom_type_2& obj) 
	{
		writer | obj.v0_ | obj.v1_;
	}

	static void deserialize(fox::serialize::bit_reader& reader, custom_type_2& obj)
	{
		reader | obj.v0_ | obj.v1_;
	}
};

void sample_custom_2()
{
	fox::serialize::bit_writer writer;

	custom_type_2 a = custom_type_2("Fox", 1);
	writer | a;

	fox::serialize::bit_reader reader(std::from_range, writer.data());

	custom_type_2 b;
	reader | b;
}
```
### [Constructor accepting bit_reader](sample/sample_custom_3.cpp)
```cpp
#include <fox/serialize.hpp>

#include <iostream>
#include <format>
#include <string>

class custom_type_3
{
	friend void sample_custom_3();

	std::string v0_;
	int v1_;

public:
	custom_type_3() = default;

	custom_type_3(fox::serialize::from_bit_reader_t, fox::serialize::bit_reader& reader)
	{
		reader | v0_ | v1_;
	}

	custom_type_3(const std::string& v0, int v1)
		: v0_(v0), v1_(v1) {}

	static void serialize(fox::serialize::bit_writer& writer, const custom_type_3& obj)
	{
		writer | obj.v0_ | obj.v1_;
	}

};

void sample_custom_3()
{
	fox::serialize::bit_writer writer;

	custom_type_3 a = custom_type_3("Fox", 1);
	writer | a;

	fox::serialize::bit_reader reader(std::from_range, writer.data());

	custom_type_3 b;
	reader | b;
}
```
### [Custom member trait](sample/sample_custom_4.cpp)
```cpp
#include <fox/serialize.hpp>

class custom_type_4
{
	friend void sample_custom_4();

	std::string v0_;
	int v1_;

public:
	custom_type_4() = default;

	custom_type_4(const std::string& v0, int v1)
		: v0_(v0), v1_(v1) {}

	// This can be a hand written trait like in a sample samples/sample_custom_0.cpp
	using serialize_trait = fox::serialize::serialize_from_members<custom_type_4, &custom_type_4::v0_, &custom_type_4::v1_>;
};

void sample_custom_4()
{
	fox::serialize::bit_writer writer;

	custom_type_4 a = custom_type_4("Fox", 1);
	writer | a;

	fox::serialize::bit_reader reader(std::from_range, writer.data());

	custom_type_4 b;
	reader | b;
}
```
### Member variable list
```cpp
struct custom_struct
{
	std::string a;
	int b;
};

template<>
struct fox::serialize::serialize_traits<custom_struct> : 
	fox::serialize::serialize_from_members<custom_struct, &custom_struct::a, &custom_struct::b>
{
};
```

# Supported compilers
This is a C++ 23 library. All compilers that support `std::ranges::to` and `std::format` should support this library. Following compilers are known to work:
* Microsoft Visual C++ 2022 (msvc) / Build Tools 19.38.33134 (and possibly later)
* Microsoft Visual C++ 2022 (clang) 16.0.5 (and possibly later)

# Integration
[`serialize.hpp`](include/fox/serialize.hpp) is single header library. It requires C++ 23. If you want to enable aggreage serialization, install (single header library) [`fox::reflexpr`](https://github.com/RedSkittleFox/reflexpr) and define `FOX_SERIALIZE_HAS_REFLEXPR` macro before including `fox/serialize.hpp`.

## CMake
```cmake
include(FetchContent)

# One could disable fox::reflexpr by setting FOX_SERIALIZE_INCLUDE_REFLEXPR to OFF
# set(FOX_SERIALIZE_INCLUDE_REFLEXPR OFF)

FetchContent_Declare(fox_serialize GIT_REPOSITORY https://github.com/RedSkittleFox/serialize.git)
FetchContent_MakeAvailable(fox_serialize)
target_link_libraries(foo PRIVATE fox::serialize)
```

# License
This library is licensed under the [MIT License](LICENSE).