//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_NMEA_FRAME_HPP
#define MA_NMEA_FRAME_HPP

#include <string>
#include <boost/smart_ptr.hpp>

namespace ma
{
  namespace nmea
  {     
    typedef std::string frame;
    typedef boost::shared_ptr<frame> frame_ptr;
    
  } // namespace nmea
} // namespace ma

#endif // MA_NMEA_FRAME_HPP