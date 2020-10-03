// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#pragma once

#include "../daw_json_assert.h"
#include "daw_json_iterator_range.h"
#include "daw_json_parse_unsigned_int.h"

#include <daw/daw_cxmath.h>

#include <ciso646>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace daw::json::json_details {
	static inline constexpr double dpow10_tbl[] = {
	  1e0,   1e1,   1e2,   1e3,   1e4,   1e5,   1e6,   1e7,   1e8,   1e9,   1e10,
	  1e11,  1e12,  1e13,  1e14,  1e15,  1e16,  1e17,  1e18,  1e19,  1e20,  1e21,
	  1e22,  1e23,  1e24,  1e25,  1e26,  1e27,  1e28,  1e29,  1e30,  1e31,  1e32,
	  1e33,  1e34,  1e35,  1e36,  1e37,  1e38,  1e39,  1e40,  1e41,  1e42,  1e43,
	  1e44,  1e45,  1e46,  1e47,  1e48,  1e49,  1e50,  1e51,  1e52,  1e53,  1e54,
	  1e55,  1e56,  1e57,  1e58,  1e59,  1e60,  1e61,  1e62,  1e63,  1e64,  1e65,
	  1e66,  1e67,  1e68,  1e69,  1e70,  1e71,  1e72,  1e73,  1e74,  1e75,  1e76,
	  1e77,  1e78,  1e79,  1e80,  1e81,  1e82,  1e83,  1e84,  1e85,  1e86,  1e87,
	  1e88,  1e89,  1e90,  1e91,  1e92,  1e93,  1e94,  1e95,  1e96,  1e97,  1e98,
	  1e99,  1e100, 1e101, 1e102, 1e103, 1e104, 1e105, 1e106, 1e107, 1e108, 1e109,
	  1e110, 1e111, 1e112, 1e113, 1e114, 1e115, 1e116, 1e117, 1e118, 1e119, 1e120,
	  1e121, 1e122, 1e123, 1e124, 1e125, 1e126, 1e127, 1e128, 1e129, 1e130, 1e131,
	  1e132, 1e133, 1e134, 1e135, 1e136, 1e137, 1e138, 1e139, 1e140, 1e141, 1e142,
	  1e143, 1e144, 1e145, 1e146, 1e147, 1e148, 1e149, 1e150, 1e151, 1e152, 1e153,
	  1e154, 1e155, 1e156, 1e157, 1e158, 1e159, 1e160, 1e161, 1e162, 1e163, 1e164,
	  1e165, 1e166, 1e167, 1e168, 1e169, 1e170, 1e171, 1e172, 1e173, 1e174, 1e175,
	  1e176, 1e177, 1e178, 1e179, 1e180, 1e181, 1e182, 1e183, 1e184, 1e185, 1e186,
	  1e187, 1e188, 1e189, 1e190, 1e191, 1e192, 1e193, 1e194, 1e195, 1e196, 1e197,
	  1e198, 1e199, 1e200, 1e201, 1e202, 1e203, 1e204, 1e205, 1e206, 1e207, 1e208,
	  1e209, 1e210, 1e211, 1e212, 1e213, 1e214, 1e215, 1e216, 1e217, 1e218, 1e219,
	  1e220, 1e221, 1e222, 1e223, 1e224, 1e225, 1e226, 1e227, 1e228, 1e229, 1e230,
	  1e231, 1e232, 1e233, 1e234, 1e235, 1e236, 1e237, 1e238, 1e239, 1e240, 1e241,
	  1e242, 1e243, 1e244, 1e245, 1e246, 1e247, 1e248, 1e249, 1e250, 1e251, 1e252,
	  1e253, 1e254, 1e255, 1e256, 1e257, 1e258, 1e259, 1e260, 1e261, 1e262, 1e263,
	  1e264, 1e265, 1e266, 1e267, 1e268, 1e269, 1e270, 1e271, 1e272, 1e273, 1e274,
	  1e275, 1e276, 1e277, 1e278, 1e279, 1e280, 1e281, 1e282, 1e283, 1e284, 1e285,
	  1e286, 1e287, 1e288, 1e289, 1e290, 1e291, 1e292, 1e293, 1e294, 1e295, 1e296,
	  1e297, 1e298, 1e299, 1e300, 1e301, 1e302, 1e303, 1e304, 1e305, 1e306, 1e307,
	  1e308 };

	template<typename Result>
	constexpr Result power10( constexpr_exec_tag const &, double result,
	                          std::int32_t p ) {
		constexpr int max_exp = std::numeric_limits<double>::max_exponent10;
		constexpr double max_v = dpow10_tbl[max_exp];

		if( p > max_exp ) {
			do {
				result *= max_v;
				p -= max_exp;
			} while( p > max_exp );
		} else if( p < -max_exp ) {
			do {
				result /= max_v;
				p += max_exp;
			} while( p < -max_exp );
		}
		if( p < 0 ) {
			return static_cast<Result>( result / dpow10_tbl[-p] );
		}
		return static_cast<Result>( result * dpow10_tbl[p] );
	}

	template<typename Result>
	constexpr Result copy_sign( constexpr_exec_tag const &, Result to,
	                            Result from ) {
		return daw::cxmath::copy_sign( to, from );
	}
	template<typename Result>
	inline Result copy_sign( runtime_exec_tag const &, Result to, Result from ) {
		return std::copysign( to, from );
	}
	template<typename Real>
	struct real_part_result {
		Real result;
		int exponent;
	};

	template<typename Range>
	constexpr void parse_digits( Range &rng, std::uint64_t &v, int &c ) {

		char const * first = rng.first;
		char const * const last = rng.last;
		std::uint64_t value = v;
		int count = c;
		if( first >= last ) {
			return;
		}
		unsigned dig =
		  static_cast<unsigned>( *first ) - static_cast<unsigned>( '0' );
		while( ( first < last ) and ( ( dig < 10U ) & ( count >= 0 ) ) ) {
			value *= 10U;
			value += dig;
			++first;
			dig = static_cast<unsigned>( *first ) - static_cast<unsigned>( '0' );
		}
		while( ( first < last ) and ( dig < 10U ) ) {
			--count;
			++first;
			dig = static_cast<unsigned>( *first ) - static_cast<unsigned>( '0' );
		}
		v = value;
		c = count;
		rng.first = first;
	}

	template<typename Result, typename Range>
	static constexpr real_part_result<Result>
	parse_real_part( constexpr_exec_tag const &, Range &rng ) {
		constexpr int max_dig =
		  std::min( std::numeric_limits<std::uint64_t>::digits10,
		            std::numeric_limits<Result>::max_digits10 );
		int digits_avail = max_dig + 1;

		std::uint64_t value = 0;
		unsigned dig =
		  static_cast<unsigned>( rng.front( ) ) - static_cast<unsigned>( '0' );
		while( dig < 10U ) {
			if( digits_avail > 0 ) {
				value *= 10U;
				value += dig;
			}
			--digits_avail;
			rng.remove_prefix( );
			if constexpr( not Range::is_unchecked_input ) {
				if( not rng.has_more( ) ) {
					return { static_cast<Result>( value ), 0 };
				}
			}
			dig =
			  static_cast<unsigned>( rng.front( ) ) - static_cast<unsigned>( '0' );
		}
		if( rng.front( ) != '.' ) {
			return { static_cast<Result>( value ), 0 };
		}
		rng.remove_prefix( );
		int exponent = digits_avail >= 0 ? 0 : -digits_avail;
		dig = static_cast<unsigned>( rng.front( ) ) - static_cast<unsigned>( '0' );
		while( dig < 10U ) {
			if( digits_avail > 0 ) {
				value *= 10U;
				value += dig;
				--digits_avail;
				--exponent;
			}
			rng.remove_prefix( );
			if constexpr( not Range::is_unchecked_input ) {
				if( not rng.has_more( ) ) {
					return { static_cast<Result>( value ), exponent };
				}
			}
			dig =
			  static_cast<unsigned>( rng.front( ) ) - static_cast<unsigned>( '0' );
		}
		return { static_cast<Result>( value ), exponent };
	}

	template<typename Result, bool KnownRange, typename Range>
	[[nodiscard]] DAW_ATTRIBUTE_FLATTEN inline constexpr Result
	parse_real( Range &rng ) {
		// [-]WHOLE[.FRACTION][(e|E)[+|-]EXPONENT]
		// TODO: See if we can use the KnownRange parts to get better performance
		daw_json_assert_weak(
		  rng.has_more( ) and parse_policy_details::is_number_start( rng.front( ) ),
		  "Expected a real number", rng );

		Result const sign = [&] {
			if( rng.front( ) == '-' ) {
				rng.remove_prefix( );
				return static_cast<Result>( -1 );
			}
			return static_cast<Result>( 1 );
		}( );
		auto [digits, exp] = parse_real_part<Result>( Range::exec_tag, rng );
		if( rng.is_exponent_checked( ) ) {
			rng.remove_prefix( );
			int exp_tmp = [&] {
				switch( rng.front( ) ) {
				case '-':
					rng.remove_prefix( );
					return -1;
				case '+':
					rng.remove_prefix( );
					return 1;
				default:
					return 1;
				}
			}( );
			exp_tmp *= unsigned_parser<int, JsonRangeCheck::Never, false>(
			  Range::exec_tag, rng );
			exp += exp_tmp;
		}
		return power10<Result>( Range::exec_tag,
		                        static_cast<double>( sign * digits ), exp );
	}
} // namespace daw::json::json_details
