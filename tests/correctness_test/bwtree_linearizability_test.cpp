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
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "BwTree/test/test_suite.h"
#include "shm/cxl_type.h"
#include "shm/mm.h"

using namespace std;

// 定义BwTree的键值类型
using KeyType = uint64_t;
using ValueType = uint64_t;
using BwTreeType = TreeType;

enum class OpType { Insert, Read, Delete, Update };

struct OpRecord {
    // 不可变信息
    uint32_t threadId;
    uint64_t opId;  // 全局操作ID，用于排序参考
    KeyType key;
    ValueType value;  // 写/更新的新值
    OpType type;
    
    // 时间信息
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point end;
    
    // 观察结果
    bool found;
    std::optional<ValueType> observedValue;  // 读取操作：如果找到则返回值
};

// 生成随机键
static KeyType random_key(std::mt19937_64 &rng, uint32_t key_space) {
    std::uniform_int_distribution<KeyType> dist(0, key_space - 1);
    return dist(rng);
}

// 从键生成值
static ValueType value_from_key(const KeyType &k, uint64_t vtag) {
    return k + vtag;
}

// 线性一致性检查上下文
struct LinearCheckCtx {
    // 前驱图：如果i在j开始前完成，则i必须在j之前
    std::vector<std::vector<int>> preds;  // 每个操作的前驱列表
    std::vector<int> indeg;  // 动态入度
};

