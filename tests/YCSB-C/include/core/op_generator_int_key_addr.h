#pragma once
#include "core/op_generator.h"

namespace ycsbc {
class OpGeneratorIntKeyAddr : public OpGenerator<uint64_t, void*> {
 public:
  OpGeneratorIntKeyAddr(CoreWorkload* wl)
      : OpGenerator(wl) {}
  using Operation = OperationInternal<uint64_t, void*>;
  int GenerateRead(cxl_vector<Operation>& ops) override;
  int GenerateReadModifyWrite(cxl_vector<Operation>& ops) override;
  int GenerateScan(cxl_vector<Operation>& ops) override;
  int GenerateUpdate(cxl_vector<Operation>& ops) override;
  int GenerateInsert(cxl_vector<Operation>& ops) override;
};
}  // namespace ycsbc