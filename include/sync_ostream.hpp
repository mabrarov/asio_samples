//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SYNC_OSTREAM_HPP
#define SYNC_OSTREAM_HPP

#include <boost/thread.hpp>

namespace ma
{
  template <typename Ostream>
  class sync_ostream : private boost::noncopyable
  { 
  private:
    typedef sync_ostream<Ostream> this_type;

  public:
    typedef Ostream stream_type;

    sync_ostream(stream_type& stream)
      : stream_(stream)
      , mutex_()
    {
    }

    ~sync_ostream()
    {
    }

    template <typename Object>
    this_type& operator<<(const Object& object)
    {
      boost::mutex::scoped_lock lock(mutex_);
      stream_ << object;
      return (*this);
    }

  private:    
    stream_type& stream_;
    boost::mutex mutex_;
  };
} // namespace ma

#endif // SYNC_OSTREAM_HPP