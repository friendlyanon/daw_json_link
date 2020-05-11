// The MIT License( MIT )
//
// Copyright (c) Darrell Wright
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and / or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "daw_iso8601_utils.h"
#include "daw_iterator_range.h"
#include "daw_json_assert.h"
#include "daw_json_parse_common.h"
#include "daw_json_parse_value.h"
#include "daw_json_to_string.h"
#ifndef _MSC_VER
#include "daw_murmur3.h"
#endif

#include <daw/daw_algorithm.h>
#include <daw/daw_cxmath.h>
#include <daw/daw_parser_helper_sv.h>
#include <daw/daw_scope_guard.h>
#include <daw/daw_sort_n.h>
#include <daw/daw_string_view.h>
#include <daw/daw_traits.h>
#include <daw/daw_utility.h>
#include <daw/iterator/daw_back_inserter.h>
#include <daw/iterator/daw_inserter.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace daw::json {
	template<JsonNullable>
	struct construct_from_iso8601_timestamp;

	template<>
	struct construct_from_iso8601_timestamp<JsonNullable::Never> {
		using result_type = std::chrono::time_point<std::chrono::system_clock,
		                                            std::chrono::milliseconds>;

		[[maybe_unused, nodiscard]] constexpr result_type
		operator( )( char const *ptr, std::size_t sz ) const {
			return datetime::parse_iso8601_timestamp( daw::string_view( ptr, sz ) );
		}
	};

	template<>
	struct construct_from_iso8601_timestamp<JsonNullable::Nullable> {
		using result_type =
		  std::optional<std::chrono::time_point<std::chrono::system_clock,
		                                        std::chrono::milliseconds>>;

		[[maybe_unused, nodiscard]] constexpr result_type operator( )( ) const {
			return {};
		}

		[[maybe_unused, nodiscard]] constexpr result_type
		operator( )( char const *ptr, std::size_t sz ) const {
			return datetime::parse_iso8601_timestamp( daw::string_view( ptr, sz ) );
		}
	};

	template<typename T>
	struct custom_from_converter_t {
		[[nodiscard]] constexpr decltype( auto ) operator( )( ) {
			if constexpr( std::is_same_v<T, std::string_view> ) {
				return std::string_view{};
			} else if constexpr( std::is_same_v<T,
			                                    std::optional<std::string_view>> ) {
				return std::string_view{};
			} else {
				return from_string( daw::tag<T> );
			}
		}

		[[nodiscard]] constexpr decltype( auto )
		operator( )( std::string_view sv ) {
			if constexpr( std::is_same_v<T, std::string_view> or
			              std::is_same_v<T, std::optional<std::string_view>> ) {
				return sv;
			} else {
				return from_string( daw::tag<T>, sv );
			}
		}
	};
} // namespace daw::json

namespace daw::json::json_details {
	[[nodiscard]] constexpr char const *str_find( char const *p, char c ) {
		while( *p != c ) {
			++p;
		}
		return p;
	}

	template<typename Container>
	struct basic_kv_appender {
		Container *m_container;

		explicit constexpr basic_kv_appender( Container &container )
		  : m_container( &container ) {}

		template<typename Key, typename Value>
		constexpr void operator( )( Key &&key, Value &&value ) {
			m_container->insert( std::make_pair( std::forward<Key>( key ),
			                                     std::forward<Value>( value ) ) );
		}
	};

	template<typename string_t>
	struct kv_t {
		string_t name;
		std::size_t pos;
		JsonParseTypes expected_type;

		constexpr kv_t( string_t Name, JsonParseTypes Expected, std::size_t Pos )
		  : name( daw::move( Name ) )
		  , pos( Pos )
		  , expected_type( Expected ) {}
	};

	template<std::size_t N, typename string_t, typename... JsonMembers>
	[[nodiscard]] constexpr kv_t<string_t> get_item( ) {
		using type_t = traits::nth_type<N, JsonMembers...>;
		return kv_t<string_t>( type_t::name, type_t::expected_type, N );
	}

