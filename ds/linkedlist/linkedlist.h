#pragma once
#include <functional>

#include "utils/compare.h"

template <typename KeyType, typename ValueType>
class LLBase {
  static_assert(sizeof(KeyType) <= 8, "KeyType must be 64 bits or less");
  static_assert(sizeof(ValueType) <= 8, "ValueType must be 64 bits or less");

 public:
  virtual bool find(const KeyType key, ValueType* val) = 0;
  virtual void insert(const KeyType key, ValueType val) = 0;
  virtual void remove(const KeyType key) = 0;
  virtual void destroy() = 0;
  virtual ~LLBase() = default;
};