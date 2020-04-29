// The MIT License (MIT)
//
// Copyright (c) 2019-2020 Darrell Wright
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

#include "daw_json_assert.h"

namespace daw::json::json_details::string_quote {

	struct string_quote_parser {
		[[nodiscard]] static constexpr char const *parse_nq( char const *ptr ) {
			while( *ptr != '"' ) {
				// TODO: add ability to filter to low 7bits daw_json_assert( *ptr >=
				// 0x20 and *ptr <= 0x7F, "Use json_string_raw" );
				while( *ptr != '"' and *ptr != '\\' ) {
					++ptr;
				}
				if( *ptr == '\\' ) {
					++ptr;
					++ptr;
				}
			}
			daw_json_assert( *ptr == '"', "Expected a '\"'" );
			return ptr;
		}

		[[nodiscard]] static constexpr char const *parse_nq( char const *first,
		                                                     char const *last ) {
			while( first != last and *first != '"' ) {
				// TODO: add ability to filter to low 7bits daw_json_assert( *ptr >=
				// 0x20 and *first <= 0x7F, "Use json_string_raw" );
				while( first != last and *first != '"' and *first != '\\' ) {
					++first;
				}
				if( first != last and *first == '\\' ) {
					++first;
					daw_json_assert( first != last, "Unexpected end of string" );
					++first;
				}
			}
			daw_json_assert( first != last and *first == '"', "Expected a '\"' at end of string" );
			return first;
		}

		[[nodiscard, maybe_unused]] static constexpr char const *
		parse( char const *ptr ) {
			if( *ptr == '"' ) {
				++ptr;
			}
			return parse_nq( ptr );
		}
	};

} // namespace daw::json::json_details::string_quote
