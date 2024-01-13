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
#include <vector>
#include <format>

#include <fox/reflexpr.hpp>

namespace fox::serialize
{
	class bit_writer;
	class bit_reader;

	class bit_writer
	{
		std::vector<std::byte> buffer_;
	public:
		[[nodiscard]] void* write_bytes(std::size_t num_bytes)
		{
			const std::size_t offset = std::size(buffer_);
			buffer_.resize(offset + num_bytes);
			return static_cast<void*>(std::data(buffer_) + offset);
		}

		template<std::size_t NumBytes>
		[[nodiscard]] void* write_bytes()
		{
			const std::size_t offset = std::size(buffer_);
			buffer_.resize(offset + NumBytes);
			return static_cast<void*>(std::data(buffer_) + offset);
		}

	public:
		[[nodiscard]] std::span<const std::byte> data() const noexcept
		{
			return buffer_;
		}
	};

	class bit_reader
	{
		std::vector<std::byte> buffer_;
		std::size_t offset_{};
	public:
		template<std::ranges::range Range>
		bit_reader(Range&& range)
			requires std::is_trivial_v<std::ranges::range_value_t<Range>>
		{
			using value_type = std::ranges::range_value_t<Range>;
			if constexpr (std::ranges::contiguous_range<Range>)
			{
				auto span = std::as_bytes(std::span(range));
				buffer_ = span | std::ranges::to<std::vector<std::byte>>();
			}
			else
			{
				auto it = std::begin(range);
				auto end = std::end(range);
				buffer_ = std::ranges::subrange(it, end)
					| std::transform(
					[](const value_type& v) -> std::array<std::byte, sizeof(value_type)>
					{ return std::bit_cast<std::array<std::byte, sizeof(value_type)>>(v); })
					| std::views::join
					| std::ranges::to<std::vector<std::byte>>();
			}
		}

	public:
		[[nodiscard]] const void* read_bytes(std::size_t num_bytes)
		{
			const void* ptr = static_cast<const void*>(std::data(buffer_) + offset_);
			offset_ += num_bytes;
			return ptr;
		}

