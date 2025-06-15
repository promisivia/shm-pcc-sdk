#pragma once

#include <iostream>
#include <unordered_map>
#include <memory>

#include "generator.h"
#include "core_workload.h"

namespace ycsbc {

struct SingleTrace {
  int ts;
  uint32_t key_size;
  uint32_t val_size;
  uint32_t cid;
  std::string key;
  OpType op;
  int ttl;

  void Print() const {
    std::cout << "Timestamp: " << ts << ", Key: " << key
              << ", Key Size: " << key_size << ", Value Size: " << val_size
              << ", Client ID: " << cid << ", Operation: " << op
              << ", TTL: " << ttl << std::endl;
  }
};

class RealWorkload : public CoreWorkload {
public:
  RealWorkload(std::string trace, std::string workload_name, int client_num) : 
  CoreWorkload(), 
  trace_name_(trace), 
  workload_name_(workload_name),
  client_num_(client_num)
   {}
  RealWorkload() : CoreWorkload() {}
  virtual ~RealWorkload() override {}
  virtual void Init(utils::Properties &p) override;
#ifndef YCSB_KEY
  virtual uint64_t NextTransactionNumKey() override;
  virtual uint64_t NextSequenceNumKey() override;
#else
  virtual std::string NextTransactionKey() override;
  virtual std::string NextSequenceKey() override;
  #endif
  virtual OpType NextOperation() override { return real_op_chooser_->Next(); }
  virtual void BuildValues(std::vector<ycsbc::DB::KVPair> &values) override;
  virtual void BuildUpdate(std::vector<ycsbc::DB::KVPair> &update) override;
protected:
  std::string trace_name_{};
  std::string workload_name_{};
  int client_num_;
  std::unique_ptr<Generator<OpType>> real_op_chooser_;
  #ifndef YCSB_KEY
  std::unique_ptr<Generator<uint64_t>> real_key_chooser_;
  std::unique_ptr<Generator<uint64_t>> real_key_generator_;
  #else
  std::unique_ptr<Generator<std::string>> real_key_chooser_;
  std::unique_ptr<Generator<std::string>> real_key_generator_;
  #endif
  std::unordered_map<std::string, SingleTrace> key_to_trace_;
};

#ifndef YCSB_KEY
class RealTraceSequenceKeyGenerator : public Generator<uint64_t> {
public:
  RealTraceSequenceKeyGenerator(const std::vector<std::string> &trace);
  uint64_t Next();
  uint64_t Last();
private:
  std::atomic<uint64_t> last_index_{0};
  std::vector<uint64_t> keys{};
  uint64_t max_size;
};
#else
class RealTraceSequenceKeyGenerator : public Generator<std::string> {
public:
  RealTraceSequenceKeyGenerator(const std::vector<std::string> &trace);
  std::string Next();
  std::string Last();
private:
  uint64_t last_index_{0};
  std::vector<std::string> keys{};
  uint64_t max_size;
  std::mutex mt;
};
#endif

#ifndef YCSB_KEY
class RealTraceTransactionKeyGenerator : public Generator<uint64_t> {
public:
  RealTraceTransactionKeyGenerator(const std::vector<SingleTrace> &trace);
  uint64_t Next();
  uint64_t Last();
private:
  std::atomic<uint64_t> last_index_{0};
  std::vector<uint64_t> keys{};
  uint64_t max_size;
};
#else
class RealTraceTransactionKeyGenerator : public Generator<std::string> {
public:
  RealTraceTransactionKeyGenerator(const std::vector<SingleTrace> &trace);
  std::string Next();
  std::string Last();
private:
  uint64_t last_index_{0};
  std::vector<std::string> keys{};
  uint64_t max_size;
  std::mutex mt;
};
#endif

class RealTraceOpGenerator : public Generator<OpType> {
public:
  RealTraceOpGenerator(const std::vector<SingleTrace> &trace);
  OpType Next();
  OpType Last();
private:
  uint64_t last_index_{0};
  std::vector<OpType> ops{};
  uint64_t max_size;
  std::mutex mt;
};

class RealTraceValueSizeGenerator : public Generator<uint64_t> {
public:
  RealTraceValueSizeGenerator(const std::vector<uint64_t> &trace);
  uint64_t Next();
  uint64_t Last();
private:
  uint64_t last_index_{0};
  std::vector<uint64_t> lens{};
  uint64_t max_size;
  std::mutex mt;
};

class RealTraceGroupGenerator : public Generator<uint64_t> {
public:
  RealTraceGroupGenerator(const std::vector<SingleTrace> &trace);
  uint64_t Next();
  uint64_t Last();

 private:
  uint64_t last_index_{0};
  std::vector<uint64_t> groups{};
  uint64_t max_size;
  std::mutex mt;
};

OpType ParseOp(const std::string &op);
}
