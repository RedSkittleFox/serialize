#ifndef FOX_SERIALIZE_H_
#define FOX_SERIALIZE_H_
#pragma once

#include <version>

#if !defined(__cpp_lib_ranges_to_container)
#error "RedSkittleFox::Serialize requires STL to implement std::ranges::to for it's container serialization specializations."
#endif

#include <tuple>
#include <iterator>
#include <span>
#include <ranges>

#include <fox/reflexpr.hpp>

namespace fox::serialize
{
	class bit_writer;
	class bit_reader;

	class bit_writer
	{

	public:
		[[nodiscard]] void* write_bytes(std::size_t num_bytes)
		{

		}

		template<std::size_t NumBytes>
		[[nodiscard]] void* write_bytes()
		{

		}
	};

	class bit_reader
	{

	public:
		[[nodiscard]] const void* read_bytes(std::size_t num_bytes)
		{

		}

		template<std::size_t NumBytes>
		[[nodiscard]] const void* read_bytes()
		{

		}
	};

	template<class T>
	struct serialize_traits;

	template<class T>
	concept serializable = requires (bit_writer & writer, const T & a)
	{
		{ serialize_traits<T>::serialize(writer, a)		} -> std::same_as<void>;
	};

	template<class T>
	concept deserializable = requires (bit_reader & reader, T & b)
	{
		{ serialize_traits<T>::deserialize(reader, b)	} -> std::same_as<void>;
	};

	template<class T>
	struct is_serializable : std::bool_constant<serializable<T>> {};

	template<class T>
	static constexpr bool is_serializable_v = is_serializable<T>::value;

	template<class T>
		requires !std::ranges::range<T> && std::is_trivially_copyable_v<T>
	struct serialize_traits
	{
		using _library_provided_trait = std::true_type;

		static void serialize(bit_writer& writer, const T& value)
		{
			auto ptr = writer.write_bytes<sizeof(T)>();
			*static_cast<T*>(ptr) = value;
		}

		static void deserialize(bit_reader& reader, T& value)
		{
			auto ptr = reader.read_bytes<sizeof(T)>();
			value = *static_cast<const T*>(ptr);
		}
	};

	namespace details
	{
		template<class T, class U>
		concept to_range_convertible = requires(T & v, std::span<const U> span)
		{
			{ span | std::ranges::to<T>() } -> std::convertible_to<T>;
		} && std::same_as<std::ranges::range_value_t<T>, U>;

		template<class T>
		concept uses_builtin_serializer = requires
		{
			{ serialize_traits<T>::value_type } -> std::same_as<std::true_type>;
		};
	}

	template<std::ranges::range T>
	struct serialize_traits
	{
		using _library_provided_trait = std::true_type;

		static void serialize(bit_writer& writer, const T& range)
			requires serializable<std::ranges::range_value_t<T>>
		{
			using value_type = std::ranges::range_value_t<T>;

			// Check if we can memcpy the range
			if
			constexpr( 
				std::ranges::contiguous_range<T> && 
				::fox::serialize::details::uses_builtin_serializer<value_type> &&
				std::is_trivially_copyable_v<T>
				)
			{
				T* dest = static_cast<T*>(writer.write_bytes<sizeof(T)>());
				std::memcpy(dest, std::data(range), sizeof(value_type) * std::size(range));
			}
			else // We iterate over the range
			{
				std::size_t range_size = std::size(range);
				writer | range_size;
				for(auto&& e : range)
				{
					writer | e;
				}
			}
		}

		static void deserialize(bit_reader& reader, T& value)
			requires deserializable<std::ranges::range_value_t<T>>
		{
			auto ptr = reader.read_bytes<sizeof(T)>();
			value = *static_cast<const T*>(ptr);
		}
	};

	template<serializable T, class Allocator>
	struct serialize_traits<std::vector<T, Allocator>>
	{
		using _library_provided_trait = std::true_type;
	};

	class basic_bit_buffer
	{
		
	};



	class amogus
	{
		int a;
		int b;

		template<class> friend struct serializer;
	};

	template<class Ptr>
	struct type_of_owning_object;

	template<class T, class U>
	struct type_of_owning_object<U T::*>
	{
		using type = T;
		using member_object_type = U;
	};

	template<auto... MemberPointers>
		requires (sizeof...(MemberPointers) == 0)
	struct serializable_members_list
	{
		template<class U>
		static bit_writer& serialize(bit_writer& out, const U& object)
		{
			return out;
		}

		template<class U>
		static bit_reader& deserialize(bit_reader& in, U& object)
		{
			return in;
		}
	};

	template<auto MemberPointer, auto... MemberPointers>
	requires
		( std::is_member_object_pointer_v<decltype(MemberPointer)> ) &&
		( std::is_member_object_pointer_v<decltype(MemberPointers)> && ...) &&
		( std::same_as<
			typename type_of_owning_object<decltype(MemberPointer)>::type,
			typename type_of_owning_object<decltype(MemberPointers)>::type
		> && ... )
	struct serializer<serializable_members_list<MemberPointer, MemberPointers...>>
	{
		using class_type = typename type_of_owning_object<decltype(MemberPointer)>::type;
		static constexpr std::tuple<decltype(MemberPointers)...> member_pointers = { MemberPointers... };

		static bit_writer& serialize(bit_writer& out, const class_type& object )
		{
			// Validate that types are serializable
			auto validate = []<class T>(std::in_place_type_t<T>)
			{
				static_assert(is_serializable_v<T>, "Type [T] is not serializable.");
			};

			validate(std::in_place_type<typename type_of_owning_object<decltype(MemberPointer)>::member_object_type>);
			( validate(std::in_place_type<typename type_of_owning_object<decltype(MemberPointers)>::member_object_type>), ... );

			( out | (object).*MemberPointer );
			( (out | (object).*MemberPointers) | ...);
			return out;
		}

		static bit_reader& deserialize(bit_reader& in, class_type& object)
		{
			// Validate that types are serializable
			auto validate = []<class T>(std::in_place_type_t<T>)
			{
				static_assert(is_serializable_v<T>, "Type [T] is not serializable.");
			};

			validate(std::in_place_type<typename type_of_owning_object<decltype(MemberPointer)>::member_object_type>);
			(validate(std::in_place_type<typename type_of_owning_object<decltype(MemberPointers)>::member_object_type>), ...);

			(in | (object).*MemberPointer);
			((in | (object).*MemberPointers) | ...);
			return in;
		}
	};

	template<class T>
	struct serializer
	{
		static bit_writer& serialize(bit_writer& out, const T& v)
		{
			return out | v.x | v.y;
		}

		static bit_reader& deserialize(bit_reader& out, T& v)
		{
			return out | v.x | v.y;
		}

		using v = serializable_members_list<&amogus::a, &amogus::b>;
	};

	
}

#endif