		template<std::size_t NumBytes>
		[[nodiscard]] const void* read_bytes()
		{
			const void* ptr = static_cast<const void*>(std::data(buffer_) + offset_);
			offset_ += NumBytes;
			return ptr;
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
	static constexpr bool is_serializable_v = serializable<T>;

	template<class T>
	struct is_serializable : std::bool_constant<is_serializable_v<T>> {};

	template<class T>
	static constexpr bool is_deserializable_v = deserializable<T>;

	template<class T>
	struct is_deserializable : std::bool_constant<is_deserializable_v<T>> {};

	template<serializable T>
	bit_writer& operator|(bit_writer& lhs, const T& rhs)
	{
		serialize_traits<T>::serialize(lhs, rhs);
		return lhs;
	}

	template<serializable T>
	bit_reader& operator|(bit_reader& lhs, T& rhs)
	{
		serialize_traits<T>::deserialize(lhs, rhs);
		return lhs;
	}

	template<class T>
		requires !std::ranges::range<T> && std::is_trivially_copyable_v<T>
	struct serialize_traits<T>
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

		template<class T>
		struct is_array : std::false_type {};

		template<class T, std::size_t Size>
		struct is_array<std::array<T, Size>> : std::true_type {};

		template<class Container>
		constexpr bool reservable_container =
			std::ranges::sized_range<Container> &&
			requires(Container& c, std::ranges::range_size_t<Container> n)
		{
			c.reserve(n);
			{ c.capacity() } -> std::same_as<decltype(n)>;
			{ c.max_size() } -> std::same_as<decltype(n)>;
		};

		template<class Container, class Ref>
		constexpr bool container_insertable =
			requires(Container& c, Ref&& ref)
		{
			requires(requires {c.push_back(std::forward<Ref>(ref)); } ||
				requires {c.insert(c.end(), std::forward<Ref>(ref)); });
		};

		template<class C, std::ranges::input_range R, class... Args>
		constexpr bool is_ranges_to_convertible =
			// Mandates: C is a cv-unqualified class type
			std::is_const_v<C> == false &&
			std::is_volatile_v<C> == false &&
			std::is_class_v<C> == true &&
			(
			( 
				(std::ranges::input_range<C> == false || std::convertible_to<std::ranges::range_reference_t<R>, std::ranges::range_value_t<R>> == true) &&
				(
					std::constructible_from<C, R, Args...> == true ||
					std::constructible_from<C, std::from_range_t, R, Args...> == true ||
					(
						std::ranges::common_range<R> && 
						std::derived_from<typename std::iterator_traits<std::ranges::iterator_t<R>>::iterator_category, std::input_iterator_tag> &&
						std::constructible_from<C, std::ranges::iterator_t<R>, std::ranges::sentinel_t<R>, Args...> == true
					) ||
					(
						std::constructible_from<C, Args...> == true &&
						container_insertable<C, std::ranges::range_reference_t<R>> == true
					)
				)
			) ||
			( std::ranges::input_range<std::ranges::range_reference_t<R>> && std::convertible_to<std::ranges::range_reference_t<R>, C> )
			);

		template<template <std::size_t, class> class Trait, class T, class>
		struct indexed_conjunction_proxy : std::false_type {};

		template<template <std::size_t, class> class Trait, class T, std::size_t... Idx>
		struct indexed_conjunction_proxy<Trait, T, std::index_sequence<Idx...>> : std::conjunction<Trait<Idx, T>...> {};

		template<template <std::size_t, class> class Trait, class T, std::size_t Size>
		struct indexed_conjunction : indexed_conjunction_proxy < Trait, T, std::make_index_sequence<Size>> {};

		template<template <std::size_t, class> class Trait, class T>
		struct indexed_conjunction<Trait, T, static_cast<std::size_t>(0)> : std::true_type {};

		template<std::size_t Idx, class T>
		constexpr bool has_tuple_element_v =
			requires(T tuple)
		{
			typename std::tuple_element_t<Idx, std::remove_cvref_t<T>>;
			{ std::get<Idx>(tuple) } -> std::convertible_to<const std::tuple_element_t<Idx, T>&>;
		};

		template<std::size_t Idx, class T>
		struct has_tuple_element : std::bool_constant<has_tuple_element_v<Idx, T>> {};

		template<class T>
		constexpr bool is_tuple_like_v =
			std::is_reference_v<T> == false &&
			requires
		{
			typename std::tuple_size<T>::type;
			requires std::derived_from<std::tuple_size<T>, std::integral_constant<std::size_t, std::tuple_size_v<T>>>;
			requires indexed_conjunction<has_tuple_element, T, std::tuple_size_v<T>>::value;
		};

		template<class T>
		concept tuple_like = is_tuple_like_v<T>;
	}

	template<std::ranges::range T>
	struct serialize_traits<T>
	{
		using _library_provided_trait = std::true_type;

		static void serialize(bit_writer& writer, const T& range) requires
			serializable<std::ranges::range_value_t<T>>
		{
			using value_type = std::ranges::range_value_t<T>;

			std::size_t range_size = std::size(range);
			writer | range_size;

			// Check if we can memcpy the range
			if
			constexpr( 
				std::ranges::contiguous_range<T> && 
				::fox::serialize::details::uses_builtin_serializer<value_type> == false&&
				std::is_trivially_copyable_v<T>
				)
			{
				T* dest = static_cast<T*>(writer.write_bytes(sizeof(value_type) * range_size));
				std::memcpy(dest, std::data(range), sizeof(value_type) * range_size);
			}
			else // We iterate over the range
			{
				for(auto&& e : range)
				{
					writer | e;
				}
			}
		}

