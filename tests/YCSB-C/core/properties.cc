#include "core/properties.h"

namespace utils {
std::string ParseCommandLine(int argc, const char *argv[],
                             utils::Properties &props) {
  props.SetProperty("program_path", argv[0]);
  int argindex = 1;
  std::string filename;
  while (argindex < argc && StrStartWith(argv[argindex], "-")) {
    if (strcmp(argv[argindex], "-share_var") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("share_var", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-client_threads") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("client_thread_count", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-server_threads") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("server_thread_count", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-db") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("dbname", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-host") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("host", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-port") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("port", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-slaves") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("slaves", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-machineno") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("machineno", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-dbnum") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("dbnum", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-machinenum") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("machinenum", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-follower_list") == 0) {
      argindex++;
      props.SetProperty("follower_list", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-tracename") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("tracename", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-workloadname") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("workloadname", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-tracepath") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("tracepath", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-P") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("workload_file", argv[argindex]);
      filename.assign(argv[argindex]);
      std::ifstream input(argv[argindex]);
      try {
        props.Load(input);
      } catch (const std::string &message) {
        std::cout << message << std::endl;
        exit(0);
      }
      input.close();
      argindex++;
    } else if (strcmp(argv[argindex], "-C") == 0) {
      argindex++;
      props.SetProperty("config_file", argv[argindex]);
      argindex++;
    } else {
      std::cout << "Unknown option '" << argv[argindex] << "'" << std::endl;
      exit(0);
    }
  }

  if (argindex == 1 || argindex != argc) {
    UsageMessage(argv[0]);
    exit(0);
  }

  return filename;
}

inline void UsageMessage(const char *command) {
  std::cout << "Usage: " << command << " [options]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -client_threads n: client side threads (default: 1)"
            << std::endl;
  std::cout << "  -server_threads n: server side threads (default: 1)"
            << std::endl;
  std::cout
      << "  -db dbname: specify the name of the DB to use (default: basic)"
      << std::endl;
  std::cout
      << "  -P propertyfile: load properties from the given file. Multiple "
         "files can"
      << std::endl;
  std::cout
      << "                   be specified, and will be processed in the order "
         "specified"
      << std::endl;
}
}; // namespace utils