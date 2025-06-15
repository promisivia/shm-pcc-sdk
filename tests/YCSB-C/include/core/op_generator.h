#pragma once

#include <vector>

#include "core/core_workload.h"
#include "core/db.h"
#include "shm/cxl_type.h"

namespace ycsbc {
template <typename K, typename V>
class OpGenerator {
 public:
  OpGenerator(CoreWorkload* wl) : workload_(wl) {}
  virtual int GenerateRead(cxl_vector<OperationInternal<K, V>>& ops) = 0;
  virtual int GenerateReadModifyWrite(cxl_vector<OperationInternal<K, V>>& ops) = 0;
  virtual int GenerateScan(cxl_vector<OperationInternal<K, V>>& ops) = 0;
  virtual int GenerateUpdate(cxl_vector<OperationInternal<K, V>>& ops) = 0;
  virtual int GenerateInsert(cxl_vector<OperationInternal<K, V>>& ops) = 0;

  virtual cxl_vector<cxl_vector<OperationInternal<K, V>>> GenerateOperations(
      int num_ops, int group_size) {
    cxl_vector<cxl_vector<OperationInternal<K, V>>> ops(group_size);
    for (int i = 0; i < group_size; ++i) {
      // Reserve large enough space
      ops[i].reserve(1.1 * num_ops / group_size);
    }
    for (int i = 0; i < num_ops; ++i) {
      int group = workload_->NextGroup(group_size);
      switch (workload_->NextOperation()) {
        case READ:
          GenerateRead(ops[group]);
          break;
        case UPDATE:
          GenerateUpdate(ops[group]);
          break;
        case INSERT:
          GenerateInsert(ops[group]);
          break;
        case SCAN:
          GenerateScan(ops[group]);
          break;
        case READMODIFYWRITE:
          GenerateReadModifyWrite(ops[group]);
          break;
        default:
          throw utils::Exception("Operation request is not recognized!");
      }
    }
    return ops;
  }

  virtual ~OpGenerator() {}

 protected:
  CoreWorkload* workload_;
};
}  // namespace ycsbc