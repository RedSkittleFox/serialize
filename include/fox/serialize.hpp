#ifndef FOX_SERIALIZE_H_
#define FOX_SERIALIZE_H_
#pragma once

#include <tuple>
#include <fox/reflexpr.hpp>

namespace fox::serialize
{
	class bit_writer;
	class bit_reader;

	template<class T>
	struct serialize_traits
	{

	};

	template<class T>
	concept serializable = requires (bit_writer & writer, bit_reader & reader, const T & a, T & b)
	{
		{ serialize_traits<T>::do_serialize(writer, a) } -> std::same_as<void>;
		{ serialize_traits<T>::do_deserialize(reader, b) } -> std::same_as<void>;
	};

	template<class T>
	struct is_serializable : std::bool_constant<serializable<T>> {};

	template<class T>
	static constexpr bool is_serializable_v = is_serializable<T>::value;

	class basic_bit_buffer
	{
		
	};

	class bit_writer
	{

	public:

	};

	class bit_reader
	{

	public:

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
