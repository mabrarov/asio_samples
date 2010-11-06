//
// Copyright (c) 2008-2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_STORAGE_HPP
#define MA_HANDLER_STORAGE_HPP

#include <cstddef>
#include <boost/utility.hpp>
#include <boost/call_traits.hpp>
#include <ma/handler_storage_service.hpp>

namespace ma
{  
  template <typename Arg>
  class handler_storage : private boost::noncopyable
  {
  public:
    typedef Arg argument_type;    
    typedef typename boost::call_traits<argument_type>::param_type arg_param_type;    
    typedef handler_storage_service<argument_type>                 service_type;
    typedef typename service_type::implementation_type             implementation_type;

    explicit handler_storage(boost::asio::io_service& io_service)
      : service_(boost::asio::use_service<service_type>(io_service))
    {
      service_.construct(implementation_);
    }

    ~handler_storage()
    {
      service_.destroy(implementation_);
    } 

    boost::asio::io_service& io_service()
    {
      return service_.get_io_service();
    }

    boost::asio::io_service& get_io_service()
    {
      return service_.get_io_service();
    }

    void* data() const
    {
      return service_.data(implementation_);
    }

    bool empty() const
    {
      return service_.empty(implementation_);
    }

    template <typename Handler>
    void put(Handler handler)
    {
      service_.put(implementation_, handler);
    }

    void post(arg_param_type arg)
    {      
      service_.post(implementation_, arg);
    }    
    
  private:
    // The service associated with the I/O object.
    service_type& service_;

    // The underlying implementation of the I/O object.
    implementation_type implementation_;
  }; // class handler_storage  

} // namespace ma

#endif // MA_HANDLER_STORAGE_HPP