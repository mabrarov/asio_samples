//
// Copyright (c) 2020 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <locale>
#include <gtest/gtest.h>
#include <ma/codecvt_cast.hpp>

namespace ma {
namespace test {

typedef std::codecvt<wchar_t, char, std::mbstate_t> wide_codecvt_type;
typedef std::codecvt<char, char, std::mbstate_t> noconv_codecvt_type;

TEST(codecvt_cast_out, empty)
{
  const wide_codecvt_type& wide_codecvt =
      std::use_facet<wide_codecvt_type>(std::locale::classic());
  ASSERT_EQ("", codecvt_cast::out(std::wstring(), wide_codecvt));
}

TEST(codecvt_cast_out, latin)
{
  const wide_codecvt_type& wide_codecvt =
      std::use_facet<wide_codecvt_type>(std::locale::classic());
  ASSERT_EQ("ABc01+)",
      codecvt_cast::out(std::wstring(L"ABc01+)"), wide_codecvt));
}

TEST(codecvt_cast_out, noconv)
{
  const noconv_codecvt_type& noconv_codecvt =
      std::use_facet<noconv_codecvt_type>(std::locale::classic());
  ASSERT_EQ("test", codecvt_cast::out(std::string("test"), noconv_codecvt));
}

TEST(codecvt_cast_out, bad_cast)
{
  const wide_codecvt_type& wide_codecvt =
      std::use_facet<wide_codecvt_type>(std::locale::classic());
  ASSERT_THROW(codecvt_cast::out(std::wstring(L"\u20B5"), wide_codecvt),
      codecvt_cast::bad_conversion);
}

TEST(codecvt_cast_in, empty)
{
  const wide_codecvt_type& wide_codecvt =
      std::use_facet<wide_codecvt_type>(std::locale::classic());
  ASSERT_EQ(L"", codecvt_cast::in(std::string(), wide_codecvt));
}

TEST(codecvt_cast_in, latin)
{
  const wide_codecvt_type& wide_codecvt =
      std::use_facet<wide_codecvt_type>(std::locale::classic());
  ASSERT_EQ(L"ABc01+)", codecvt_cast::in(std::string("ABc01+)"), wide_codecvt));
}

TEST(codecvt_cast_in, noconv)
{
  const noconv_codecvt_type& noconv_codecvt =
      std::use_facet<noconv_codecvt_type>(std::locale::classic());
  ASSERT_EQ("test", codecvt_cast::in(std::string("test"), noconv_codecvt));
}

} // namespace test
} // namespace ma
