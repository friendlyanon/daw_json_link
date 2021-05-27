// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#pragma once

#include "daw_json_assert.h"
#include "daw_json_enums.h"
#include "daw_json_exec_modes.h"
#include "daw_json_name.h"
#include "daw_json_traits.h"
#include "version.h"

#include <daw/daw_arith_traits.h>
#include <daw/daw_cpp_feature_check.h>
#include <daw/daw_move.h>
#include <daw/daw_parser_helper_sv.h>
#include <daw/daw_string_view.h>
#include <daw/daw_traits.h>

#include <ciso646>
#include <cstddef>
#include <iterator>

#if defined( __cpp_constexpr_dynamic_alloc )
#define CPP20CONSTEXPR constexpr
#else
#define CPP20CONSTEXPR
#endif

namespace daw::json {
	inline namespace DAW_JSON_VER {
		/***
		 *
		 * Allows specifying an unnamed json mapping where the
		 * result is a tuple
		 */
		template<typename... Members>
		struct tuple_json_mapping {
			std::tuple<typename Members::parse_to_t...> members;

			template<typename... Ts>
			constexpr tuple_json_mapping( Ts &&...values )
			  : members{ DAW_FWD( values )... } {}
		};

		namespace json_details {
			template<typename T>
			using ordered_member_subtype_test = typename T::json_member;

			template<typename T>
			using ordered_member_subtype_t =
			  typename daw::detected_or_t<T, ordered_member_subtype_test, T>;

			template<typename T>
			inline constexpr auto json_class_constructor =
			  json_class_constructor_t<T>{ };

			template<typename Value, typename Constructor, typename ParseState,
			         typename... Args>
			DAW_ATTRIB_FLATINLINE static inline constexpr auto
			construct_value( template_params<Value, Constructor>,
			                 ParseState &parse_state, Args &&...args ) {
				if constexpr( ParseState::has_allocator ) {
					using alloc_t =
					  typename ParseState::template allocator_type_as<Value>;
					auto alloc = parse_state.get_allocator_for( template_arg<Value> );
					if constexpr( std::is_invocable<Constructor, Args...,
					                                alloc_t>::value ) {
						return Constructor{ }( DAW_FWD( args )..., DAW_MOVE( alloc ) );
					} else if constexpr( daw::traits::is_callable_v<Constructor,
					                                                std::allocator_arg_t,
					                                                alloc_t, Args...> ) {
						return Constructor{ }( std::allocator_arg, DAW_MOVE( alloc ),
						                       DAW_FWD( args )... );
					} else {
						static_assert( std::is_invocable<Constructor, Args...>::value );
						return Constructor{ }( DAW_FWD( args )... );
					}
				} else {
					static_assert( std::is_invocable<Constructor, Args...>::value );
					if constexpr( std::is_invocable<Constructor, Args...>::value ) {
						return Constructor{ }( DAW_FWD( args )... );
					}
				}
			}

			struct construct_value_tp_invoke_t {
				template<typename Func, typename... TArgs, std::size_t... Is>
				DAW_ATTRIB_FLATINLINE inline constexpr decltype( auto )
				operator( )( Func &&f, std::tuple<TArgs...> &&tp,
				             std::index_sequence<Is...> ) const {
					return DAW_FWD( f )( DAW_MOVE( std::get<Is>( tp ) )... );
				}

				template<typename Func, typename... TArgs, typename Allocator,
				         std::size_t... Is>
				DAW_ATTRIB_FLATINLINE inline constexpr decltype( auto )
				operator( )( Func &&f, std::tuple<TArgs...> &&tp, Allocator &alloc,
				             std::index_sequence<Is...> ) const {
					return DAW_FWD( f )( DAW_MOVE( std::get<Is>( tp ) )...,
					                     DAW_FWD( alloc ) );
				}

				template<typename Alloc, typename Func, typename... TArgs,
				         std::size_t... Is>
				DAW_ATTRIB_FLATINLINE inline constexpr decltype( auto )
				operator( )( Func &&f, std::allocator_arg_t, Alloc &&alloc,
				             std::tuple<TArgs...> &&tp,
				             std::index_sequence<Is...> ) const {
					return DAW_FWD( f )( std::allocator_arg, DAW_FWD( alloc ),
					                     DAW_MOVE( std::get<Is>( tp ) )... );
				}
			};

			inline constexpr construct_value_tp_invoke_t construct_value_tp_invoke =
			  construct_value_tp_invoke_t{ };

			template<typename Value, typename Constructor, typename ParseState,
			         typename... Args>
			DAW_ATTRIB_FLATINLINE static inline constexpr auto
			construct_value_tp( Constructor &&ctor, ParseState &parse_state,
			                    std::tuple<Args...> &&tp_args ) {
				if constexpr( ParseState::has_allocator ) {
					using alloc_t =
					  typename ParseState::template allocator_type_as<Value>;
					auto alloc = parse_state.get_allocator_for( template_arg<Value> );
					if constexpr( std::is_invocable<Constructor, Args...,
					                                alloc_t>::value ) {
						return construct_value_tp_invoke(
						  ctor, DAW_MOVE( tp_args ), DAW_MOVE( alloc ),
						  std::index_sequence_for<Args...>{ } );
					} else if constexpr( daw::traits::is_callable_v<Constructor,
					                                                std::allocator_arg_t,
					                                                alloc_t, Args...> ) {
						return construct_value_tp_invoke(
						  ctor, std::allocator_arg, DAW_MOVE( alloc ), DAW_MOVE( tp_args ),
						  std::index_sequence_for<Args...>{ } );
					} else {
						static_assert( std::is_invocable<Constructor, Args...>::value );
						return construct_value_tp_invoke(
						  ctor, DAW_MOVE( tp_args ), std::index_sequence_for<Args...>{ } );
					}
				} else {
					static_assert( std::is_invocable<Constructor, Args...>::value );
					return construct_value_tp_invoke(
					  ctor, DAW_MOVE( tp_args ), std::index_sequence_for<Args...>{ } );
				}
			}

			template<typename Allocator>
			using has_allocate_test =
			  decltype( std::declval<Allocator &>( ).allocate( size_t{ 1 } ) );

			template<typename Allocator>
			using has_deallocate_test =
			  decltype( std::declval<Allocator &>( ).deallocate(
			    static_cast<void *>( nullptr ), size_t{ 1 } ) );

			template<typename Allocator>
			inline constexpr bool is_allocator_v = std::conjunction<
			  daw::is_detected<has_allocate_test, Allocator>,
			  daw::is_detected<has_deallocate_test, Allocator>>::value;

			template<typename T>
			inline constexpr bool has_json_data_contract_trait_v =
			  not std::is_same_v<missing_json_data_contract_for<T>,
			                     json_data_contract_trait_t<T>>;

			template<typename Container, typename Value>
			using detect_push_back = decltype( std::declval<Container &>( ).push_back(
			  std::declval<Value>( ) ) );

			template<typename Container, typename Value>
			using detect_insert_end = decltype( std::declval<Container &>( ).insert(
			  std::end( std::declval<Container &>( ) ), std::declval<Value>( ) ) );

			template<typename Container, typename Value>
			inline constexpr bool has_push_back_v =
			  daw::is_detected<detect_push_back, Container, Value>::value;

			template<typename Container, typename Value>
			inline constexpr bool has_insert_end_v =
			  daw::is_detected<detect_insert_end, Container, Value>::value;

			template<typename JsonMember>
			using json_result = typename JsonMember::parse_to_t;

			template<typename JsonMember>
			using json_base_type = typename JsonMember::base_type;

			template<typename Container>
			struct basic_appender {
				using value_type = typename Container::value_type;
				using reference = value_type;
				using pointer = value_type const *;
				using difference_type = std::ptrdiff_t;
				using iterator_category = std::output_iterator_tag;
				Container *m_container;

				explicit inline constexpr basic_appender( Container &container )
				  : m_container( &container ) {}

				template<typename Value>
				DAW_ATTRIB_FLATINLINE inline constexpr void
				operator( )( Value &&value ) const {
					if constexpr( has_push_back_v<Container,
					                              daw::remove_cvref_t<Value>> ) {
						m_container->push_back( DAW_FWD( value ) );
					} else if constexpr( has_insert_end_v<Container,
					                                      daw::remove_cvref_t<Value>> ) {
						m_container->insert( std::end( *m_container ), DAW_FWD( value ) );
					} else {
						static_assert(
						  has_push_back_v<Container, daw::remove_cvref_t<Value>> or
						    has_insert_end_v<Container, daw::remove_cvref_t<Value>>,
						  "basic_appender requires a Container that either has push_back "
						  "or "
						  "insert with the end iterator as first argument" );
					}
				}

				template<
				  typename Value,
				  std::enable_if_t<
				    not std::is_same<basic_appender, daw::remove_cvref_t<Value>>::value,
				    std::nullptr_t> = nullptr>
				inline constexpr basic_appender &operator=( Value &&v ) {
					operator( )( DAW_FWD( v ) );
					return *this;
				}

				inline constexpr basic_appender &operator++( ) {
					return *this;
				}

				inline constexpr basic_appender operator++( int ) const {
					return *this;
				}

				inline constexpr basic_appender &operator*( ) {
					return *this;
				}
			};

			template<typename T>
			using json_parser_to_json_data_t =
			  decltype( json_data_contract<T>::to_json_data( std::declval<T &>( ) ) );

			template<typename T>
			inline constexpr bool has_json_to_json_data_v =
			  daw::is_detected<json_parser_to_json_data_t, T>::value;

			template<typename T>
			using is_submember_tagged_variant_t =
			  typename json_data_contract<T>::type::i_am_a_submember_tagged_variant;

			template<typename T>
			inline constexpr bool is_submember_tagged_variant_v =
			  daw::is_detected<is_submember_tagged_variant_t, T>::value;

			template<typename>
			inline constexpr std::size_t parse_space_needed_v = 1U;

			template<>
			inline constexpr std::size_t parse_space_needed_v<simd_exec_tag> = 16U;

			template<typename JsonType>
			using is_json_nullable =
			  std::bool_constant<JsonType::expected_type == JsonParseTypes::Null>;

			template<typename JsonType>
			static inline constexpr bool is_json_nullable_v =
			  is_json_nullable<JsonType>::value;

			template<typename T>
			using dereffed_type_impl =
			  daw::remove_cvref_t<decltype( *( std::declval<T &>( ) ) )>;

			template<typename T>
			using dereffed_type =
			  typename daw::detected_or<T, dereffed_type_impl, T>::type;

			template<typename T, JsonNullable Nullable>
			using unwrap_type =
			  typename std::conditional_t<Nullable == JsonNullable::Nullable,
			                              dereffed_type<T>, T>;
		} // namespace json_details

		/**
		 * Link to a JSON class
		 * @tparam Name name of JSON member to link to
		 * @tparam T type that has specialization of
		 * daw::json::json_data_contract
		 * @tparam Constructor A callable used to construct T.  The
		 * default supports normal and aggregate construction
		 * @tparam Nullable Can the member be missing or have a null value
		 */
		template<JSONNAMETYPE Name, typename T,
		         typename Constructor = default_constructor<T>,
		         JsonNullable Nullable = JsonNullable::Never>
		struct json_class;

		/**
		 * The member is a number
		 * @tparam Name name of json member
		 * @tparam T type of number(e.g. double, int, unsigned...) to pass to
		 * Constructor
		 * @tparam LiteralAsString Could this number be embedded in a string
		 * @tparam Constructor Callable used to construct result
		 * @tparam RangeCheck Check if the value will fit in the result
		 * @tparam Nullable Can the member be missing or have a null value
		 */
		template<JSONNAMETYPE Name, typename T = double,
		         LiteralAsStringOpt LiteralAsString = LiteralAsStringOpt::Never,
		         typename Constructor = default_constructor<T>,
		         JsonRangeCheck RangeCheck = JsonRangeCheck::Never,
		         JsonNullable Nullable = JsonNullable::Never>
		struct json_number;

		/**
		 * The member is a boolean
		 * @tparam Name name of json member
		 * @tparam T result type to pass to Constructor
		 * @tparam LiteralAsString Could this number be embedded in a string
		 * @tparam Constructor Callable used to construct result
		 * @tparam Nullable Can the member be missing or have a null value
		 */
		template<JSONNAMETYPE Name, typename T = bool,
		         LiteralAsStringOpt LiteralAsString = LiteralAsStringOpt::Never,
		         typename Constructor = default_constructor<T>,
		         JsonNullable Nullable = JsonNullable::Never>
		struct json_bool;

		/**
		 * Member is an escaped string and requires unescaping and escaping of
		 * string data
		 * @tparam Name of json member
		 * @tparam String result type constructed by Constructor
		 * @tparam Constructor a callable taking as arguments ( char const *,
		 * std::size_t )
		 * @tparam EmptyStringNull if string is empty, call Constructor with no
		 * arguments
		 * @tparam EightBitMode Allow filtering of characters with the MSB set
		 * @tparam Nullable Can the member be missing or have a null value
		 */
		template<JSONNAMETYPE Name, typename String = std::string,
		         typename Constructor = default_constructor<String>,
		         JsonNullable EmptyStringNull = JsonNullable::Never,
		         EightBitModes EightBitMode = EightBitModes::AllowFull,
		         JsonNullable Nullable = JsonNullable::Never>
		struct json_string;

		/**
		 * Member is an escaped string and requires unescaping and escaping of
		 * string data
		 * @tparam Name of json member
		 * @tparam String result type constructed by Constructor
		 * @tparam Constructor a callable taking as arguments ( char const *,
		 * std::size_t )
		 * @tparam EmptyStringNull if string is empty, call Constructor with no
		 * arguments
		 * @tparam EightBitMode Allow filtering of characters with the MSB set
		 * arguments
		 * @tparam Nullable Can the member be missing or have a null value
		 * @tparam AllowEscape Tell parser if we know a \ or escape will be in the
		 * data
		 */
		template<JSONNAMETYPE Name, typename String = std::string,
		         typename Constructor = default_constructor<String>,
		         JsonNullable EmptyStringNull = JsonNullable::Never,
		         EightBitModes EightBitMode = EightBitModes::AllowFull,
		         JsonNullable Nullable = JsonNullable::Never,
		         AllowEscapeCharacter AllowEscape = AllowEscapeCharacter::Allow>
		struct json_string_raw;

		namespace json_details {
			template<typename T>
			using can_deref =
			  typename std::bool_constant<daw::is_detected_v<dereffed_type_impl, T>>;

			template<typename T>
			using cant_deref = daw::not_trait<can_deref<T>>;

			template<typename T>
			inline constexpr bool can_deref_v = can_deref<T>::value;

			template<typename T>
			inline constexpr JsonParseTypes number_parse_type_impl_v = [] {
				static_assert( daw::is_arithmetic<T>::value, "Unexpected non-number" );
				if constexpr( daw::is_floating_point<T>::value ) {
					return JsonParseTypes::Real;
				} else if constexpr( daw::is_signed<T>::value ) {
					return JsonParseTypes::Signed;
				} else if constexpr( daw::is_unsigned<T>::value ) {
					return JsonParseTypes::Unsigned;
				}
			}( );

			template<typename T>
			constexpr auto number_parse_type_test( )
			  -> std::enable_if_t<std::is_enum<T>::value, JsonParseTypes> {

				return number_parse_type_impl_v<std::underlying_type_t<T>>;
			}
			template<typename T>
			constexpr auto number_parse_type_test( )
			  -> std::enable_if_t<not std::is_enum<T>::value, JsonParseTypes> {

				return number_parse_type_impl_v<T>;
			}

			template<typename T>
			inline constexpr JsonParseTypes
			  number_parse_type_v = number_parse_type_test<T>( );
		} // namespace json_details

		template<typename>
		struct json_link_basic_type_map {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::Unknown;
		};

		template<>
		struct json_link_basic_type_map<daw::string_view> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::StringRaw;
		};

