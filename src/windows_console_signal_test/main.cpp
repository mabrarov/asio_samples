//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#endif

#include <cstdlib>
#include <exception>
#include <iostream>

namespace ma {
namespace test {
namespace windows_console_signal_destruction {

void run_test();

} // namespace windows_console_signal_destruction
} // namespace test
} // namespace ma

#if defined(WIN32)
int _tmain(int /*argc*/, _TCHAR* /*argv*/[])
#else
int main(int /*argc*/, char* /*argv*/[])
#endif
{
  try
  {
    ma::test::windows_console_signal_destruction::run_test();    
    return EXIT_SUCCESS;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected exception: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Unknown exception" << std::endl;
  }
  return EXIT_FAILURE;
}

namespace ma {
namespace test {
namespace windows_console_signal_destruction {

void run_test()
{
  //todo
}

} // namespace windows_console_signal_destruction
} // namespace test
} // namespace ma
