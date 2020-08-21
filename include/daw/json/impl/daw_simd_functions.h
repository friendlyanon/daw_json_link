// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#pragma once

#include "daw_simd_modes.h"

#if defined( DAW_ALLOW_SSE3 )
#include <emmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>
#include <xmmintrin.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace daw::json {
	static inline char const *sse3_skip_string( char const *first,
	                                            char const *const last ) {
		alignas( 16 ) static constexpr char const quote_str[] =
		  R"("""""""""""""""")";
		alignas( 16 ) static constexpr char const back_slash_str[] =
		  R"(\\\\\\\\\\\\\\\\)";
		__m128i const quotes =
		  _mm_load_si128( reinterpret_cast<__m128i const *>( quote_str ) );
		__m128i const back_slash =
		  _mm_load_si128( reinterpret_cast<__m128i const *>( back_slash_str ) );
		while( last - first >= 16U ) {
			__m128i const cur_set =
			  _mm_loadu_si128( reinterpret_cast<__m128i const *>( first ) );
			__m128i const has_quotes = _mm_cmpeq_epi8( cur_set, quotes );
			__m128i const has_backslashes = _mm_cmpeq_epi8( cur_set, back_slash );
			__m128i const has_either = _mm_or_si128( has_quotes, has_backslashes );
			int const any_set = _mm_movemask_epi8( has_either );
			if( any_set != 0 ) {
				// Find out which is the first character
#if defined( __GNUC__ ) or defined( __clang__ )
				first += __builtin_ffs( any_set ) - 1;
#elif defined( _MSC_VER )
				unsigned long idx;
				_BitScanForward( &idx, any_set );
				first += idx;
#endif
				break;
			}
			first += 16;
		}
		return first;
	}

	template<typename Range>
	static inline void sse3_skip_ws( Range &rng ) {
		char const *first = rng.first;
		char const *const last = rng.last;
		alignas( 16 ) static constexpr char const whitespace_str[] =
		  "                ";
		__m128i const whitespace =
		  _mm_loadu_si128( reinterpret_cast<__m128i const *>( whitespace_str ) );

		alignas( 16 ) static constexpr char const ones_str[] =
		  "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
		__m128i const ones =
		  _mm_loadu_si128( reinterpret_cast<__m128i const *>( ones_str ) );
		while( last - first >= 16U ) {
			__m128i const cur_set =
			  _mm_loadu_si128( reinterpret_cast<__m128i const *>( first ) );
			__m128i const is_ws = _mm_max_epu8( cur_set, whitespace );
			__m128i const has_ws = _mm_cmpeq_epi8( is_ws, whitespace );
			__m128i const not_ws = _mm_xor_si128( has_ws, ones );
			int const any_set = _mm_movemask_epi8( not_ws );
			if( any_set != 0 ) {
				// Find out which is the first character
#if defined( __GNUC__ ) or defined( __clang__ )
				first += __builtin_ffs( any_set ) - 1;
#elif defined( _MSC_VER )
				unsigned long idx;
				_BitScanForward( &idx, any_set );
				first += idx;
#endif
				break;
			}
			first += 16;
		}
		if constexpr( Range::is_unchecked_input ) {
			while( *first <= 0x20 ) {
				++first;
			}
		} else {
			while( first != last and *first <= 0x20 ) {
				++first;
			}
		}
		rng.first = first;
	}
} // namespace daw::json
#endif