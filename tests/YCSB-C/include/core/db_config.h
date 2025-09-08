#pragma once

// (STRING / INT)
// #define INT_YCSBC_KEY
#define INT_KEY_ADDR
// #define YCSB_KEY

// #define USE_MSG_QUEUE

#ifdef USE_MSG_QUEUE
  #define USE_MWAIT
  #define RETURN_SYNC
  // #define USE_NO_CC_QUEUE
  #define ASYNC_CLIENT
#endif

#ifdef INT_KEY_ADDR
#define VALUE_ADDR_SIZE 8
#endif