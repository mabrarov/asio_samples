//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_THREAD_GROUP_HPP
#define MA_THREAD_GROUP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>
#include <boost/noncopyable.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/utility.hpp>

#if defined(MA_USE_CXX11_STDLIB_THREAD) && defined(MA_HAS_RVALUE_REFS)
#include <thread>
#include <vector>
#include <algorithm>
#else
#include <boost/thread/thread.hpp>
#endif

namespace ma {

class thread_group : private boost::noncopyable
{
public:
  thread_group();

  template <typename Task>
  void create_thread(MA_FWD_REF(Task) task);

  void join_all();

private:

#if defined(MA_USE_CXX11_STDLIB_THREAD) && defined(MA_HAS_RVALUE_REFS)
  std::vector<std::thread> threads_;
#else
  boost::thread_group      threads_;
#endif

}; // class thread_group

inline thread_group::thread_group()
  : threads_()
{
}

template <typename Task>
void thread_group::create_thread(MA_FWD_REF(Task) task)
{
#if defined(MA_USE_CXX11_STDLIB_THREAD) && defined(MA_HAS_RVALUE_REFS)
  threads_.emplace_back(detail::forward<Task>(task));
#else
  threads_.create_thread(detail::forward<Task>(task));
#endif
}

inline void thread_group::join_all()
{
#if defined(MA_USE_CXX11_STDLIB_THREAD) && defined(MA_HAS_RVALUE_REFS)
  std::for_each(threads_.begin(), threads_.end(),
      detail::bind(&std::thread::join, detail::placeholders::_1));
#else
  threads_.join_all();
#endif
}

} // namespace ma

#endif // MA_THREAD_GROUP_HPP
