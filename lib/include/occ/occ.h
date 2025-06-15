#include <cstdint>
#include <unordered_map>
#include <vector>
#include <tbb/concurrent_hash_map.h>

#include "utils/config.h"

namespace occ { 
    using NodePtrType = uint64_t;
    using NodeVersionType = uint64_t;
    using CheckPairMapType = tbb::concurrent_hash_map<NodePtrType, NodeVersionType>;

    /*
     * CheckPair:
     * - This class is used to store the node pointer and the node version.
     */
    using CheckPair = std::pair<NodePtrType, NodeVersionType>;
    // class CheckPair {
    // public:
    //     NodePtrType node_p;
    //     NodeVersionType node_ver;

    //     CheckPair() : node_p(0), node_ver(0) {}
    //     CheckPair(NodePtrType p, NodeVersionType ver) : node_p(p), node_ver(ver) {}

    //     void Set(NodePtrType p, NodeVersionType ver) {
    //         node_p = p;
    //         node_ver = ver;
    //     }
    // };

    /*
     * OCC:
     * - This class is used to store the CheckPairMap.
     */
    class OCC {
    public:
        OCC (int machine_num = 2) {
            total_machine_num = machine_num;
            for (int i = 0; i < total_machine_num; i++) {
                CheckPairMaps.push_back(CheckPairMapType());
            }
        }

        /**
         * Resize:
         * - Resize the CheckPairMap with machine_id.
         * - If the machine_id is greater than the total_machine_num, resize the CheckPairMaps.
         */
        void Resize(int machine_id) {
            if (machine_id >= total_machine_num) {
                total_machine_num = machine_id + 1;
                CheckPairMaps.resize(total_machine_num);
            }
        }

        /**
         * Update:
         * - Update CheckPair with machine_id, node_p, and node_ver.
         */
        void Update(int machine_id, CheckPair *pair) {
            Resize(machine_id);
            CheckPairMapType& CheckPairMap = CheckPairMaps[machine_id];
            CheckPairMapType::accessor accessor;
            if (!CheckPairMap.find(accessor, pair->first)) {
                /* insert new entry */
                CheckPairMap.insert(accessor, pair->first);
            } 
            
            /* update version */
            accessor->second = pair->second;
            // accessor.release();
        }

        /**
         * IsMatch:
         * Check if the CheckPair exists in the CheckPairMap with machine_id and the version is the same.
         * - If the CheckPair does not exist, return true and insert the CheckPair into the CheckPairMap.
         */
        bool IsMatch(int machine_id, CheckPair *pair) {
            Resize(machine_id);
            CheckPairMapType& CheckPairMap = CheckPairMaps[machine_id];
            CheckPairMapType::accessor accessor;
            if (!CheckPairMap.find(accessor, pair->first)) {
                /* insert new entry */
                CheckPairMap.insert(accessor, pair->first);
                accessor->second = pair->second;
                // accessor.release();
                return true;
            }

            return (accessor->second == pair->second);
        }

    private:
        int total_machine_num;
        /* per-machine map */
        std::vector<CheckPairMapType> CheckPairMaps;
    };
}