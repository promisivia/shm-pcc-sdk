//
//  uniform_generator.h
//  YCSB-C
//
//  Created by Jinglei Ren on 12/6/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#ifndef YCSB_C_UNIFORM_GENERATOR_H_
#define YCSB_C_UNIFORM_GENERATOR_H_

#include "generator.h"

#include <atomic>
#include <mutex>
#include <random>
#include <iostream>

namespace ycsbc {

class UniformGenerator : public Generator<uint64_t> {
 public:
  // Both min and max are inclusive
  UniformGenerator(uint64_t min, uint64_t max) : min(min), max(max) { Next(); }

  uint64_t Next();
  uint64_t Last();
  
 private:
  inline static thread_local std::mt19937_64 generator_;
  uint64_t last_int_;
  uint64_t min;
  uint64_t max;
  // std::mutex mutex_;
};

// thread_local std::mt19937_64 UniformGenerator::generator_;

inline uint64_t UniformGenerator::Next() {
  std::uniform_int_distribution<uint64_t> dist_(min, max);
  // std::lock_guard<std::mutex> lock(mutex_);
  uint64_t last_int_ = dist_(generator_);
  // std::cout << "last_int_: " << last_int_ << std::endl;
  return last_int_;
}

inline uint64_t UniformGenerator::Last() {
  std::uniform_int_distribution<uint64_t> dist_(min, max);
  // std::lock_guard<std::mutex> lock(mutex_);
  return last_int_;
}

} // ycsbc

#endif // YCSB_C_UNIFORM_GENERATOR_H_
