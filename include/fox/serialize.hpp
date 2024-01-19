#ifndef FOX_SERIALIZE_H_
#define FOX_SERIALIZE_H_
#pragma once

#include <version>

#if !defined(__cpp_lib_ranges_to_container)
#error "RedSkittleFox::Serialize requires STL to implement std::ranges::to for it's container serialization specializations."
#endif

#include <tuple>
#include <utility>
#include <iterator>
#include <span>
#include <ranges>
#include <vector>
#include <format>

#ifdef FOX_SERIALIZE_HAS_REFLEXPR
#include <fox/reflexpr.hpp>
#endif

namespace fox::serialize
{
#pragma region streams
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

#pragma endregion streams

#pragma region traits
	/**
	 * \brief Trait class used to provide serialization methods for a given type.
	 * \tparam T Serialized type.
	 */
	template<class T> struct serialize_traits;

	namespace details
	{
		// Internal serialization trait, selected if no public serialize_traits is available
		template<class T> struct builtin_serialize_traits;

		template<class T>
		concept builtin_serializable = requires (bit_writer & writer, const T & a)
		{
			{ ::fox::serialize::details::builtin_serialize_traits<T>::serialize(writer, a)		} -> std::same_as<void>;
		};

		template<class T>
		concept builtin_deserializable = requires (bit_reader & reader, T & b)
		{
			{ ::fox::serialize::details::builtin_serialize_traits<T>::deserialize(reader, b)	} -> std::same_as<void>;
		};

		template<class T>
		concept custom_serializable = requires (bit_writer & writer, const T & a)
		{
			{ serialize_traits<T>::serialize(writer, a)		} -> std::same_as<void>;
		};

		template<class T>
		concept custom_deserializable = requires (bit_reader & reader, T & b)
		{
			{ serialize_traits<T>::deserialize(reader, b)	} -> std::same_as<void>;
		};
	}

	template<class T>
	concept serializable = ::fox::serialize::details::builtin_serializable<T> || ::fox::serialize::details::custom_serializable<T>;

	template<class T>
	concept deserializable = ::fox::serialize::details::builtin_deserializable<T> || ::fox::serialize::details::custom_deserializable<T>;

	template<class T>
	static constexpr bool is_serializable_v = serializable<T>;

	template<class T>
	struct is_serializable : std::bool_constant<is_serializable_v<T>> {};

	template<class T>
	static constexpr bool is_deserializable_v = deserializable<T>;

	template<class T>
	struct is_deserializable : std::bool_constant<is_deserializable_v<T>> {};

	namespace details
	{
		template<::fox::serialize::serializable T>
		void do_serialize(bit_writer& lhs, const T& rhs)
		{
			if constexpr (::fox::serialize::details::custom_serializable<T>)
			{
				return serialize_traits<T>::serialize(lhs, rhs);
			}
			else
			{
				return ::fox::serialize::details::builtin_serialize_traits<T>::serialize(lhs, rhs);
			}
		}

		template<::fox::serialize::deserializable T>
		void do_deserialize(bit_reader& lhs, T& rhs)
		{
			if constexpr (::fox::serialize::details::custom_deserializable<T>)
			{
				return serialize_traits<T>::deserialize(lhs, rhs);
			}
			else
			{
				return ::fox::serialize::details::builtin_serialize_traits<T>::deserialize(lhs, rhs);
			}
		}
	}

	template<serializable T>
	bit_writer& operator|(bit_writer& lhs, const T& rhs)
	{
		::fox::serialize::details::do_serialize<T>(lhs, rhs);
		return lhs;
	}

	template<deserializable T>
	bit_reader& operator|(bit_reader& lhs, T& rhs)
	{
		::fox::serialize::details::do_deserialize<T>(lhs, rhs);
		return lhs;
	}


#pragma endregion traits