		static void deserialize(bit_reader& reader, T& value) requires
			!std::ranges::borrowed_range<T> && 
			deserializable<std::ranges::range_value_t<T>> &&
			(
				details::is_array<T>::value || 
				::fox::serialize::details::is_ranges_to_convertible<T, std::span<const std::ranges::range_value_t<T>>>
			)
		{
			using value_type = std::ranges::range_value_t<T>;
			using const_value_type = std::add_const_t<std::ranges::range_value_t<T>>;

			std::size_t size{};
			reader | size;

			if
			constexpr (
				::fox::serialize::details::is_ranges_to_convertible<T, std::span<const value_type>> &&
				::fox::serialize::details::uses_builtin_serializer<value_type> == false &&
				std::is_trivially_default_constructible_v<value_type> && std::is_trivially_copyable_v<value_type>
				)
			{
				const value_type* ptr = static_cast<const value_type*>(reader.read_bytes(sizeof(value_type) * size));
				value = std::span<const_value_type >{ ptr, size } | std::ranges::to<T>();
			}
			else if constexpr(::fox::serialize::details::is_array<T>::value)
			{
				if (size > std::size(value))
				{
					throw std::out_of_range(std::format("Trying to read ranges ({} elements) bigger than the array ({} elements).",
						size, std::size(value))
					);
				}

				if constexpr(std::is_trivially_default_constructible_v<value_type> && std::is_trivially_copyable_v<value_type>)
				{
				
					auto ptr = static_cast<const value_type*>(reader.read_bytes(sizeof(value_type) * size));
					std::memcpy(static_cast<void*>(std::data(value)), static_cast<const void*>(ptr), sizeof(value_type)* size);
				}
				else
				{
					for (auto&& e : value)
					{
						reader | e;
					}
				}
			}
			else
			{
				auto ptr = static_cast<const T*>(reader.read_bytes(sizeof(value_type) * size));
				value = std::views::iota(static_cast<std::size_t>(0), size)
					| std::ranges::transform([&](auto) -> value_type { value_type v; serialize_traits<value_type>::deserialize(v); return v; })
					| std::views::as_rvalue
					| std::ranges::to<T>();
			}
		}
	};

	template<details::tuple_like T>
	requires !std::ranges::range<T> && !std::is_trivially_copyable_v<T>
	struct serialize_traits<T>
	{
		using _library_provided_trait = std::true_type;

	private:
		template<std::size_t Idx, class T>
		struct _tuple_element_serializable : is_serializable<std::tuple_element_t<Idx, T>> {};

		template<std::size_t Idx, class T>
		struct _tuple_element_deserializable : is_deserializable<std::tuple_element_t<Idx, T>> {};

	public:
		static void serialize(bit_writer& writer, const T& tuple)
			requires details::indexed_conjunction<_tuple_element_serializable, T, std::tuple_size_v<T>>::value
		{
			[&] <std::size_t... Idx>(std::index_sequence<Idx...>)
			{
				( serialize_traits<std::tuple_element_t<Idx, T>>::serialize(writer, std::get<Idx>(tuple)), ...);
			}(std::make_index_sequence<std::tuple_size_v<T>>{});
		}

		static void deserialize(bit_reader& reader, T& tuple)
			requires details::indexed_conjunction<_tuple_element_deserializable, T, std::tuple_size_v<T>>::value
		{
			[&]<std::size_t... Idx>(std::index_sequence<Idx...>)
			{
				(serialize_traits<std::tuple_element_t<Idx, T>>::deserialize(reader, std::get<Idx>(tuple)), ...);
			}(std::make_index_sequence<std::tuple_size_v<T>>{});
		}
	};

	template<::fox::reflexpr::aggregate T>
		requires !std::ranges::range<T> && !std::is_trivially_copyable_v<T> && !details::tuple_like<T>
	struct serialize_traits<T>
	{
		static void serialize(bit_writer& writer, const T& aggregate)
		{
			using tuple = decltype(fox::reflexpr::make_tuple(std::declval<T>()));
			fox::reflexpr::for_each_member_variable(aggregate, [&](auto&& v) { writer | v; });
		}

		static void deserialize(bit_reader& reader, T& aggregate)
		{
			fox::reflexpr::for_each_member_variable(aggregate, [&](auto&& v) { reader | v; });
		}
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

		// using v = serializable_members_list<&amogus::a, &amogus::b>;
	};

	
}

#endif
