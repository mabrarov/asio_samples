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
#include <boost/utility.hpp>
#include <boost/throw_exception.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>

namespace ma
{      
  class cyclic_buffer : private boost::noncopyable
  {
  private:
    template <typename Buffer>
    class buffers2
    {
    public:
      typedef Buffer value_type;
      typedef const value_type* const_iterator;

      explicit buffers2()
        : filled_buffers_(0)
      {
      }

      explicit buffers2(const value_type& buffer1)
        : filled_buffers_(1)
      {
        buffers_[0] = buffer1;
      }

      explicit buffers2(const value_type& buffer1, const value_type& buffer2)
        : filled_buffers_(2)
      {
        buffers_[0] = buffer1;
        buffers_[1] = buffer2;
      }

      const_iterator begin() const
      {
        return boost::addressof(buffers_[0]);
      }

      const_iterator end() const
      {
        return boost::addressof(buffers_[0]) + filled_buffers_;
      }

    private:
      boost::array<value_type, 2> buffers_;
      std::size_t filled_buffers_;
    }; // class buffers2

  public:        
    typedef buffers2<boost::asio::const_buffer> const_buffers_type;
    typedef buffers2<boost::asio::mutable_buffer> mutable_buffers_type;        

    explicit cyclic_buffer(std::size_t size)
      : data_(new char[size])
      , size_(size)
      , input_start_(0)
      , input_size_(size)
      , output_start_(0)
      , output_size_(0)
    {
    }

    void reset()
    {
      input_size_  = size_;
      input_start_ = output_start_ = output_size_ = 0;
    }

    void commit(std::size_t size)
    {
      if (size > output_size_)
      {
        boost::throw_exception(
          std::length_error("output sequence size is too small to consume given size"));
      }
      output_size_ -= size;
      input_size_  += size;
      std::size_t d = size_ - output_start_;
      if (size < d)
      {
        output_start_ += size;
      }
      else
      {
        output_start_ = size - d;
      }
    }

    void consume(std::size_t size)         
    {
      if (size > input_size_)
      {
        boost::throw_exception(
          std::length_error("input sequence size is too small to consume given size"));
      }
      output_size_ += size;
      input_size_  -= size;
      std::size_t d = size_ - input_start_;
      if (size < d)
      {
        input_start_ += size;
      }
      else
      {
        input_start_ = size - d;
      }
    }

    const_buffers_type data() const
    {
      if (!output_size_)
      {
        return const_buffers_type();
      }
      std::size_t d = size_ - output_start_;
      if (output_size_ > d)
      {
        return const_buffers_type(
          boost::asio::const_buffer(
            data_.get() + output_start_, d),
          boost::asio::const_buffer(
            data_.get(), output_size_ - d));
      }          
      return const_buffers_type(
        boost::asio::const_buffer(
          data_.get() + output_start_, output_size_));
    }

    mutable_buffers_type prepared() const
    {                    
      if (!input_size_)
      {
        return mutable_buffers_type();
      }          
      std::size_t d = size_ - input_start_;
      if (input_size_ > d)
      {
        return mutable_buffers_type(
          boost::asio::mutable_buffer(
            data_.get() + input_start_, d),
          boost::asio::mutable_buffer(
            data_.get(), input_size_ - d));
      }
      return mutable_buffers_type(
        boost::asio::mutable_buffer(
          data_.get() + input_start_, input_size_));          
    }

  private:
    boost::scoped_array<char> data_;
    std::size_t size_;
    std::size_t input_start_;
    std::size_t input_size_;
    std::size_t output_start_;
    std::size_t output_size_;
  }; // class cyclic_buffer

} // namespace ma

#endif // MA_CYCLIC_BUFFER_HPP
