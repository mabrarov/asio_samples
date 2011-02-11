//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CODECVT_CAST_HPP
#define MA_CODECVT_CAST_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <locale>
#include <string>
#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace ma
{
  namespace codecvt_cast
  {
    class bad_conversion
      : public std::runtime_error
    {
    public:
      bad_conversion()
        : std::runtime_error(0)
      {
      }
    }; // class bad_conversion  

    template <typename CharType, typename Byte>
    const std::basic_string<CharType> in(
      const std::basic_string<Byte>& external_str, 
      const std::codecvt<CharType, Byte, mbstate_t>& codecvt)
    {
      typedef std::basic_string<CharType> wstring;
      typedef std::basic_string<Byte> string;
      typedef std::codecvt<CharType, Byte, mbstate_t> codecvt_type;

      wstring internal_str;

      string::size_type external_str_size = external_str.length();
      const Byte* first_external = external_str.data();
      const Byte* last_external  = first_external + external_str_size;
      const Byte* next_external  = last_external;

      codecvt_type::state_type state(0);
      wstring::size_type out_buf_size = codecvt.length(state, first_external, last_external, internal_str.max_size());
      internal_str.resize(out_buf_size);

      CharType* first_internal = const_cast<CharType*>(internal_str.data());
      CharType* last_internal  = first_internal + out_buf_size;
      CharType* next_internal  = first_internal;

      codecvt_type::result r = codecvt.in(state, 
        first_external, last_external, next_external,
        first_internal, last_internal, next_internal);
      
      if (codecvt_type::noconv == r)
      {
        internal_str.assign(reinterpret_cast<const CharType*>(first_external),
          reinterpret_cast<const CharType*>(last_external));                  
      }
      else if (codecvt_type::ok != r)
      {
        boost::throw_exception(bad_conversion());          
      }	    

	    return internal_str;
    } // in

    template <typename CharType, typename Byte>
    const std::basic_string<Byte> out(
      const std::basic_string<CharType>& internal_str, 
      const std::codecvt<CharType, Byte, mbstate_t>& codecvt)
    {
      typedef std::basic_string<CharType> wstring;
      typedef std::basic_string<Byte> string;
      typedef std::codecvt<CharType, Byte, mbstate_t> codecvt_type;

      string external_str;

      wstring::size_type internal_str_size = internal_str.length();
      wstring::size_type out_buf_size = codecvt.max_length() * internal_str_size;
      boost::scoped_array<Byte> out_buf(new Byte[out_buf_size]);

      const CharType* first_internal = internal_str.data();
      const CharType* last_internal  = first_internal + internal_str_size;
      const CharType* next_internal  = first_internal;

      Byte* first_external = out_buf.get();
      Byte* last_external  = first_external + out_buf_size;
      Byte* next_external  = first_external;

      codecvt_type::state_type state(0);

      codecvt_type::result r = codecvt.out(state, 
        first_internal, last_internal, next_internal,
        first_external, last_external, next_external);

      if (codecvt_type::ok == r)
      {
        //external_str.assign(first_external, next_external - first_external);		  
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
    } // out

  } // namespace codecvt_cast
} //namespace ma

#endif //MA_CODECVT_CAST_HPP