	template<typename Range>
	struct location_info_t {
		daw::string_view name;
#ifndef _MSC_VER
		std::uint32_t hash_value;
#endif
		Range location{};

#ifndef _MSC_VER
		explicit constexpr location_info_t( daw::string_view Name ) noexcept
		  : name( Name )
		  , hash_value( daw::murmur3_32( Name ) ) {}
#else
		explicit constexpr location_info_t( daw::string_view Name ) noexcept
		  : name( Name ) {}
#endif

		[[maybe_unused, nodiscard]] constexpr bool missing( ) const {
			return location.is_null( );
		}
	};

	template<std::size_t N, typename Range>
	struct locations_info_t {
		using value_type = location_info_t<Range>;
		std::array<value_type, N> names;

		constexpr decltype( auto ) begin( ) const {
			return names.begin( );
		}

		constexpr decltype( auto ) begin( ) {
			return names.begin( );
		}

		constexpr decltype( auto ) end( ) const {
			return names.end( );
		}

		constexpr decltype( auto ) end( ) {
			return names.end( );
		}

		constexpr location_info_t<Range> const &
		operator[]( std::size_t idx ) const {
			return names[idx];
		}

		constexpr location_info_t<Range> &operator[]( std::size_t idx ) {
			return names[idx];
		}

		static constexpr std::size_t size( ) noexcept {
			return N;
		}

		[[nodiscard]] constexpr std::size_t
		find_name( daw::string_view key ) const {
#ifdef _MSC_VER
			// Bug in MSVC is making the constexpr string creation not work
			return algorithm::find_index_of_if(
			  begin( ), end( ),
			  [key]( auto const &loc ) { return loc.name == key; } );
#else
			return algorithm::find_index_of_if(
			  begin( ), end( ),
			  [&, hash = daw::murmur3_32( key )]( auto const &loc ) {
				  return loc.hash_value == hash and loc.name == key;
			  } );
#endif
		}
	}; // namespace daw::json::json_details

	template<typename JsonMember, std::size_t N, typename Range>
	[[nodiscard]] constexpr Range
	find_class_member( std::size_t pos, locations_info_t<N, Range> &locations,
	                   Range &rng ) {

		daw_json_assert_weak(
		  ( is_json_nullable_v<JsonMember> or not locations[pos].missing( ) or
		    not rng.front( '}' ) ),
		  "Unexpected end of class.  Non-nullable members still not found" );

		rng.trim_left_unchecked( );
		while( locations[pos].missing( ) and rng.front( ) != '}' ) {
			daw_json_assert_weak( rng.can_parse_more( ), "Unexpected end of stream" );
			auto const name = parse_name( rng );
			auto const name_pos = locations.find_name( name );
			if( name_pos >= locations.size( ) ) {
				// This is not a member we are concerned with
				(void)skip_value( rng );
				rng.clean_tail( );
				continue;
			}
			if( name_pos == pos ) {
				locations[pos].location = rng;
			} else {
				// We are out of order, store position for later
				// OLDTODO:	use type knowledge to speed up skip
				// OLDTODO:	on skipped classes see if way to store
				// 				member positions so that we don't have to
				//				reparse them after
				// RESULT: storing preparsed is slower, don't try 3 times
				// it also limits the type of things we can parse potentially
				// Using locations to switch on BaseType is slower too
				locations[name_pos].location = skip_value( rng );
				rng.clean_tail( );
			}
		}
		return locations[pos].location;
	}

	namespace pocm_details {
		template<typename Range>
		constexpr void move_rng_to_member_n( Range &rng,
		                                     std::size_t &current_position,
		                                     std::size_t desired_position ) {

			daw_json_assert_weak( rng.can_parse_more( ), "Unexpected end of range" );
			daw_json_assert_weak( current_position <= desired_position,
			                      "Order of ordered members must be ascending" );
			rng.clean_tail( );
			while( current_position < desired_position and rng.front( ) != ']' ) {
				(void)skip_value( rng );
				rng.clean_tail( );
				++current_position;
				daw_json_assert_weak( rng.can_parse_more( ),
				                      "Unexpected end of range" );
			}
		}
	} // namespace pocm_details

