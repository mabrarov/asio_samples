//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CODECVT_CAST_HPP
#define MA_CODECVT_CAST_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstring>
#include <locale>
#include <string>
#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <ma/detail/memory.hpp>

namespace ma {
namespace codecvt_cast {

class bad_conversion : public std::runtime_error
{
public:
  bad_conversion()
    : std::runtime_error(std::string())
  {
  }
}; // class bad_conversion

template <typename CharType, typename Byte>
std::basic_string<CharType> in(
    const std::basic_string<Byte>& external_str,
    const std::codecvt<CharType, Byte, std::mbstate_t>& codecvt)
{
  typedef std::basic_string<CharType> wstring;
  typedef std::basic_string<Byte>     string;
  typedef typename wstring::size_type wstring_size_type;
  typedef typename string::size_type  string_size_type;
  typedef std::codecvt<CharType, Byte, std::mbstate_t> codecvt_type;
  typedef typename codecvt_type::state_type codecvt_state_type;

  string_size_type external_len = external_str.length();
  const Byte* first_external = external_str.data();
  const Byte* last_external  = first_external + external_len;
  const Byte* next_external  = last_external;

  wstring internal_str;

  codecvt_state_type len_state = codecvt_state_type();
  wstring_size_type out_buf_size = static_cast<wstring_size_type>(
      codecvt.length(len_state, first_external, last_external, external_len));

#if defined(MA_USE_CXX11_STDLIB_MEMORY)
  detail::unique_ptr<CharType[]> out_buf(new CharType[out_buf_size]);
#else
  detail::scoped_array<CharType> out_buf(new CharType[out_buf_size]);
#endif

  CharType* first_internal = out_buf.get();
  CharType* last_internal  = first_internal + out_buf_size;
  CharType* next_internal  = first_internal;

  codecvt_state_type conv_state = codecvt_state_type();
  typename codecvt_type::result r = codecvt.in(conv_state,
      first_external, last_external, next_external,
      first_internal, last_internal, next_internal);

  if (codecvt_type::ok == r)
  {
    internal_str.assign(first_internal, next_internal);
  }
  else if (codecvt_type::noconv == r)
  {
    internal_str.assign(reinterpret_cast<const CharType*>(first_external),
        reinterpret_cast<const CharType*>(last_external));
  }
  else
  {
    boost::throw_exception(bad_conversion());
  }

  return internal_str;
}

template <typename CharType, typename Byte>
std::basic_string<Byte> out(
    const std::basic_string<CharType>& internal_str,
    const std::codecvt<CharType, Byte, std::mbstate_t>& codecvt)
{
  typedef std::basic_string<CharType> wstring;
  typedef std::basic_string<Byte>     string;
  typedef typename wstring::size_type wstring_size_type;
  typedef typename string::size_type  string_size_type;
  typedef std::codecvt<CharType, Byte, std::mbstate_t> codecvt_type;
  typedef typename codecvt_type::state_type codecvt_state_type;

  string_size_type internal_char_max_len =
      static_cast<string_size_type>(codecvt.max_length());
  wstring_size_type internal_len = internal_str.length();
  string external_str;
  string_size_type external_max_len = external_str.max_size();
  string_size_type out_buf_size =
      internal_len > external_max_len / internal_char_max_len
          ? external_max_len
          : static_cast<string_size_type>(internal_len * internal_char_max_len);

#if defined(MA_USE_CXX11_STDLIB_MEMORY)
  detail::unique_ptr<Byte[]> out_buf(new Byte[out_buf_size]);
#else
  detail::scoped_array<Byte> out_buf(new Byte[out_buf_size]);
#endif

  const CharType* first_internal = internal_str.data();
  const CharType* last_internal  = first_internal + internal_len;
  const CharType* next_internal  = first_internal;

  Byte* first_external = out_buf.get();
  Byte* last_external  = first_external + out_buf_size;
  Byte* next_external  = first_external;

  codecvt_state_type state = codecvt_state_type();
  typename codecvt_type::result r = codecvt.out(state,
      first_internal, last_internal, next_internal,
      first_external, last_external, next_external);

  if (codecvt_type::ok == r)
  {
    external_str.assign(first_external, next_external);
  }
  else if (codecvt_type::noconv == r)
  {
    external_str.assign(reinterpret_cast<const Byte*>(first_internal),
        reinterpret_cast<const Byte*>(last_internal));
  }
  else
  {
    boost::throw_exception(bad_conversion());
  }

  return external_str;
}

} // namespace codecvt_cast
} // namespace ma

#endif // MA_CODECVT_CAST_HPP
