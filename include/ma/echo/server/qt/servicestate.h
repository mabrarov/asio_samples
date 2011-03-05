//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_SERVICESTATE_H
#define MA_ECHO_SERVER_QT_SERVICESTATE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

namespace ma
{    
  namespace echo
  {
    namespace server
    {    
      namespace qt 
      {
        namespace ServiceState
        {
          enum State
          {
            stopped,
            startInProgress,
            started,
            stopInProgress
          }; // enum State
        }

      } // namespace qt
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_SERVICESTATE_H
