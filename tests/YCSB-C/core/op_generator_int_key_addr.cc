#include "core/op_generator_int_key_addr.h"
namespace ycsbc {
int OpGeneratorIntKeyAddr::GenerateRead(
    cxl_vector<Operation>& ops) {
  uint64_t key = workload_->NextTransactionNumKey();
  ops.emplace_back(READ, key, nullptr);
  return 0;
}

int OpGeneratorIntKeyAddr::GenerateReadModifyWrite(
    cxl_vector<Operation>& ops) {
  uint64_t key = workload_->NextTransactionNumKey();
  ops.emplace_back(READ, key, nullptr);
  void* value = workload_->BuildTransactionValue();
  ops.emplace_back(UPDATE, key, value);
  return 0;
}

int OpGeneratorIntKeyAddr::GenerateScan(
    cxl_vector<Operation>& ops) {
  uint64_t key = workload_->NextTransactionNumKey();
  int len = workload_->NextScanLength();
  ops.emplace_back(SCAN, key, reinterpret_cast<void*>(len));
  return 0;
}

int OpGeneratorIntKeyAddr::GenerateUpdate(
    cxl_vector<Operation>& ops) {
  uint64_t key = workload_->NextTransactionNumKey();
  void* value = workload_->BuildTransactionValue();
  ops.emplace_back(UPDATE, key, value);
  return 0;
}

int OpGeneratorIntKeyAddr::GenerateInsert(
    cxl_vector<Operation>& ops) {
  uint64_t key = workload_->NextSequenceNumKey();
  void* value = workload_->BuildTransactionValue();
  ops.emplace_back(INSERT, key, value);
  return 0;
}
}