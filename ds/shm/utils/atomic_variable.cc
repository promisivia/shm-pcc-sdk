#include "utils/atomic_variable.h"
#include <sys/mman.h>

#ifdef STAT_NT_POINTER
int Statistic::atomic_cas_counter = 0;
int Statistic::atomic_load_counter = 0;
int Statistic::get_node_load_counter = 0;
int Statistic::read_in_use_flag = 0;
std::map<std::string, int> Statistic::function_count;
#endif