	namespace details
	{
		// This section provides implementation of default serialization functions for most common cases.
		// Serializations are divided into sections and then later bunched together, this simplifies SFINAE-compatible implementation later

#pragma region builtin_serialize_const_types
		// Implementation for const-types
		template<class T> requires builtin_serializable<T>
		struct builtin_serialize_traits<const T>
		{
			static void serialize(bit_writer& writer, const T& value)
			{
				return builtin_serialize_traits<T>::serialize(writer, value);
			}
		};
#pragma endregion builtin_serialize_const_types

#pragma region builtin_serialize_trivially_copyable
		// Implementation for trivially copyable types
		template<class T> requires !std::ranges::range<T> && std::is_trivially_copyable_v<T>
		struct builtin_serialize_traits<T>
		{
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
#pragma endregion builtin_serialize_trivially_copyable

#pragma region builtin_serialize_ranges
		template<class T>
		struct is_array : std::false_type {};

		template<class T, std::size_t Size>
		struct is_array<std::array<T, Size>> : std::true_type {};

		// Helper traits for std::ranges::to - concept 
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

		// std::ranges::to concept, std::ranges::to isn't SFINAE compatible - it uses static_assert
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

#pragma region tuple_like
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
#pragma endregion tuple_like

		template<class T>
		struct tuple_like_remove_const
		{
			using type = T;
		};

		template<template<class...> class Tuple, class... Ts> requires
			is_tuple_like_v<Tuple<Ts...>> && std::is_constructible_v<Tuple<Ts...>, Tuple<std::remove_const_t<Ts>...>>
		struct tuple_like_remove_const<Tuple<Ts...>>
		{
			using type = Tuple<std::remove_const_t<Ts>...>;
		};

		template<std::ranges::range T>
		struct builtin_serialize_traits<T>
		{
			static void serialize(bit_writer& writer, const T& range) requires
				serializable<std::ranges::range_value_t<T>>
			{
				using value_type = std::ranges::range_value_t<T>;

				const std::size_t range_size = std::size(range);
				writer | range_size;

				// Check if we can memcpy the range
				constexpr bool memcpy_compatible =
					std::ranges::contiguous_range<T> &&
					static_cast<bool>(fox::serialize::details::custom_serializable<value_type>) == false &&
					static_cast<bool>(fox::serialize::details::custom_deserializable<value_type>) == false &&
					std::is_trivially_copyable_v<value_type>;

				if constexpr (memcpy_compatible)
				{
					auto dest = static_cast<std::remove_const_t<T>*>(writer.write_bytes(sizeof(value_type) * range_size));
					(void)std::memcpy(dest, std::data(range), sizeof(value_type) * range_size);
				}
				else // We iterate over the range
				{
					for (auto&& e : range)
					{
						writer | e;
					}
				}
			}

			static constexpr bool is_deserializable =
				!std::ranges::borrowed_range<T> &&
				deserializable<tuple_like_remove_const<std::ranges::range_value_t<T>>> && // In case we are tuple with const member, remove const if constructible from it
				(::fox::serialize::details::is_array<T>::value || ::fox::serialize::details::is_ranges_to_convertible<T, std::span<const std::ranges::range_value_t<T>>>);

			static void deserialize(bit_reader& reader, T& value) requires is_deserializable
				