// 应用操作并验证状态
static bool apply_and_validate(const OpRecord &op, std::optional<ValueType> &state) {
    switch (op.type) {
        case OpType::Insert:
            if (state.has_value()) {
                return op.found == true;  // 之前已存在
            } else {
                state = op.value;  // 新值等于提供的值
                return op.found == false;
            }
            
        case OpType::Read:
            if (state.has_value()) {
                return op.found == true && op.observedValue == state;
            } else {
                return op.found == false;
            }
            
        case OpType::Update:
            if (state.has_value()) {
                // 更新成功
                if (op.found != true) return false;
                state = op.value;
                return true;
            } else {
                // 对不存在的键进行更新
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

// 检查单个键的线性一致性
static bool linearizable_per_key(const std::vector<OpRecord> &ops) {
    const int n = (int)ops.size();
    if (n == 0) return true;

    LinearCheckCtx g;
    g.preds.assign(n, {});
    g.indeg.assign(n, 0);
    
    // 构建前驱图：如果i在j开始前完成，则i是j的前驱
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            // 实时顺序：如果i在j开始前完成，则i在j之前
            if (ops[i].end <= ops[j].start) {
                g.preds[j].push_back(i);
                g.indeg[j]++;
            }
        }
    }

    // 带前驱约束的回溯算法
    std::vector<char> placed(n, 0);
    std::vector<int> order;
    order.reserve(n);
    std::optional<ValueType> state;

    std::function<bool()> dfs = [&]() -> bool {
        if ((int)order.size() == n) return true;
        
        // 收集入度为0且未放置的候选操作
        for (int i = 0; i < n; ++i) {
            if (placed[i] || g.indeg[i] != 0) continue;
            
            // 尝试放置操作i
            placed[i] = 1;
            order.push_back(i);
            
            // 保存状态
            auto saved_state = state;
            
            // 减少后继操作的入度（模拟移除）
            for (int k = 0; k < n; ++k) {
                // 如果i是k的前驱，减少入度
                for (int p : g.preds[k]) {
                    if (p == i) {
                        g.indeg[k]--;
                        break;
                    }
                }
            }
            
            bool ok = apply_and_validate(ops[i], state);
            if (ok && dfs()) return true;
            
            // 回溯
            state = saved_state;
            for (int k = 0; k < n; ++k) {
                for (int p : g.preds[k]) {
                    if (p == i) {
                        g.indeg[k]++;
                        break;
                    }
                }
            }
            order.pop_back();
            placed[i] = 0;
        }
        return false;
    };

    // 启发式：按结束时间排序，优先尝试较小的结束时间
    std::vector<int> idx(n);
    for (int i = 0; i < n; ++i) idx[i] = i;
    std::stable_sort(idx.begin(), idx.end(), [&](int a, int b) {
        return ops[a].end < ops[b].end;
    });
    
    return dfs();
}

// 线程分配和释放管理（参考bwtree_db.cc）
class ThreadManager {
private:
    cxl_vector<std::atomic<uint8_t>> bits;
    uint32_t thread_num;

public:
    ThreadManager(uint32_t num) : thread_num(num), bits(num) {
        for (uint32_t i = 0; i < num; ++i) {
            bits[i].store(0);
        }
    }

    int allocate() {
        for (uint32_t i = 0; i < thread_num; ++i) {
            uint8_t expected = 0;
            if (bits[i].compare_exchange_strong(expected, 1)) {
                return i;
            }
        }
        std::cerr << "Failed to allocate thread ID" << std::endl;
        return -1;
    }

    void release(int id) {
        if (id >= 0 && id < (int)thread_num) {
            bits[id].store(0);
        }
    }
};

int main(int argc, char **argv) {
    // 参数解析
    uint32_t threads = 4;
    uint32_t ops_per_thread = 100;
    uint32_t key_space = 10000;
    uint64_t seed = 42;
    
    if (argc >= 2) threads = std::stoul(argv[1]);
    if (argc >= 3) ops_per_thread = std::stoul(argv[2]);
    if (argc >= 4) key_space = std::stoul(argv[3]);
    if (argc >= 5) seed = std::stoull(argv[4]);
    
    // 限制单线程操作次数以缩短运行时间
    uint32_t ops_limit = std::min<uint32_t>(ops_per_thread, 1000);
    
    // 验证参数合理性
    if (threads == 0 || ops_limit == 0 || key_space == 0) {
        std::cerr << "Invalid parameters: threads=" << threads 
                  << ", ops_limit=" << ops_limit 
                  << ", key_space=" << key_space << std::endl;
        return 1;
    }

    // 初始化SimThreadInfo
    SimThreadInfo::setup_worker_machine_count(1);
    SimThreadInfo::setup_machine_ids(0);
    SimThreadInfo::setup_worker_nr(threads);
    for (uint32_t i = 0; i < threads; ++i) {
        SimThreadInfo::setup_worker_ids(0, i);
    }
    
    // 初始化memkind内存池 - 这是关键！避免memkind_malloc段错误
    const char* shm_path = "/dev/shm/cxl";
    size_t mem_size = 1024 * 1024 * 1024; // 1GB
    void* base = nullptr;
    
    try {
        init_cacheable_allocator(shm_path, base, mem_size);
        if (base == nullptr) {
            std::cerr << "Failed to initialize memory allocator" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Memory allocator initialization failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "BwTree Linearizability Test" << std::endl;
    std::cout << "threads=" << threads
              << " ops_per_thread=" << ops_per_thread
              << " key_space=" << key_space
              << " seed=" << seed << std::endl;

    // 创建BwTree实例，参考bwtree_db.cc的写法
    BwTreeType* tree = nullptr;
    try {
        tree = GetEmptyTree(true);
        if (tree == nullptr) {
            std::cerr << "Failed to create BwTree instance" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "BwTree creation failed: " << e.what() << std::endl;
        return 1;
    }
    
    // 设置线程数并更新线程本地资源
    try {
        tree->UpdateThreadLocal(threads);
    } catch (const std::exception& e) {
        std::cerr << "Failed to update thread local resources: " << e.what() << std::endl;
        DestroyTree(tree, true);
        return 1;
    }
    
    // 创建线程管理器
    ThreadManager thread_mgr(threads);

    // 预填充数据
    std::cout << "Pre-filling data..." << std::endl;
    try {
        std::mt19937_64 prefill_gen(seed + 999);
        uint32_t prefill_limit = std::min<uint32_t>(key_space / 10, 1000);
        for (uint32_t i = 0; i < prefill_limit; ++i) {
            KeyType prefill_key = prefill_gen() % key_space;
            ValueType prefill_val = value_from_key(prefill_key, i);
            bool success = tree->Insert(prefill_key, prefill_val);
            if (!success) {
                std::cerr << "Warning: Pre-fill insert failed for key " << prefill_key << std::endl;
            }
            
            if (i % 100 == 0) {
                std::cout << "Pre-filled " << i << " keys..." << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Pre-filling failed: " << e.what() << std::endl;
        DestroyTree(tree, true);
        return 1;
    }
    std::cout << "Pre-filling completed. Starting concurrent test..." << std::endl;

    std::vector<std::thread> workers;
    workers.reserve(threads);
    std::atomic<uint64_t> nextOpId{0};
    std::vector<OpRecord> history;
    history.reserve(static_cast<size_t>(threads) * ops_limit);
    std::mutex hist_mu;

    // 写者发布的新键集合，读者只读这些键
    std::vector<KeyType> published_keys;
    published_keys.reserve(ops_limit);
    std::mutex pub_mu;
    std::atomic<uint64_t> write_seq{0};

    // 预生成键以控制分布
    std::vector<KeyType> keys;
    keys.reserve(key_space);
    {
        std::mt19937_64 gen(seed);
        for (uint32_t i = 0; i < key_space; ++i) {
            keys.push_back(gen() % key_space);
        }
    }

    auto worker = [&](uint32_t tid) {
        // 为线程分配GC ID并注册到BwTree
        int thread_id = thread_mgr.allocate();
        if (thread_id < 0) {
            std::cerr << "Thread " << tid << " failed to allocate GC ID, skipping..." << std::endl;
            return;  // 直接返回，不执行任何操作
        }
        
        // 注册到BwTree
        tree->AssignGCID(thread_id);
        
        std::mt19937_64 rng(seed + tid * 9973ULL);
        std::uniform_real_distribution<double> u01(0.0, 1.0);
        std::bernoulli_distribution hot_sel(0.7);  // 70%选择热键集
        std::uniform_int_distribution<uint32_t> cold_pick(key_space / 10, key_space - 1);
        std::uniform_int_distribution<uint32_t> hot_pick(0, std::max(1u, key_space / 10) - 1);
        uint64_t vtag = tid + 1;  // 每个线程的版本源
        
        for (uint32_t i = 0; i < ops_limit; ++i) {
            OpRecord rec;
            rec.threadId = tid;
            rec.opId = nextOpId.fetch_add(1, std::memory_order_relaxed);
            rec.found = false;
            rec.observedValue.reset();
            rec.start = std::chrono::steady_clock::now();

            try {
                if (tid == 0) {
                    // 专用写线程：写入新键，并发布给读者
                    uint64_t seq = write_seq.fetch_add(1);
                    rec.key = seq;
                    rec.value = value_from_key(rec.key, seq);
                    rec.type = OpType::Insert;
                    rec.found = tree->Insert(rec.key, rec.value);
                    {
                        std::lock_guard<std::mutex> lk(pub_mu);
                        published_keys.push_back(rec.key);
                    }
                } else if (tid == 1) {
                    // 只读线程：只读取发布的新写
                    rec.type = OpType::Read;
                    KeyType k = 0;
                    {
                        std::lock_guard<std::mutex> lk(pub_mu);
                        if (!published_keys.empty()) {
                            k = published_keys.back();
                        }
                    }
                    if (k != 0) {
                        rec.key = k;
                        std::vector<long> values;
                        tree->GetValue(k, values);
                        rec.found = !values.empty();
                        if (rec.found && !values.empty()) {
                            rec.observedValue = values[0];
                        }
                    }
                } else {
                    // 其它线程：轻量混合读写
                    double p = u01(rng);
                    uint32_t idx = hot_sel(rng) ? hot_pick(rng) : cold_pick(rng);
                    const KeyType &k = keys[idx];
                    rec.key = k;
                    rec.value = value_from_key(k, vtag++);
                    
                    if (p < 0.2) {
                        rec.type = OpType::Insert;
                        rec.found = tree->Insert(k, rec.value);
                    } else if (p < 0.4) {
                        rec.type = OpType::Delete;
                        // 修复：Delete只需要key参数
                        rec.found = tree->Delete(k, k);  // 使用key作为value参数（BwTree要求）
                    } else {
                        rec.type = OpType::Read;
                        std::vector<long> values;
                        tree->GetValue(k, values);
                        rec.found = !values.empty();
                        if (rec.found && !values.empty()) {
                            rec.observedValue = values[0];
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Thread " << tid << " operation " << i << " failed: " << e.what() << std::endl;
                rec.found = false;
                rec.observedValue.reset();
            }

            rec.end = std::chrono::steady_clock::now();
            {
                std::lock_guard<std::mutex> lg(hist_mu);
                history.emplace_back(std::move(rec));
            }
        }
        
        // 线程结束时取消注册并释放GC ID
        tree->UnregisterThread(thread_id);
        thread_mgr.release(thread_id);
    };

    // 启动工作线程
    for (uint32_t t = 0; t < threads; ++t) {
        workers.emplace_back(worker, t);
    }
    
    // 等待所有线程完成
    for (auto &th : workers) {
        th.join();
    }

    // 按键分组并检查每个键的线性一致性
    std::unordered_map<KeyType, std::vector<OpRecord>> per_key;
    per_key.reserve(key_space * 2);
    for (auto &r : history) {
        per_key[r.key].push_back(r);
    }
    
    // 按开始时间排序每个键的操作向量，以改善检查器局部性
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
        if (res) {
            ok++;
        } else {
            failed++;
        }
    }

    std::cout << "Linearizability check completed." << std::endl;
    std::cout << "Checked keys: " << checked << ", ok=" << ok << ", failed=" << failed << std::endl;
    
    // 清理BwTree实例，参考bwtree_db.cc的写法
    tree->UpdateThreadLocal(1);  // 重置线程本地资源
    DestroyTree(tree, true);     // 正确清理
    
    if (failed == 0) {
        std::cout << "✅ Linearizability check passed for all keys." << std::endl;
        return 0;
    } else {
        std::cerr << "❌ Linearizability violations detected on " << failed << " keys." << std::endl;
        return 1;
    }
}
