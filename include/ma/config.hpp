//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CONFIG_HPP
#define MA_CONFIG_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/config.hpp>

#if defined(BOOST_HAS_RVALUE_REFS)
#define MA_HAS_RVALUE_REFS
#else
#undef MA_HAS_RVALUE_REFS
#endif // defined(BOOST_HAS_RVALUE_REFS)

#endif // MA_CONFIG_HPP