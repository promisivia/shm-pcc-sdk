#include "core/op_generator_ycsb.h"

namespace ycsbc {
int OpGeneratorYCSB::GenerateRead(cxl_vector<Operation> &ops) {
  const std::string &table = workload_->NextTable();
  const std::string &key = workload_->NextTransactionKey();
  std::vector<DB::KVPair> result;
  if (!workload_->read_all_fields()) {
    std::vector<std::string> fields;
    fields.push_back("field" + workload_->NextFieldName());
    ops.emplace_back(READ, table, key, fields, result);
  } else {
    ops.emplace_back(READ, table, key, result);
  }
  return 0;
}

int OpGeneratorYCSB::GenerateReadModifyWrite(cxl_vector<Operation> &ops) {
  const std::string &table = workload_->NextTable();
  const std::string &key = workload_->NextTransactionKey();
  std::vector<DB::KVPair> result;

  if (!workload_->read_all_fields()) {
    std::vector<std::string> fields;
    fields.push_back("field" + workload_->NextFieldName());
    ops.emplace_back(READ, table, key, fields, result);
  } else {
    ops.emplace_back(READ, table, key, result);
  }

  std::vector<DB::KVPair> values;
  if (workload_->write_all_fields()) {
    workload_->BuildValues(values);
  } else {
    workload_->BuildUpdate(values);
  }
  ops.emplace_back(UPDATE, table, key, values);
  return 0;
}

int OpGeneratorYCSB::GenerateScan(cxl_vector<Operation> &ops) {
  const std::string &table = workload_->NextTable();
  const std::string &key = workload_->NextTransactionKey();
  int len = workload_->NextScanLength();
  std::vector<std::vector<DB::KVPair>> result;
  if (!workload_->read_all_fields()) {
    std::vector<std::string> fields;
    fields.push_back("field" + workload_->NextFieldName());
    ops.emplace_back(SCAN, table, key, fields, len);
  } else {
    ops.emplace_back(SCAN, table, key, len);
  }
  return 0;
}

int OpGeneratorYCSB::GenerateUpdate(cxl_vector<Operation> &ops) {
  const std::string &table = workload_->NextTable();
  const std::string &key = workload_->NextTransactionKey();
  std::vector<DB::KVPair> values;
  if (workload_->write_all_fields()) {
    workload_->BuildValues(values);
  } else {
    workload_->BuildUpdate(values);
  }
  ops.emplace_back(UPDATE, table, key, values);
  return 0;
}

int OpGeneratorYCSB::GenerateInsert(cxl_vector<Operation> &ops) {
  const std::string &table = workload_->NextTable();
  const std::string &key = workload_->NextSequenceKey();
  std::vector<DB::KVPair> values;
  workload_->BuildValues(values);
  ops.emplace_back(INSERT, table, key, values);
  return 0;
}
}  // namespace ycsbc