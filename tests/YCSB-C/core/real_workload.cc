#include <fstream>
#include <sstream>

#include "core/uniform_generator.h"
#include "core/real_workload.h"
#include "db/utils.h"

namespace ycsbc {

void RealWorkload::Init(utils::Properties &p) {
  table_name_ = p.GetProperty(TABLENAME_PROPERTY,TABLENAME_DEFAULT);
  
  field_count_ = std::stoi(p.GetProperty(FIELD_COUNT_PROPERTY,
                                         FIELD_COUNT_DEFAULT));
  
  std::string request_dist = p.GetProperty(REQUEST_DISTRIBUTION_PROPERTY,
                                           REQUEST_DISTRIBUTION_DEFAULT);
  zero_padding_ = std::stoi(p.GetProperty(ZERO_PADDING_PROPERTY, ZERO_PADDING_DEFAULT));
  
  read_all_fields_ = utils::StrToBool(p.GetProperty(READ_ALL_FIELDS_PROPERTY,
                                                    READ_ALL_FIELDS_DEFAULT));
  write_all_fields_ = utils::StrToBool(p.GetProperty(WRITE_ALL_FIELDS_PROPERTY,
                                                     WRITE_ALL_FIELDS_DEFAULT));
  
  if (p.GetProperty(INSERT_ORDER_PROPERTY, INSERT_ORDER_DEFAULT) == "hashed") {
    ordered_inserts_ = false;
  } else {
    ordered_inserts_ = true;
  }
  
  field_chooser_ = new UniformGenerator(0, field_count_ - 1);

  std::string sample_path = p.GetProperty("tracepath", "");
  std::string trace = p.GetProperty("tracename", "");
  std::string workload_name = p.GetProperty("workloadname", "");

  std::vector<SingleTrace> trace_list;
  std::vector<std::string> record_list;
  int op_count = 0;
  std::string wl_fname;
  wl_fname = sample_path + "/" + trace + "/" + workload_name;
  std::ifstream file(wl_fname);
  std::string line;

  if (!file.is_open()) {
    std::cerr << "open file error! " << wl_fname << std::endl;
    return;
  }

  // A map that when a entry is read, whether it exists in the DB. If not, then we need to insert it in advance
  std::unordered_map<std::string, bool> records_exist_map;
  // Value size of non-existing records, need to load into the DB in advance
  std::vector<uint64_t> prefill_value_size;
  // Value size of traces that update or insert new entry
  std::vector<uint64_t> transaction_value_size;

  while (std::getline(file, line)) {
    if (line.empty()) {
      continue;
    }

    std::istringstream iss(line);
    SingleTrace trace;

    char comma;
    std::string opbuf;
    if (iss >> trace.ts >> comma && std::getline(iss, trace.key, ',') &&
        iss >> trace.key_size >> comma && iss >> trace.val_size >> comma &&
        iss >> trace.cid >> comma && std::getline(iss, opbuf, ',') &&
        iss >> trace.ttl) {
      OpType op = ParseOp(opbuf);
      if (op == UNSUPPORTED) {
        continue;
      }
      if (trace.key.empty()) {
        continue;
      }
      trace.op = op;
      op_count++;
      if (op == READ && records_exist_map[trace.key] == false) {
        record_list.emplace_back(trace.key);
        prefill_value_size.emplace_back(trace.val_size);
      }
      records_exist_map[trace.key] = true;
      trace_list.emplace_back(trace);
      if(op == INSERT || op == UPDATE) {
        transaction_value_size.emplace_back(trace.val_size);
      }
    } else {
      std::cerr << "parse error: " << line << std::endl;
    }
  }

  file.close();

  p.SetProperty(CoreWorkload::RECORD_COUNT_PROPERTY,
                std::to_string(record_list.size()));
  p.SetProperty(CoreWorkload::OPERATION_COUNT_PROPERTY,
                std::to_string(op_count));

  prefill_val_size_generator_ =
      new RealTraceValueSizeGenerator(prefill_value_size);
  transaction_val_size_generator_ =
      new RealTraceValueSizeGenerator(transaction_value_size);
  real_op_chooser_ = std::make_unique<RealTraceOpGenerator>(trace_list);
  real_key_chooser_ =
      std::make_unique<RealTraceTransactionKeyGenerator>(trace_list);
  real_key_generator_ =
      std::make_unique<RealTraceSequenceKeyGenerator>(record_list);
  group_chooser_ = new RealTraceGroupGenerator(trace_list);
  // real_val_generator_ = 
}

#ifndef YCSB_KEY

uint64_t RealWorkload::NextSequenceNumKey() {
  return real_key_generator_->Next();
}

uint64_t RealWorkload::NextTransactionNumKey() {
  return real_key_chooser_->Next();
}

#else

std::string RealWorkload::NextSequenceKey() {
  return real_key_generator_->Next();
}


std::string RealWorkload::NextTransactionKey() {
  return real_key_chooser_->Next();
}

#endif

void RealWorkload::BuildValues(std::vector<ycsbc::DB::KVPair> &values) {
  for (int i = 0; i < field_count_; ++i) {
    ycsbc::DB::KVPair pair;
    #ifndef YCSB_KEY
    pair.first = utils::Random64();
    pair.second = pair.first;
    #else
    pair.first.append("field").append(std::to_string(i));
    pair.second.append(field_len_generator_->Next(), utils::RandomPrintChar());
    #endif
    values.push_back(pair);
  }
}

void RealWorkload::BuildUpdate(std::vector<ycsbc::DB::KVPair> &update) {
  ycsbc::DB::KVPair pair;
  #ifndef YCSB_KEY
  pair.first = utils::Random64();
  pair.second = pair.first;
  #else
  pair.first.append(NextFieldName());
  pair.second.append(field_len_generator_->Next(), utils::RandomPrintChar());
  #endif
  update.push_back(pair);
}

#ifndef YCSB_KEY

RealTraceSequenceKeyGenerator::RealTraceSequenceKeyGenerator(const std::vector<std::string> &records) {
  for (const auto &r : records) {
    keys.emplace_back(convert_std_hash(r));
  }
  max_size = keys.size();
}

uint64_t RealTraceSequenceKeyGenerator::Next() {
  uint64_t next = last_index_.fetch_add(1);
  return keys[next % max_size];
}

uint64_t RealTraceSequenceKeyGenerator::Last() {
  return keys[last_index_ % max_size];
}

RealTraceTransactionKeyGenerator::RealTraceTransactionKeyGenerator(const std::vector<SingleTrace> &trace) {
  for (const auto &t : trace) {
    keys.emplace_back(convert_std_hash(t.key));
  }
  max_size = keys.size();
}

uint64_t RealTraceTransactionKeyGenerator::Next() {
  uint64_t next = last_index_.fetch_add(1);
  return keys[next % max_size];
}

uint64_t RealTraceTransactionKeyGenerator::Last() {
  return keys[last_index_ % max_size];
}

#else

RealTraceSequenceKeyGenerator::RealTraceSequenceKeyGenerator(const std::vector<std::string> &records) {
  for (const auto &r : records) {
    keys.emplace_back(r);
  }
  max_size = keys.size();
}

std::string RealTraceSequenceKeyGenerator::Next() {
  std::unique_lock<std::mutex> lock(mt);
  uint64_t next = last_index_++;
  return keys[next % max_size];
}

std::string RealTraceSequenceKeyGenerator::Last() {
  return keys[last_index_ % max_size];
}

RealTraceTransactionKeyGenerator::RealTraceTransactionKeyGenerator(const std::vector<SingleTrace> &trace) {
  for (const auto &t : trace) {
    keys.emplace_back(t.key);
  }
  max_size = keys.size();
}

std::string RealTraceTransactionKeyGenerator::Next() {
  std::unique_lock<std::mutex> lock(mt);
  uint64_t next = last_index_++;
  return keys[next % max_size];
}

std::string RealTraceTransactionKeyGenerator::Last() {
  return keys[last_index_ % max_size];
}

#endif

RealTraceOpGenerator::RealTraceOpGenerator(const std::vector<SingleTrace> &trace) {
  for (const auto &t : trace) {
    ops.emplace_back(t.op);
  }
  max_size = ops.size();
}

OpType RealTraceOpGenerator::Next() {
  std::unique_lock<std::mutex> lock(mt);
  uint64_t next = last_index_++;
  return ops[next % max_size];
}

OpType RealTraceOpGenerator::Last() {
  return ops[last_index_ % max_size];
}

RealTraceValueSizeGenerator::RealTraceValueSizeGenerator(
    const std::vector<uint64_t> &trace) {
  for (const auto &t : trace) {
    lens.emplace_back(t);
  }
  max_size = lens.size();
}

uint64_t RealTraceValueSizeGenerator::Next() {
  std::unique_lock<std::mutex> lock(mt);
  uint64_t next = last_index_++;
  return lens[next % max_size];
}

uint64_t RealTraceValueSizeGenerator::Last() {
  return lens[last_index_ % max_size];
}

RealTraceGroupGenerator::RealTraceGroupGenerator(const std::vector<SingleTrace> &trace) {
  for (const auto &t : trace) {
    groups.emplace_back(t.cid);
  }
  max_size = groups.size();
}

uint64_t RealTraceGroupGenerator::Next() {
  std::unique_lock<std::mutex> lock(mt);
  uint64_t next = last_index_++;
  return groups[next % max_size];
}

uint64_t RealTraceGroupGenerator::Last() {
  return groups[last_index_ % max_size];
}

OpType ParseOp(const std::string &op) {
  if (op == "get" || op == "gets") {
    return OpType::READ;
  }
  if (op == "add") {
    return OpType::UPDATE;
  }
  if (op == "set") {
    return OpType::UPDATE;
  }
  return OpType::UNSUPPORTED;
}

};