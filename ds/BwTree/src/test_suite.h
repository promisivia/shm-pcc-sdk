#pragma once

#include <pthread.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <unordered_map>

#include "../benchmark/btree/btree_multimap.h"
#include "../benchmark/art/art.h"
#include "bwtree.h"

using namespace wangziqi2013::bwtree;
using namespace stx;

/*
 * class KeyComparator - Test whether BwTree supports context
 *                       sensitive key comparator
 *
 * If a context-sensitive KeyComparator object is being used
 * then it should follow rules like:
 *   1. There could be no default constructor
 *   2. There MUST be a copy constructor
 *   3. operator() must be const
 *
 */
class KeyComparator {
 public:
  inline bool operator()(const long int k1, const long int k2) const {
    return k1 < k2;
  }

  KeyComparator(int dummy) {
    (void)dummy;

    return;
  }

  KeyComparator() = delete;
  // KeyComparator(const KeyComparator &p_key_cmp_obj) = delete;
};

/*
 * class KeyEqualityChecker - Tests context sensitive key equality
 *                            checker inside BwTree
 *
 * NOTE: This class is only used in KeyEqual() function, and is not
 * used as STL template argument, it is not necessary to provide
 * the object everytime a container is initialized
 */
class KeyEqualityChecker {
 public:
  inline bool operator()(const long int k1, const long int k2) const {
    return k1 == k2;
  }

  KeyEqualityChecker(int dummy) {
    (void)dummy;

    return;
  }

  KeyEqualityChecker() = delete;
  // KeyEqualityChecker(const KeyEqualityChecker &p_key_eq_obj) = delete;
};

using TreeType = BwTree<long int, long int, KeyComparator, KeyEqualityChecker>;
using BTreeType = btree_multimap<long, long, KeyComparator>;
using ARTType = art_tree;

/*
 * Initialize and destroy btree
 */
TreeType *GetEmptyTree(bool no_print = false);
void DestroyTree(TreeType *t, bool no_print = false);

/*
 * Btree
 */
BTreeType *GetEmptyBTree();
void DestroyBTree(BTreeType *t);

void PrintStat(TreeType *t);
void PinToCore(size_t core_id);