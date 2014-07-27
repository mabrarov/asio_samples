//
// Copyright (c) 2010-2014 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_THREAD_GROUP_HPP
#define MA_THREAD_GROUP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>
#include <ma/functional.hpp>
#include <boost/noncopyable.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif

#if defined(MA_USE_CXX11_THREAD) && defined(MA_HAS_RVALUE_REFS)
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

#if defined(MA_USE_CXX11_THREAD) && defined(MA_HAS_RVALUE_REFS)
  template <typename Task> 
  void create_thread(Task&&);
#elif defined(MA_HAS_RVALUE_REFS)
  template <typename Task>
  void create_thread(Task&&);
#else
  template <typename Task>
  void create_thread(const Task&);
#endif

  void join_all();  

private:

#if defined(MA_USE_CXX11_THREAD) && defined(MA_HAS_RVALUE_REFS)
  std::vector<std::thread> threads_;
#else
  boost::thread_group      threads_;
#endif

}; // class thread_group

inline thread_group::thread_group()
  : threads_()
{
}

#if defined(MA_USE_CXX11_THREAD) && defined(MA_HAS_RVALUE_REFS)

template <typename Task>
inline void thread_group::create_thread(Task&& task)
{
  threads_.emplace_back(std::forward<Task>(task));
}

inline void thread_group::join_all()
{
  std::for_each(threads_.begin(), threads_.end(), 
      MA_BIND(&std::thread::join, MA_PLACEHOLDER_1));
}

#else  // defined(MA_USE_CXX11_THREAD) && defined(MA_HAS_RVALUE_REFS)

#if defined(MA_HAS_RVALUE_REFS)

template <typename Task>
inline void thread_group::create_thread(Task&& task)
{
  threads_.create_thread(std::forward<Task>(task));
}

#else  // defined(MA_HAS_RVALUE_REFS)

template <typename Task>
inline void thread_group::create_thread(const Task& task)
{
  threads_.create_thread(task);
}

#endif // defined(MA_HAS_RVALUE_REFS)

inline void thread_group::join_all()
{
  threads_.join_all();
}

#endif // defined(MA_USE_CXX11_THREAD) && defined(MA_HAS_RVALUE_REFS)

} // namespace ma

#endif // MA_THREAD_GROUP_HPP
