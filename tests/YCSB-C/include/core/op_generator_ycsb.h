#pragma once
#include "core/op_generator.h"

namespace ycsbc {
class OpGeneratorYCSB
    : public OpGenerator<std::string, std::vector<DB::KVPair>> {
 public:
 OpGeneratorYCSB(CoreWorkload* wl) : OpGenerator(wl) {}
  using Operation = OperationInternal<std::string, std::vector<DB::KVPair>>;
  int GenerateRead(cxl_vector<Operation>& ops) override;
  int GenerateReadModifyWrite(cxl_vector<Operation>& ops) override;
  int GenerateScan(cxl_vector<Operation>& ops) override;
  int GenerateUpdate(cxl_vector<Operation>& ops) override;
  int GenerateInsert(cxl_vector<Operation>& ops) override;
};
}  // namespace ycsbc