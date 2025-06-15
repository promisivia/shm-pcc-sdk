#include "core/perf.h"

#include <atomic>
#include <boost/multiprecision/cpp_int.hpp>
#include <iostream>
#include <numeric>
#include <vector>

#include "core/db_config.h"

void print_define() {
  std::string no_cc, opt_gc, opt_root_read, opt_in_use_flag, mq;
#ifdef NO_CC
  no_cc = "true";
#else
  no_cc = "false";
#endif
#ifdef OPT_GC
  opt_gc = "true";
#else
  opt_gc = "false";
#endif
#ifdef OPT_ROOT_READ
  opt_root_read = "true";
#else
  opt_root_read = "false";
#endif
#ifdef OPT_IN_USE_FLAG
  opt_in_use_flag = "true";
#else
  opt_in_use_flag = "false";
#endif
#ifdef USE_MSG_QUEUE
  mq = "true";
#else
  mq = "false";
#endif

  std::cerr << "NO_CC\t" << no_cc << "\n"
            << "OPT_GC\t" << opt_gc << "\n"
            << "OPT_ROOT_READ\t" << opt_root_read << "\n"
            << "OPT_IN_USE_FLAG\t" << opt_in_use_flag << "\n"
            << "USE_MSG_QUEUE\t" << mq << "\n";
}
