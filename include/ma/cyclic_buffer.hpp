//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CYCLIC_BUFFER_HPP
#define MA_CYCLIC_BUFFER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <stdexcept>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/throw_exception.hpp>
#include <boost/scoped_array.hpp>

namespace ma {      

/// Read/write buffer with circular behaviour.
/** 
 * Buffer space is limited and cannot grow up. Also buffer space is separated
 * between two parts:
 *
 * @li unfilled (input) part,
 * @li filled (output) part.
 *
 * It is not necessarily each part to be represented as one noncontinuous 
 * memory block. In general each part can be represented by one or two 
 * noncontinuous memory blocks - Asio buffers.
 */
class cyclic_buffer : private boost::noncopyable
{
private:

  /// Generalized buffer sequence that represents cyclic_buffer space: unfilled
  /// or filled.
  /**
   * buffers_2 is CopyConstructible to meet Asio constant/mutable buffer
   * sequence requirements. See:
   * http://www.boost.org/doc/libs/1_46_1/doc/html/boost_asio/reference/ConstBufferSequence.html
   * and
   * http://www.boost.org/doc/libs/1_46_1/doc/html/boost_asio/reference/MutableBufferSequence.html
   */
  template <typename Buffer>
  class buffers_2
  {
  public:
    /// Asio requirements.
    typedef Buffer            value_type;    
    typedef const value_type* const_iterator;

    buffers_2()
      : buffers_count_(0)
    {
    }
    
    explicit buffers_2(const value_type& buffer1)
      : buffers_count_(1)
    {
      buffers_[0] = buffer1;
    }

    buffers_2(const value_type& buffer1, const value_type& buffer2)
      : buffers_count_(2)
    {
      buffers_[0] = buffer1;
      buffers_[1] = buffer2;
    }

    /// Asio requirement.
    const_iterator begin() const
    {
      return boost::addressof(buffers_[0]);
    }

    /// Asio requirement.
    const_iterator end() const
    {
      return boost::addressof(buffers_[0]) + buffers_count_;
    }

    /// Small but useful optimization.
    bool empty() const
    {
      return !buffers_count_;
    }

  private:
    boost::array<value_type, 2> buffers_;
    unsigned short buffers_count_;
  }; // class buffers_2

public:
  /// Constant buffer sequence.
  typedef buffers_2<boost::asio::const_buffer>   const_buffers_type;
  /// Mutable buffer sequence.
  typedef buffers_2<boost::asio::mutable_buffer> mutable_buffers_type;        

  explicit cyclic_buffer(std::size_t size)
    : data_(new char[size])
    , size_(size)
    , unfilled_start_(0)
    , unfilled_size_(size)
    , filled_start_(0)
    , filled_size_(0)
  {
  }

  /// Return to the state as right after construction.
  void reset()
  {
    unfilled_size_  = size_;
    unfilled_start_ = filled_start_ = filled_size_ = 0;
  }

  /// Reduce filled part by marking first size bytes of filled part as unfilled
  /// part.
  /**
   * Doesn't move or copy anything. Size of unfilled part part grows up by size
   * bytes. Start of unfilled part doesn't change. Size of filled part reduces 
   * by size bytes. Start of filled part moves up (circular) by size bytes.
   */
  void commit(std::size_t size)
  {
    if (size > filled_size_)
    {
      boost::throw_exception(std::length_error(
          "output sequence size is too small to consume given size"));
    }
    filled_size_   -= size;
    unfilled_size_ += size;
    std::size_t d = size_ - filled_start_;
    if (size < d)
    {
      filled_start_ += size;
    }
    else
    {
      filled_start_ = size - d;
    }
  }

  /// Reduce unfilled part by marking first size bytes of unfilled part as 
  /// filled part.
  /**
   * Doesn't move or copy anything. Size of filled part part grows up by size
   * bytes. Start of filled part doesn't change. Size of unfilled part reduces 
   * by size bytes. Start of unfilled part moves up (circular) by size bytes.
   */
  void consume(std::size_t size)         
  {
    if (size > unfilled_size_)
    {
      boost::throw_exception(std::length_error(
          "input sequence size is too small to consume given size"));
    }
    filled_size_   += size;
    unfilled_size_ -= size;
    std::size_t d = size_ - unfilled_start_;
    if (size < d)
    {
      unfilled_start_ += size;
    }
    else
    {
      unfilled_start_ = size - d;
    }
  }

  /// Return constant buffer sequence representing filled part.
  const_buffers_type data() const
  {
    if (!filled_size_)
    {
      return const_buffers_type();
    }
    std::size_t d = size_ - filled_start_;
    if (filled_size_ > d)
    {
      return const_buffers_type(
          boost::asio::const_buffer(data_.get() + filled_start_, d), 
          boost::asio::const_buffer(data_.get(), filled_size_ - d));
    }          
    return const_buffers_type(boost::asio::const_buffer(data_.get() 
        + filled_start_, filled_size_));
  }

  /// Return mutable buffer sequence representing unfilled part.
  mutable_buffers_type prepared() const
  {                    
    if (!unfilled_size_)
    {
      return mutable_buffers_type();
    }          
    std::size_t d = size_ - unfilled_start_;
    if (unfilled_size_ > d)
    {
      return mutable_buffers_type(
          boost::asio::mutable_buffer(data_.get() + unfilled_start_, d),
          boost::asio::mutable_buffer(data_.get(), unfilled_size_ - d));
    }
    return mutable_buffers_type(boost::asio::mutable_buffer(data_.get() 
        + unfilled_start_, unfilled_size_));          
  }

private:
  boost::scoped_array<char> data_;
  std::size_t size_;
  std::size_t unfilled_start_;
  std::size_t unfilled_size_;
  std::size_t filled_start_;
  std::size_t filled_size_;
}; // class cyclic_buffer

} // namespace ma

#endif // MA_CYCLIC_BUFFER_HPP
