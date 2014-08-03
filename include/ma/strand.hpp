//
// Copyright (c) 2010-2014 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
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

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {

#if defined(MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER)

class strand : private boost::noncopyable
{
public:
  explicit strand(boost::asio::io_service& io_service);

  boost::asio::io_service& get_io_service();

#if defined(MA_HAS_RVALUE_REFS)

  template<typename Handler>
  void dispatch(Handler&& handler);

  template<typename Handler>
  void post(Handler&& handler);

  template<typename Handler>
  strand_wrapped_handler<typename remove_cv_reference<Handler>::type>
  wrap(Handler&& handler);

#else  // defined(MA_HAS_RVALUE_REFS)

  template<typename Handler>
  void dispatch(const Handler& handler);

  template<typename Handler>
  void post(const Handler& handler);

  template<typename Handler>
  strand_wrapped_handler<Handler> wrap(const Handler& handler);

#endif // defined(MA_HAS_RVALUE_REFS)

#if BOOST_VERSION >= 105400

  bool running_in_this_thread() const;

#endif // BOOST_VERSION >= 105400

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

#if defined(MA_HAS_RVALUE_REFS)

template<typename Handler>
void strand::dispatch(Handler&& handler)
{
  strand_.dispatch(std::forward<Handler>(handler));
}

template<typename Handler>
void strand::post(Handler&& handler)
{
  strand_.post(std::forward<Handler>(handler));
}

template<typename Handler>
strand_wrapped_handler<typename remove_cv_reference<Handler>::type>
strand::wrap(Handler&& handler)
{
  typedef typename remove_cv_reference<Handler>::type handler_type;
  return strand_wrapped_handler<handler_type>(
    strand_, std::forward<Handler>(handler));
}

#else  // defined(MA_HAS_RVALUE_REFS)

template<typename Handler>
void strand::dispatch(const Handler& handler)
{
  strand_.dispatch(handler);
}

template<typename Handler>
void strand::post(const Handler& handler)
{
  strand_.post(handler);
}

template<typename Handler>
strand_wrapped_handler<Handler> strand::wrap(const Handler& handler)
{
  return strand_wrapped_handler<Handler>(strand_, handler);
}

#endif // defined(MA_HAS_RVALUE_REFS)

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
