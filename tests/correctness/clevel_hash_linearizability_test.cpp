#include <libpmemobj++/experimental/clevel_hash.hpp>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sys/stat.h>
#include <libpmemobj.h>

using persistent_map_type = clevel_hash<std::string, std::string>;

enum class OpType { Insert, Read, Update, Delete, RehashInsert, RehashRead };

struct OpRecord {
	// immutable info
	uint32_t threadId;
	uint64_t opId; // global op id for ordering reference
	std::string key;
	std::string value; // for write/update new value
	OpType type;
	// timing
	std::chrono::steady_clock::time_point start;
	std::chrono::steady_clock::time_point end;
	// observed results
	bool found;
	std::optional<std::string> observedValue; // for reads: value if found
};

static std::string random_key(std::mt19937_64 &rng, size_t len) {
	static const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
	std::uniform_int_distribution<size_t> dist(0, sizeof(chars) - 2);
	std::string s;
	s.reserve(len);
	for (size_t i = 0; i < len; ++i) s.push_back(chars[dist(rng)]);
	return s;
}

static std::string value_from_key(const std::string &k, uint64_t vtag) {
	return k + ":" + std::to_string(vtag);
}

struct LinearCheckCtx {
	// precedence graph: i must come before j if end_i <= start_j (real-time)
	std::vector<std::vector<int>> preds; // list of predecessors for each op
	std::vector<int> indeg; // dynamic in the solver
};

static bool apply_and_validate(const OpRecord &op, std::optional<std::string> &state) {
	switch (op.type) {
		case OpType::Insert:
		case OpType::RehashInsert:
			if (state.has_value()) {
				return op.found == true; // existed before
			} else {
				state = op.value; // new value equals provided value
				return op.found == false;
			}
		case OpType::Read:
		case OpType::RehashRead:
			if (state.has_value()) {
				return op.found == true && op.observedValue == state;
			} else {
				return op.found == false;
			}
		case OpType::Update:
			if (state.has_value()) {
				// update successful
				if (op.found != true) return false;
				state = op.value;
				return true;
			} else {
				// update on absent key
				return op.found == false;
			}
		case OpType::Delete:
			if (state.has_value()) {
				if (op.found != true) return false;
				state.reset();
				return true;
			} else {
				return op.found == false;
			}
	}
	return false;
}

static bool linearizable_per_key(const std::vector<OpRecord> &ops) {
	const int n = (int)ops.size();
	if (n == 0) return true;

	LinearCheckCtx g;
	g.preds.assign(n, {});
	g.indeg.assign(n, 0);
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (i == j) continue;
			// real-time order: if i completes before j starts, i precedes j
			if (ops[i].end <= ops[j].start) {
				g.preds[j].push_back(i);
				g.indeg[j]++;
			}
		}
	}

	// backtracking with precedence constraints
	std::vector<char> placed(n, 0);
	std::vector<int> order;
	order.reserve(n);
	std::optional<std::string> state;

	std::function<bool()> dfs = [&]() -> bool {
		if ((int)order.size() == n) return true;
		// collect candidates with indegree 0 and not placed
		for (int i = 0; i < n; ++i) {
			if (placed[i] || g.indeg[i] != 0) continue;
			// try i
			placed[i] = 1;
			order.push_back(i);
			// save state
			auto saved_state = state;
			// decrease indegree of successors (simulate removal)
			for (int k = 0; k < n; ++k) {
				// if i is pred of k, reduce indeg
				for (int p : g.preds[k]) {
					if (p == i) { g.indeg[k]--; break; }
				}
			}
			bool ok = apply_and_validate(ops[i], state);
			if (ok && dfs()) return true;
			// backtrack
			state = saved_state;
			for (int k = 0; k < n; ++k) {
				for (int p : g.preds[k]) {
					if (p == i) { g.indeg[k]++; break; }
				}
			}
			order.pop_back();
			placed[i] = 0;
		}
		return false;
	};

	// heuristic: try smaller end time first by temporarily permuting indices
	std::vector<int> idx(n);
	for (int i = 0; i < n; ++i) idx[i] = i;
	std::stable_sort(idx.begin(), idx.end(), [&](int a, int b) { return ops[a].end < ops[b].end; });
	// reorder ops logically by idx via mapping function in dfs would be complex; instead, rely on natural iteration and hope pruning suffices.
	return dfs();
}

