#pragma once
#include "core/properties.h"
#include "follower/global_variable.h"
#include "utils/sim_id.h"
#include <cstdint>
#include <filesystem>
#include <linux/limits.h>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>

#define MAX_DB_COUNT 16

class CommandBuilder {
public:
  static std::string build(utils::Properties &props, uint32_t machine_id) {
    std::stringstream ss;
    char program_path_char[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", program_path_char, PATH_MAX);

    if (count == -1) {
      std::cerr << "Error reading the symbolic link" << std::endl;
      exit(EXIT_FAILURE);
    }

    program_path_char[count] = '\0'; // null-terminate the result
    
    namespace fs = std::filesystem;
    fs::path program_path(program_path_char);
    fs::path program_dir = program_path.parent_path().parent_path();
    ss << "cd " << program_dir << " && ";
    ss << "ulimit -c unlimited && ";
    ss << "LD_LIBRARY_PATH=" << program_dir.parent_path().parent_path() << "/shared_libs ";
    printf("master shared var=%p\n", g_var_struct);
    ss << "./ycsbc" << " -machineno " << machine_id << " -machinenum "
       << SimThreadInfo::worker_machine_count << " -client_threads "
       << props.GetProperty("client_thread_count") << " -server_threads "
       << props.GetProperty("server_thread_count") << " -db "
       << props.GetProperty("dbname") << " -share_var " << std::hex
       << g_var_struct << std::dec << " -C config.ini ";
    bool use_real_trace = (props.GetProperty("tracename", "") != "");
    if (use_real_trace) {
      ss << " -tracepath " << props.GetProperty("tracepath") << " -tracename"
         << props.GetProperty("tracename") << " -workloadname "
         << props.GetProperty("workloadname");
    } else {
      ss << " -P " << props.GetProperty("workload_file");
    }
    ss << "| tee -a ./log/followers/output-" << machine_id << ".log";
    return ss.str();
  }
};