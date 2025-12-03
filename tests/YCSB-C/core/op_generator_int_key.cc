#include "core/op_generator_int_key.h"
#include <iostream>

namespace ycsbc {

int OpGeneratorIntKey::GenerateRead(
    cxl_vector<Operation>& ops) {
  uint64_t key = workload_->NextTransactionNumKey();
  ops.emplace_back(READ, key, 0);
  return 0;
}

int OpGeneratorIntKey::GenerateReadModifyWrite(
    cxl_vector<Operation>& ops) {
  uint64_t key = workload_->NextTransactionNumKey();
  ops.emplace_back(READ, key, 0);
  uint64_t value = workload_->BuildNumValue();
  ops.emplace_back(UPDATE, key, value);
  return 0;
}

int OpGeneratorIntKey::GenerateScan(
    cxl_vector<Operation>& ops) {
  uint64_t key = workload_->NextTransactionNumKey();
  int len;
  do {
    len = workload_->NextScanLength();
  } while (len <= 0);
  ops.emplace_back(SCAN, key, len);
  return 0;
}

int OpGeneratorIntKey::GenerateUpdate(
    cxl_vector<Operation>& ops) {
  uint64_t key = workload_->NextTransactionNumKey();
  uint64_t value = workload_->BuildNumValue();
  ops.emplace_back(UPDATE, key, value);
  return 0;
}

int OpGeneratorIntKey::GenerateInsert(
    cxl_vector<Operation>& ops) {
  uint64_t key = workload_->NextSequenceNumKey();
  uint64_t value = workload_->BuildNumValue();
  ops.emplace_back(INSERT, key, value);
  return 0;
}

}  // namespace ycsbc