	template<typename JsonMember, typename Range>
	[[nodiscard]] constexpr auto
	parse_ordered_class_member( std::size_t &member_position, Range &rng ) {

		if constexpr( is_an_ordered_member_v<JsonMember> ) {
			pocm_details::move_rng_to_member_n( rng, member_position,
			                                    JsonMember::member_index );
		} else {
			rng.clean_tail( );
		}
		using json_member_type = ordered_member_subtype_t<JsonMember>;

		// this is an out value, get position ready
		++member_position;
		if( rng.front( ) == ']' ) {
			if constexpr( is_json_nullable_v<ordered_member_subtype_t<JsonMember>> ) {
				using constructor_t = typename json_member_type::constructor_t;
				return constructor_t{}( );
			} else if constexpr( is_json_nullable_v<json_member_type> ) {
				daw_json_error( missing_member( "ordered_class_member" ) );
			}
		}
		return parse_value<json_member_type>(
		  ParseTag<json_member_type::expected_type>{}, rng );
	}

	template<typename JsonMember, std::size_t N, typename Range>
	[[nodiscard]] constexpr json_result<JsonMember>
	parse_class_member( std::size_t member_position,
	                    locations_info_t<N, Range> &locations, Range &rng ) {

		rng.clean_tail( );
		static_assert( not is_no_name<JsonMember::name>,
		               "Array processing should never call parse_class_member" );

		daw_json_assert_weak( rng.front( "\"}" ),
		                      "Expected end of class or start of member" );
		auto loc = find_class_member<JsonMember>( member_position, locations, rng );

		// If the member was found loc will have it's position
		if( not loc.is_null( ) ) {
			if( loc.begin( ) == rng.begin( ) ) {
				return parse_value<JsonMember>( ParseTag<JsonMember::expected_type>{},
				                                rng );
			}
			return parse_value<JsonMember, true>(
			  ParseTag<JsonMember::expected_type>{}, loc );
		}
		if constexpr( is_json_nullable_v<JsonMember> ) {
			return parse_value<JsonMember, true>(
			  ParseTag<JsonMember::expected_type>{}, loc );
		} else {
			daw_json_error( missing_member( JsonMember::name ) );
		}
	}

	template<typename... JsonMembers, typename OutputIterator, std::size_t... Is,
	         typename Tuple, typename Value>
	[[nodiscard]] constexpr OutputIterator
	serialize_json_class( OutputIterator it, std::index_sequence<Is...>,
	                      Tuple const &args, Value const &value ) {

		bool is_first = true;
		*it++ = '{';

		auto visited_members =
		  daw::bounded_vector_t<daw::string_view, sizeof...( JsonMembers ) * 2U>{};
		// Tag Members, if any.  Putting them ahead means we can parse this faster
		// in the future
		(void)( ( tags_to_json_str<Is,
		                           daw::traits::nth_element<Is, JsonMembers...>>(
		            is_first, it, value, visited_members ),
		          ... ),
		        0 );
		// Regular Members
		(void)( ( to_json_str<Is, daw::traits::nth_element<Is, JsonMembers...>>(
		            is_first, it, args, value, visited_members ),
		          ... ),
		        0 );

		*it++ = '}';
		return it;
	}

	template<typename... JsonMembers, typename OutputIterator, typename Tuple,
	         typename Value, std::size_t... Is>
	[[nodiscard]] constexpr OutputIterator
	serialize_ordered_json_class( OutputIterator it, std::index_sequence<Is...>,
	                              Tuple const &args, Value const &value ) {

		*it++ = '[';
		size_t array_idx = 0;
		Unused( value );
		(void)std::array{
		  ( to_json_ordered_str<Is, daw::traits::nth_element<Is, JsonMembers...>>(
		      array_idx, it, args ),
		    0 )...};

		*it++ = ']';
		return it;
	}

	template<typename Range, typename... JsonMembers>
	static inline constexpr auto known_locations_v =
	  locations_info_t<sizeof...( JsonMembers ), Range>{
	    location_info_t<Range>( JsonMembers::name )...};

