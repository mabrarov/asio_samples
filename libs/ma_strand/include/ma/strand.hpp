//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_STRAND_HPP
#define MA_STRAND_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <ma/config.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/detail/type_traits.hpp>
#include <ma/detail/utility.hpp>

namespace ma {

#if defined(MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER)

class strand : private boost::noncopyable
{
public:
  explicit strand(boost::asio::io_service& io_service);

  boost::asio::io_service& get_io_service();

  template<typename Handler>
  void dispatch(MA_FWD_REF(Handler) handler);

  template<typename Handler>
  void post(MA_FWD_REF(Handler) handler);

  template<typename Handler>
  strand_wrapped_handler<typename detail::decay<Handler>::type>
  wrap(MA_FWD_REF(Handler) handler);

#if BOOST_VERSION >= 105400

  bool running_in_this_thread() const;

#endif

private:
  boost::asio::io_service::strand strand_;
}; // class strand

inline strand::strand(boost::asio::io_service& io_service)
  : strand_(io_service)
{
}

inline boost::asio::io_service& strand::get_io_service()
{
  return strand_.get_io_service();
}

template<typename Handler>
void strand::dispatch(MA_FWD_REF(Handler) handler)
{
  strand_.dispatch(detail::forward<Handler>(handler));
}

template<typename Handler>
void strand::post(MA_FWD_REF(Handler) handler)
{
  strand_.post(detail::forward<Handler>(handler));
}

template<typename Handler>
strand_wrapped_handler<typename detail::decay<Handler>::type>
strand::wrap(MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<Handler>::type handler_type;
  return strand_wrapped_handler<handler_type>(
      strand_, detail::forward<Handler>(handler));
}

#if BOOST_VERSION >= 105400

inline bool strand::running_in_this_thread() const
{
  return strand_.running_in_this_thread();
}

#endif // BOOST_VERSION >= 105400

#else  // defined(MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER)

typedef boost::asio::io_service::strand strand;

#endif // defined(MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER)

} // namespace ma

#endif // MA_STRAND_HPP