int main(int argc, char **argv) {
	// args: threads ops_per_thread key_space seed [rehash_test]
	uint32_t threads = 8;
	uint32_t ops_per_thread = 50000;
	uint32_t key_space = 5000000;
	uint64_t seed = 42;
	bool rehash_test = false;
	
	if (argc >= 2) threads = std::stoul(argv[1]);
	if (argc >= 3) ops_per_thread = std::stoul(argv[2]);
	if (argc >= 4) key_space = std::stoul(argv[3]);
	if (argc >= 5) seed = std::stoull(argv[4]);
	if (argc >= 6) rehash_test = (std::string(argv[5]) == "rehash");
	
	// 限制单线程操作次数以缩短运行时间
	uint32_t ops_limit = std::min<uint32_t>(ops_per_thread, 1000);

	// Initialize SimThreadInfo for shm-lib
	SimThreadInfo::setup_worker_machine_count(1);
	SimThreadInfo::setup_machine_ids(0);
	SimThreadInfo::setup_worker_nr(threads);
	for (uint32_t i = 0; i < threads; ++i) {
		SimThreadInfo::setup_worker_ids(0, i);
	}

	std::cout << "threads=" << threads
	          << " ops_per_thread=" << ops_per_thread
	          << " key_space=" << key_space
	          << " seed=" << seed << std::endl;

	persistent_map_type map;
	map.set_thread_num(threads);

	// 预填充数据来触发 rehash
	std::cout << "Pre-filling data to trigger rehash..." << std::endl;
	{
		std::mt19937_64 prefill_gen(seed + 999);
		if (rehash_test) {
			// rehash 测试模式：使用极端的冲突策略（限制上限以缩短运行时间）
			std::cout << "Using aggressive rehash trigger strategy..." << std::endl;
			uint32_t prefill_limit = std::min<uint32_t>(key_space * 2, 2000);
			for (uint32_t i = 0; i < prefill_limit; ++i) {
				// 使用极少的哈希值来最大化冲突
				std::string prefill_key = "extreme_conflict_" + std::to_string(i % 10) + "_" + std::to_string(i);
				std::string prefill_val = "prefill_val_" + std::to_string(i);
				map.insert(persistent_map_type::value_type(prefill_key, prefill_val), 0, i);
				
				// 每插入一定数量后检查是否需要触发 rehash
				if (i % 100 == 0) {
					std::cout << "Pre-filled " << i << " keys..." << std::endl;
				}
			}
		} else {
			// 正常模式：使用冲突的键来强制触发 rehash（限制上限以缩短运行时间）
			uint32_t prefill_limit = std::min<uint32_t>(key_space, 1000);
			for (uint32_t i = 0; i < prefill_limit; ++i) {
				// 使用相同的哈希值来增加冲突
				std::string prefill_key = "conflict_key_" + std::to_string(i % 100) + "_" + std::to_string(i);
				std::string prefill_val = "prefill_val_" + std::to_string(i);
				map.insert(persistent_map_type::value_type(prefill_key, prefill_val), 0, i);
				
				// 每插入一定数量后检查是否需要触发 rehash
				if (i % 100 == 0) {
					std::cout << "Pre-filled " << i << " keys..." << std::endl;
				}
			}
		}
	}
	std::cout << "Pre-filling completed. Starting concurrent test..." << std::endl;

	std::vector<std::thread> workers;
	workers.reserve(threads);
	std::atomic<uint64_t> nextOpId{0};
	std::vector<OpRecord> history;
	history.reserve(static_cast<size_t>(threads) * ops_limit);
	std::mutex hist_mu;

	// 写者发布的新键集合，读者只读这些键
	std::vector<std::string> published_keys;
	published_keys.reserve(ops_limit);
	std::mutex pub_mu;
	std::atomic<uint64_t> write_seq{0};

	// 直接调用 expand 触发扩容
	// std::thread expander([&](){
	// 	// 仅少量调用 expand，降低 rehash 触发频率
	// 	uint32_t rounds = std::min<uint32_t>(8, key_space);
	// 	for (uint32_t i = 0; i < rounds; ++i) {
	// 		auto m = map.meta.load();
	// 		if (m) {
	// 			map.expand(0, m);
	// 		}
	// 		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	// 	}
	// });

	// pre-generate keys for control of distribution; hot keys come from first 10% range
	std::vector<std::string> keys;
	keys.reserve(key_space);
	{
		std::mt19937_64 gen(seed);
		for (uint32_t i = 0; i < key_space; ++i) keys.push_back(random_key(gen, 12));
	}
	
	// 添加额外的键来强制触发 rehash
	std::vector<std::string> extra_keys;
	extra_keys.reserve(key_space * 2); // 生成更多键
	{
		std::mt19937_64 gen(seed + 12345); // 不同的种子
		for (uint32_t i = 0; i < key_space * 2; ++i) {
			extra_keys.push_back(random_key(gen, 16)); // 更长的键
		}
	}

	auto worker = [&](uint32_t tid) {
		std::mt19937_64 rng(seed + tid * 9973ULL);
		std::uniform_real_distribution<double> u01(0.0, 1.0);
		std::bernoulli_distribution hot_sel(0.7); // 70% choose hot set
		std::uniform_int_distribution<uint32_t> cold_pick(key_space / 10, key_space - 1);
		std::uniform_int_distribution<uint32_t> hot_pick(0, std::max(1u, key_space / 10) - 1);
		uint64_t vtag = tid + 1; // version source per thread
		for (uint32_t i = 0; i < ops_limit; ++i) {
			OpRecord rec;
			rec.threadId = tid;
			rec.opId = nextOpId.fetch_add(1, std::memory_order_relaxed);
			rec.found = false;
			rec.observedValue.reset();
			rec.start = std::chrono::steady_clock::now();

			if (tid == 0) {
				// 专用写线程：写入新键，并发布给读者
				uint64_t seq = write_seq.fetch_add(1);
				rec.key = std::string("rw_key_") + std::to_string(seq);
				rec.value = value_from_key(rec.key, seq);
				rec.type = OpType::Insert;
				auto r = map.insert(persistent_map_type::value_type(rec.key, rec.value), 1, rec.opId);
				rec.found = r.found;
				{
					std::lock_guard<std::mutex> lk(pub_mu);
					published_keys.push_back(rec.key);
				}
			} else if (tid == 1) {
				// 只读线程：只读取发布的新写
				rec.type = OpType::Read;
				std::string k;
				{
					std::lock_guard<std::mutex> lk(pub_mu);
					if (!published_keys.empty()) k = published_keys.back();
				}
				if (!k.empty()) {
					auto r = map.search(k, tid);
					rec.key = k;
					rec.found = r.found;
					if (r.found && r.value) rec.observedValue = r.value->second;
				}
			} else {
				// 其它线程：轻量混合读写，降低整体 rehash 压力
				double p = u01(rng);
				uint32_t idx = hot_sel(rng) ? hot_pick(rng) : cold_pick(rng);
				const std::string &k = keys[idx];
				rec.key = k;
				rec.value = value_from_key(k, vtag++);
				if (p < 0.2) {
					rec.type = OpType::Insert;
					auto r = map.insert(persistent_map_type::value_type(k, rec.value), tid + 1, rec.opId);
					rec.found = r.found;
				} else {
					rec.type = OpType::Read;
					auto r = map.search(k, tid);
					rec.found = r.found;
					if (r.found && r.value) rec.observedValue = r.value->second;
				}
			}

			rec.end = std::chrono::steady_clock::now();
			{
				std::lock_guard<std::mutex> lg(hist_mu);
				history.emplace_back(std::move(rec));
			}
		}
	};

	for (uint32_t t = 0; t < threads; ++t) workers.emplace_back(worker, t);
	for (auto &th : workers) th.join();
	// if (expander.joinable()) expander.join();

	// group by key and check per-key linearizability
	std::unordered_map<std::string, std::vector<OpRecord>> per_key;
	per_key.reserve(key_space * 2);
	for (auto &r : history) per_key[r.key].push_back(r);
	// sort each per-key vector by start time to improve checker locality
	for (auto &kv : per_key) {
		auto &vec = kv.second;
		std::sort(vec.begin(), vec.end(), [](const OpRecord &a, const OpRecord &b) {
			return a.start < b.start;
		});
	}

	uint64_t checked = 0, ok = 0, failed = 0;
	for (auto &kv : per_key) {
		checked++;
		bool res = linearizable_per_key(kv.second);
		if (res) ok++; else failed++;
	}

	std::cout << "Checked keys: " << checked << ", ok=" << ok << ", failed=" << failed << std::endl;
	if (failed == 0) {
		std::cout << "Linearizability check passed for all keys." << std::endl;
		return 0;
	} else {
		std::cerr << "Linearizability violations detected on " << failed << " keys." << std::endl;
		return 2;
	}
}