			{
				using value_type = typename tuple_like_remove_const<std::ranges::range_value_t<T>>::type;
				using const_value_type = std::add_const_t<std::ranges::range_value_t<T>>;

				std::size_t size{};
				reader | size;

				// Check if we can memcpy the range
				constexpr bool memcpy_compatible =
					static_cast<bool>(fox::serialize::details::custom_serializable<value_type>) == false &&
					static_cast<bool>(fox::serialize::details::custom_deserializable<value_type>) == false &&
					std::is_trivially_copyable_v<value_type>;

				if constexpr (::fox::serialize::details::is_ranges_to_convertible<T, std::span<const value_type>> && memcpy_compatible)
				{
					const value_type* ptr = static_cast<const value_type*>(reader.read_bytes(sizeof(value_type) * size));
					value = std::span<const_value_type >{ ptr, size } | std::ranges::to<T>();
				}
				else if constexpr (::fox::serialize::details::is_array<T>::value)
				{
					if (size != std::size(value))
					{
						throw std::out_of_range(std::format("Trying to read ranges ({} elements) of a different size than the array ({} elements).",
							size, std::size(value))
						);
					}

					// Memcpy array
					if constexpr (memcpy_compatible)
					{
						auto ptr = static_cast<const value_type*>(reader.read_bytes(sizeof(value_type) * size));
						std::memcpy(static_cast<void*>(std::data(value)), static_cast<const void*>(ptr), sizeof(value_type) * size);
					}
					else // Or iterate over elements and serialize them
					{
						for (auto&& e : value)
						{
							reader | e;
						}
					}
				}
				else // Iterate over elements, serialize them and then convert them into the range
				{
					value = std::views::iota(static_cast<std::size_t>(0), size)
						| std::views::transform([&](auto) -> value_type { value_type v; reader | v; return v; })
						| std::views::as_rvalue
						| std::ranges::to<T>();
				}
			}
		};

#pragma endregion builtin_serialize_ranges

#pragma region builtin_tuple_like
		template<class T>
			requires !std::ranges::range<T> && !std::is_trivially_copyable_v<T> && ::fox::serialize::details::tuple_like<T>
		struct builtin_serialize_traits<T>
		{
			template<std::size_t Idx, class U>
			struct tuple_element_serializable : is_serializable<std::tuple_element_t<Idx, U>> {};

			template<std::size_t Idx, class U>
			struct tuple_element_deserializable : is_deserializable<std::tuple_element_t<Idx, U>> {};

			static void serialize(bit_writer& writer, const T& tuple)
				requires details::indexed_conjunction<tuple_element_serializable, T, std::tuple_size_v<T>>::value
			{
				[&] <std::size_t... Idx>(std::index_sequence<Idx...>)
				{
					(::fox::serialize::details::do_serialize<std::tuple_element_t<Idx, T>>(writer, std::get<Idx>(tuple)), ...);
				}(std::make_index_sequence<std::tuple_size_v<T>>{});
			}

			static void deserialize(bit_reader& reader, T& tuple) requires
				::fox::serialize::details::indexed_conjunction<tuple_element_deserializable, T, std::tuple_size_v<T>>::value
			{
				[&] <std::size_t... Idx>(std::index_sequence<Idx...>)
				{
					(::fox::serialize::details::do_deserialize<std::tuple_element_t<Idx, T>>(reader, std::get<Idx>(tuple)), ...);
				}(std::make_index_sequence<std::tuple_size_v<T>>{});
			}
		};
#pragma endregion builtin_tuple_like

#pragma region builtin_aggregate_types
#ifdef FOX_SERIALIZE_HAS_REFLEXPR
		template<::fox::reflexpr::aggregate T>
			requires !std::ranges::range<T> && !std::is_trivially_copyable_v<T> && !::fox::serialize::details::tuple_like<T>
		struct builtin_serialize_traits<T>
		{
			static void serialize(bit_writer& writer, const T& aggregate)
				requires serializable<decltype(fox::reflexpr::tie(std::declval<T&>()))>
			{
				auto tie = fox::reflexpr::tie(aggregate);
				::fox::serialize::details::do_serialize<decltype(tie)>(writer, tie);
			}

			static void deserialize(bit_reader& reader, T& aggregate)
				requires deserializable<decltype(fox::reflexpr::tie(std::declval<T&>()))>
			{
				auto tie = fox::reflexpr::tie(aggregate);
				::fox::serialize::details::do_deserialize<decltype(tie)>(reader, tie);
			}
		};
#endif
#pragma endregion builtin_aggregate_types
	}

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
