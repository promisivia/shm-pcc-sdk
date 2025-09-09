#include <iostream>
#include <string>
#include <vector>
#include "BwTree/test/test_suite.h"
using namespace wangziqi2013::bwtree;

int main() {
#ifdef BWTREE_AUTODUMP
    g_bwtree_auto_dump_path = "bwtree_dump.txt";
    std::cout << "=== BwTree Tree Visualization Test ===\n\n";
    
    // 创建一个BwTree实例
    BwTree<int, std::string> tree;
    
    std::cout << "Inserting test data...\n";
    
    // 插入一些测试数据
    tree.Insert(10, "value_10");
    tree.Insert(20, "value_20");
    tree.Insert(5, "value_5");
    tree.Insert(15, "value_15");
    tree.Insert(25, "value_25");
    tree.Insert(3, "value_3");
    tree.Insert(7, "value_7");
    tree.Insert(12, "value_12");
    tree.Insert(18, "value_18");
    tree.Insert(30, "value_30");
    
    std::cout << "Data insertion completed.\n\n";
    
    // 打印树结构
    std::cout << "=== Tree Structure ===\n";
    tree.PrintTreeStructure();
    
    std::cout << "\n=== Tree Statistics ===\n";
    tree.PrintTreeStatistics();
    
    // 测试不同的参数
    std::cout << "\n=== Limited Depth View (max_depth = 2) ===\n";
    tree.PrintTreeStructure(std::cout, true, 2);
    
    std::cout << "\n=== Without Delta Chain Info ===\n";
    tree.PrintTreeStructure(std::cout, false);
#endif // BWTREE_AUTODUMP
    return 0;
}