	template<typename JsonClass, typename... JsonMembers, std::size_t... Is,
	         typename Range>
	[[nodiscard]] constexpr JsonClass
	parse_json_class( Range &rng, std::index_sequence<Is...> ) {
		static_assert( has_json_data_contract_trait_v<JsonClass>,
		               "Unexpected type" );
		static_assert(
		  can_construct_a_v<JsonClass, typename JsonMembers::parse_to_t...>,
		  "Supplied types cannot be used for	construction of this type" );

		rng.trim_left( );
		daw_json_assert_weak( rng.front( '{' ), "Expected start of class" );
		rng.set_class_position( );
		rng.remove_prefix( );
		rng.move_to_next_of( "\"}" );

		auto const cleanup_fn = [&] {
			if( not rng.has_more( ) ) {
				return;
			}
			rng.clean_tail( );
			// If we fullfill the contract before all values are parses
			daw_json_assert_weak( rng.can_parse_more( ), "Unexpected end of range" );
			while( rng.front( ) != '}' ) {
				(void)parse_name( rng );
				(void)skip_value( rng );
				rng.clean_tail( );
				daw_json_assert_weak( rng.can_parse_more( ),
				                      "Unexpected end of range" );
			}

			daw_json_assert_weak( rng.front( ) == '}',
			                      "Expected class to end with '}'" );
			rng.remove_prefix( );
			rng.trim_left_checked( );
		};
		if constexpr( sizeof...( JsonMembers ) == 0 ) {
			cleanup_fn( );
			return daw::construct_a<JsonClass>( );
		} else {
			auto known_locations = known_locations_v<Range, JsonMembers...>;

			using tp_t = std::tuple<decltype(
			  parse_class_member<traits::nth_type<Is, JsonMembers...>>(
			    Is, known_locations, rng ) )...>;

#if defined( __cpp_constexpr_dynamic_alloc ) or                                \
  defined( DAW_JSON_NO_CONST_EXPR )
			// This relies on non-trivial dtor's being allowed.  So C++20 constexpr
			// or not in a constant expression.  It does allow for construction of
			// classes without move/copy special members

			// Do this before we exit but after return
			auto const oe = daw::on_exit_success( cleanup_fn );
			/*
			 * Rather than call directly use apply/tuple to evaluate left->right
			 */
			return std::apply(
			  daw::construct_a<JsonClass>,
			  tp_t{parse_class_member<traits::nth_type<Is, JsonMembers...>>(
			    Is, known_locations, rng )...} );
#else
			JsonClass result = std::apply(
			  daw::construct_a<JsonClass>,
			  tp_t{parse_class_member<traits::nth_type<Is, JsonMembers...>>(
			    Is, known_locations, rng )...} );
			cleanup_fn( );
			return result;
#endif
		}
	}

	template<typename JsonClass, typename... JsonMembers, typename Range>
	[[nodiscard]] constexpr JsonClass parse_ordered_json_class( Range &rng ) {
		static_assert( has_json_data_contract_trait_v<JsonClass>,
		               "Unexpected type" );
		static_assert(
		  can_construct_a_v<JsonClass, typename JsonMembers::parse_to_t...>,
		  "Supplied types cannot be used for	construction of this type" );

		rng.trim_left( );
		daw_json_assert_weak( rng.front( '[' ), "Expected start of array" );
		rng.set_class_position( );
		rng.remove_prefix( );
		rng.trim_left( );

		auto const cleanup_fn = [&] {
			rng.clean_tail( );
			daw_json_assert_weak( rng.can_parse_more( ), "Unexpected end of stream" );
			while( rng.front( ) != ']' ) {
				(void)skip_value( rng );
				rng.clean_tail( );
				daw_json_assert_weak( rng.can_parse_more( ),
				                      "Unexpected end of stream" );
			}
			daw_json_assert_weak( rng.front( ) == ']', "Expected a ']'" );
			rng.remove_prefix( );
			rng.trim_left( );
		};
		size_t current_idx = 0;
		using tp_t = std::tuple<decltype(
		  parse_ordered_class_member<JsonMembers>( current_idx, rng ) )...>;

#if defined( __cpp_constexpr_dynamic_alloc ) or                                \
  defined( DAW_JSON_NO_CONST_EXPR )
		auto const oe = daw::on_exit_success( cleanup_fn );
		return std::apply(
		  daw::construct_a<JsonClass>,
		  tp_t{parse_ordered_class_member<JsonMembers>( current_idx, rng )...} );
#else
		JsonClass result = std::apply(
		  daw::construct_a<JsonClass>,
		  tp_t{parse_ordered_class_member<JsonMembers>( current_idx, rng )...} );

		cleanup_fn( );
		return result;
#endif
	}
} // namespace daw::json::json_details