		template<>
		struct json_link_basic_type_map<std::string_view> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::StringRaw;
		};

		template<>
		struct json_link_basic_type_map<std::string> {
			constexpr static JsonParseTypes parse_type =
			  JsonParseTypes::StringEscaped;
		};

		template<>
		struct json_link_basic_type_map<bool> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::Bool;
		};

		template<>
		struct json_link_basic_type_map<short> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::Signed;
		};

		template<>
		struct json_link_basic_type_map<int> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::Signed;
		};

		template<>
		struct json_link_basic_type_map<long> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::Signed;
		};

		template<>
		struct json_link_basic_type_map<long long> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::Signed;
		};

		template<>
		struct json_link_basic_type_map<unsigned short> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::Unsigned;
		};

		template<>
		struct json_link_basic_type_map<unsigned int> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::Unsigned;
		};

		template<>
		struct json_link_basic_type_map<unsigned long> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::Unsigned;
		};

		template<>
		struct json_link_basic_type_map<unsigned long long> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::Unsigned;
		};

		template<>
		struct json_link_basic_type_map<float> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::Real;
		};

		template<>
		struct json_link_basic_type_map<double> {
			constexpr static JsonParseTypes parse_type = JsonParseTypes::Real;
		};

		namespace json_details {
			template<typename T>
			inline constexpr bool has_basic_type_map_v =
			  daw::is_detected_v<json_link_basic_type_map, T>;

			template<typename Mapped, bool Found = true>
			struct json_link_quick_map_type : std::bool_constant<Found> {
				using mapped_type = Mapped;
			};

			template<typename T>
			inline constexpr auto json_link_quick_map( ) {
				if constexpr( has_basic_type_map_v<T> ) {
					constexpr auto mapped_type = json_link_basic_type_map<T>::parse_type;
					if constexpr( mapped_type == JsonParseTypes::StringRaw ) {
						return json_link_quick_map_type<json_string_raw<no_name, T>>{ };
					} else if constexpr( mapped_type == JsonParseTypes::StringEscaped ) {
						return json_link_quick_map_type<json_string<no_name, T>>{ };
					} else if constexpr( mapped_type == JsonParseTypes::Bool ) {
						return json_link_quick_map_type<json_bool<no_name, T>>{ };
					} else if constexpr( mapped_type == JsonParseTypes::Signed ) {
						return json_link_quick_map_type<json_number<no_name, T>>{ };
					} else if constexpr( mapped_type == JsonParseTypes::Unsigned ) {
						return json_link_quick_map_type<json_number<no_name, T>>{ };
					} else if constexpr( mapped_type == JsonParseTypes::Real ) {
						return json_link_quick_map_type<json_number<no_name, T>>{ };
					} else {
						return json_link_quick_map_type<void, false>{ };
					}
				} else {
					return json_link_quick_map_type<void, false>{ };
				}
			}
			/***
			 * Check if the current type has a quick map specialized for it
			 */
			template<typename T>
			inline constexpr bool has_json_link_quick_map_v =
			  decltype( json_link_quick_map<T>( ) )::value;

			/***
			 * Get the quick mapped json type for type T
			 */
			template<typename T>
			using json_link_quick_map_t =
			  typename decltype( json_link_quick_map<T>( ) )::mapped_type;

			namespace vector_detect {
				template<typename T>
				struct vector_test {
					struct not_vector {};
					static constexpr bool value = false;
					using type = not_vector;
				};

				template<typename T, typename Alloc>
				struct vector_test<std::vector<T, Alloc>> {
					static constexpr bool value = true;
					using type = T;
				};

				template<typename T>
				using vector_test_t = typename vector_test<T>::type;
			} // namespace vector_detect

			template<typename T>
			using is_vector =
			  std::bool_constant<vector_detect::vector_test<T>::value>;
			/** Link to a JSON array that has been detected
			 * @tparam Name name of JSON member to link to
			 * @tparam Container type of C++ container being constructed(e.g.
			 * vector<int>)
			 * @tparam JsonElement Json type being parsed e.g. json_number,
			 * json_string...
			 * @tparam Constructor A callable used to make Container,
			 * default will use the Containers constructor.  Both normal and aggregate
			 * are supported
			 * @tparam Nullable Can the member be missing or have a null value
			 */
			template<JSONNAMETYPE Name, typename JsonElement, typename Container,
			         typename Constructor = default_constructor<Container>,
			         JsonNullable Nullable = JsonNullable::Never>
			struct json_array_detect;

			template<typename JsonType>
			using is_json_class_map_test = typename JsonType::json_member;

			template<typename JsonType>
			struct json_class_map_type {
				using type = typename JsonType::json_member;
			};

			template<typename JsonType>
			inline constexpr bool is_json_class_map_v =
			  daw::is_detected_v<is_json_class_map_test, JsonType>;

			template<typename T, bool Contract, bool JsonType, bool QuickMap,
			         bool IsNumeric, bool IsVector>
			struct unnamed_default_type_mapping_test {
				using type = missing_json_data_contract_for<T>;
			};

			template<typename T, /* bool Contract, */ bool JsonType, bool QuickMap,
			         bool IsNumeric, bool IsVector>
			struct unnamed_default_type_mapping_test<T, true, JsonType, QuickMap,
			                                         IsNumeric, IsVector> {

				using type = json_class<no_name, T, json_class_constructor_t<T>>;

				static_assert(
				  not std::is_same_v<daw::remove_cvref_t<type>, daw::nonesuch> );
			};

			template<typename T, /* bool Contract, bool JsonType, */ bool QuickMap,
			         bool IsNumeric, bool IsVector>
			struct unnamed_default_type_mapping_test<T, false, true, QuickMap,
			                                         IsNumeric, IsVector> {

				using type =
				  typename std::conditional_t<is_json_class_map_v<T>,
				                              json_class_map_type<T>,
				                              daw::traits::identity<T>>::type;

				static_assert(
				  not std::is_same_v<daw::remove_cvref_t<type>, daw::nonesuch> );
			};

			template<typename T, /* bool Contract, bool JsonType, bool QuickMap, */
			         bool IsNumeric, bool IsVector>
			struct unnamed_default_type_mapping_test<T, false, false, true, IsNumeric,
			                                         IsVector> {
				using type = json_link_quick_map_t<T>;

				static_assert(
				  not std::is_same_v<daw::remove_cvref_t<type>, daw::nonesuch> );
			};

			template<typename T, /* bool Contract, bool JsonType, bool QuickMap,
			  bool IsNumeric, */
			         bool IsVector>
			struct unnamed_default_type_mapping_test<T, false, false, false, true,
			                                         IsVector> {
				using type = json_number<no_name, T>;

				static_assert(
				  not std::is_same_v<daw::remove_cvref_t<type>, daw::nonesuch> );
			};

			template<typename T /*, bool JsonType, bool Contract, bool QuickMap,
			  bool IsNumeric, bool IsVector*/ >
			struct unnamed_default_type_mapping_test<T, false, false, false, false,
			                                         true> {
				using type =
				  json_array_detect<no_name, vector_detect::vector_test_t<T>, T>;
				static_assert(
				  not std::is_same_v<daw::remove_cvref_t<type>, daw::nonesuch> );
			};

			template<typename T>
			using unnamed_default_type_mapping =
			  typename unnamed_default_type_mapping_test<
			    T, has_json_data_contract_trait_v<T>,
			    json_details::is_a_json_type_v<T>, has_json_link_quick_map_v<T>,
			    std::disjunction_v<daw::is_arithmetic<T>, std::is_enum<T>>,
			    is_vector<T>::value>::type;

			template<typename T>
			using has_unnamed_default_type_mapping =
			  daw::not_trait<std::is_same<unnamed_default_type_mapping<T>,
			                              missing_json_data_contract_for<T>>>;

			template<typename T>
			inline constexpr bool has_unnamed_default_type_mapping_v =
			  has_unnamed_default_type_mapping<T>::value;

			template<typename JsonMember>
			using from_json_result_t =
			  json_result<unnamed_default_type_mapping<JsonMember>>;

			template<typename Constructor, typename... Members>
			using json_class_parse_result_impl2 = decltype(
			  Constructor{ }( std::declval<typename Members::parse_to_t &&>( )... ) );

			template<typename Constructor, typename... Members>
			using json_class_parse_result_impl =
			  daw::detected_t<json_class_parse_result_impl2, Constructor, Members...>;

			template<typename Constructor, typename... Members>
			using json_class_parse_result_t = daw::remove_cvref_t<
			  json_class_parse_result_impl<Constructor, Members...>>;

			template<typename Constructor, typename... Members>
			inline constexpr bool can_defer_construction_v =
			  std::is_invocable_v<Constructor, typename Members::parse_to_t...>;
		} // namespace json_details

		template<typename T>
		struct TestInputIteratorType {
			using iterator_category = std::input_iterator_tag;
			using value_type = T;
			using reference = T &;
			using pointer = T *;
			using difference_type = std::ptrdiff_t;
		};
		static_assert(
		  std::is_same<
		    typename std::iterator_traits<TestInputIteratorType<int>>::value_type,
		    int>::value );
	} // namespace DAW_JSON_VER
} // namespace daw::json
