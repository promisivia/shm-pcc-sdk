#include <tbb/tbb.h>

#include <cassert>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <iostream>
#include <sstream>

#include "connection/establish.h"
#include "hashmap/hashmap.h"
#include "hashmap/hashmap_test.h"
#include "linkedlist/linkedlist.h"
#include "linkedlist/linkedlist_test.h"
#include "linkedlist/lock_linkedlist_big_sorted.h"
#include "shm/clstale/stale.h"
#include "shm/mempool.h"
#include "utils/cmd_parser.h"

#define LINKED_LIST_TEST 1
#define STALE_CL_TEST 2
#define LF_HASHMAP_TEST 3
// #define TEST_TYPE LINKED_LIST_TEST
#define TEST_TYPE LF_HASHMAP_TEST

volatile int *shutdown_slave;
volatile int *ready;
static bool is_slave = false;
static volatile sig_atomic_t go_quit = 0;
static int quit_pipe[2];
static void *custom_ds;

struct slave_instruct {
  int SimThreadInfo::worker_machine_id;  // machine-specific, need special treatment
  size_t SHM_SIZE;
  size_t SHM_TOTAL_SIZE;
  void *GLOBAL_BASE;
  void *custom_ds;
  volatile int *shutdown_slave;
  volatile int *ready;
  void *stale;
};

// TODO: unwrap instruct
void unwrap_slave_instr(struct slave_instruct *instr) {
  SimThreadInfo::worker_machine_id = instr->SimThreadInfo::worker_machine_id;
  SHM_SIZE = instr->SHM_SIZE;
  SHM_TOTAL_SIZE = instr->SHM_TOTAL_SIZE;
  GLOBAL_BASE = instr->GLOBAL_BASE;
  custom_ds = instr->custom_ds;
  shutdown_slave = instr->shutdown_slave;
  ready = instr->ready;
  create_stale_list(instr->stale, STALE_BUFFER_SIZE);
}

struct slave_instruct *wrap_slave_instr() {
  // TODO: may cause memory leak
  struct slave_instruct *instr = (struct slave_instruct *)memkind_malloc(
      memkind_pool, sizeof(struct slave_instruct));
  instr->SHM_SIZE = SHM_SIZE;
  instr->SHM_TOTAL_SIZE = SHM_TOTAL_SIZE;
  instr->GLOBAL_BASE = GLOBAL_BASE;
  instr->custom_ds = custom_ds;
  instr->shutdown_slave = shutdown_slave;
  instr->ready = ready;
  instr->stale = (void *)stale_list->buffer_;
  return instr;
}

ssh_session build_ssh_connection(const char *host, const char *user) {
  ssh_session session = ssh_new();
  int verbosity = SSH_LOG_WARNING;
  int port = 22;
  if (session == nullptr) {
    perror("ssh_new error");
    exit(EXIT_FAILURE);
  }
  ssh_options_set(session, SSH_OPTIONS_HOST, host);
  ssh_options_set(session, SSH_OPTIONS_USER, user);
  ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
  int rc = ssh_connect(session);
  if (rc != SSH_OK) {
    fprintf(stderr, "Error connecting to %s: %s\n", host,
            ssh_get_error(session));
    ssh_free(session);
    exit(EXIT_FAILURE);
  }
  rc = ssh_userauth_publickey_auto(session, NULL, NULL);
  if (rc != SSH_AUTH_SUCCESS) {
    fprintf(stderr, "Public key authentication failed: %s\n",
            ssh_get_error(session));
    ssh_disconnect(session);
    ssh_free(session);
    exit(EXIT_FAILURE);
  }
  return session;
}

ssh_channel build_ssh_channel(ssh_session session) {
  ssh_channel channel = ssh_channel_new(session);
  if (channel == nullptr) {
    perror("ssh_channel_new error");
    exit(EXIT_FAILURE);
  }
  int rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK) {
    fprintf(stderr, "Error opening channel: %s\n", ssh_get_error(session));
    ssh_channel_free(channel);
    exit(EXIT_FAILURE);
  }
  return channel;
}

void *read_ssh_channel_output(void *arg) {
  int machineIndex = *((int *)arg);
  free(arg);
  ssh_channel channel = slave_channels[machineIndex];
  char buf[8192];
  int nbytes;
  printf("Start listening machine %d\n", machineIndex);
  while (shutdown_slave[machineIndex] == 0 &&
         (nbytes = ssh_channel_read(channel, buf, sizeof(buf), 0)) >= 0) {
    if (nbytes == 0) {
      continue;
    }
    std::cout << "Machine " << machineIndex << ": ";
    std::cout.write(buf, nbytes);
  }
  if (nbytes < 0) {
    printf("Error reading channel: %s\n",
           ssh_get_error(slave_sessions[machineIndex]));
  }
  return NULL;
}

// #define USE_FORK
void start_slave_machine(int argc, char *argv[]) {
  struct slave_instruct *instr = wrap_slave_instr();
  std::ostringstream oss;
  oss << "sleep 20; ";
  for (int i = 0; i < argc; ++i) {
    if (i > 0) {
      oss << " ";
    }
    oss << argv[i];
  }
  oss << " --slave --slave-instr " << "0x" << std::hex << std::uppercase
      << reinterpret_cast<uint64_t>(instr) << " 2>&1";
  std::string commandStr = oss.str();
  const char *command = commandStr.c_str();
  printf("Command: %s\n", command);

  const char *username = getenv("USER");
  if (username == nullptr) {
    perror("Get username error");
    exit(EXIT_FAILURE);
  }
  for (int i = 1; i < NUM_CLIENTS; i++) {
    instr->SimThreadInfo::worker_machine_id = i;

    ssh_session session = build_ssh_connection(machines[i], username);
    slave_sessions[i] = session;
    ssh_channel channel = build_ssh_channel(session);
    slave_channels[i] = channel;
    int rc = ssh_channel_request_exec(channel, command);
    if (rc != SSH_OK) {
      ssh_channel_close(channel);
      ssh_channel_free(channel);
      continue;
    }

    int *arg = (int *)malloc(sizeof(int));
    *arg = i;
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, read_ssh_channel_output, (void *)arg);
    pthread_detach(thread_id);
  }
}

void *monitor_shutdown(void *arg) {
  while (true) {
    if (shutdown_slave[SimThreadInfo::worker_machine_id] == 1) {
      kill(getpid(), SIGINT);
      break;
    }
    sleep(1);
  }
  pthread_exit(NULL);
  return NULL;
}

void parse_argument(int argc, char *argv_ref[]) {
  auto argv = new char *[argc];
  for (int i = 0; i < argc; ++i) {
    argv[i] = strdup(argv_ref[i]);
  }
  Clp_Parser *clp = Clp_NewParser(argc, argv, (int)arraysize(options), options);
  int opt;
  while ((opt = Clp_Next(clp)) >= 0) {
    switch (opt) {
      case opt_slave:
        is_slave = true;
        break;
      case opt_slave_instr: {
        if (!is_slave) {
          perror("Master cannot be a slave\n");
          exit(EXIT_FAILURE);
        }
        const char *instr_addr_str = clp->vstr;
        // Assume 64 bit system
        uint64_t addr_val = std::stoull(instr_addr_str, nullptr, 16);
        auto instr = reinterpret_cast<slave_instruct *>(addr_val);
        unwrap_slave_instr(instr);
        break;
      }
      default:
        fprintf(stderr,
                "Usage: mtd [-np] [--ld dir1[,dir2,...]] [--cd "
                "dir1[,dir2,...]]\n");
        exit(EXIT_FAILURE);
    }
  }
  Clp_DeleteParser(clp);

  for (int i = 0; i < argc; ++i) {
    free(argv[i]);
  }
  delete[] argv;
}

void catchint(int) {
  if (!is_slave) {
    for (int i = 0; i < NUM_CLIENTS; i++) {
      shutdown_slave[i] = 1;
    }
  }
  go_quit = 1;
  char cmd = 0;
  // Does not matter if the write fails (when the pipe is full)
  int r = write(quit_pipe[1], &cmd, sizeof(cmd));
  (void)r;
}

void *canceling(void *) {
  char cmd;
  int r = read(quit_pipe[0], &cmd, sizeof(cmd));
  (void)r;
  assert(r == sizeof(cmd) && cmd == 0);
  // Cancel wake up checkpointing threads

  fprintf(stderr, "\n");
  // cancel outstanding threads. Checkpointing threads will exit safely
  // when the checkpointing thread 0 sees go_quit, and don't need cancel
  if (SimThreadInfo::worker_machine_id == 0) {
    for (int i = 1; i < NUM_CLIENTS; i++) {
      std::string command = "kill -9 " + std::to_string(slave_pid[i]);
      ssh_channel_write(slave_channels[i], command.c_str(), command.length());
      ssh_channel_send_eof(slave_channels[i]);
      ssh_channel_close(slave_channels[i]);
      ssh_channel_free(slave_channels[i]);
      ssh_disconnect(slave_sessions[i]);
      ssh_free(slave_sessions[i]);
    }
  }
  exit(0);
}

int main(int argc, char *argv[]) {
  initialize_shm_and_queue();
  parse_argument(argc, argv);
  if (!is_slave) {
    SimThreadInfo::worker_machine_id = 0;
  }
  initialize_memkind_fixed(SimThreadInfo::worker_machine_id);
  initialize_comm(SimThreadInfo::worker_machine_id);
  // for -pg profiling
  // signal(SIGINT, catchint);

#if TEST_TYPE == LINKED_LIST_TEST
  custom_ds = new_linkedlist();
#elif TEST_TYPE == STALE_CL_TEST
  if (!is_slave) {
    void *buffer;
    cacheable.clalign(memkind_pool, &buffer,
                   STALE_BUFFER_SIZE + (2 + machine_N) * CACHE_LINE_SIZE);
    size_t *head = (size_t *)buffer + STALE_BUFFER_SIZE;
    size_t *tail = head + 1;
    *head = *tail = 0;
    global_process *process = (global_process *)(tail + 1);
    create_stale_list(buffer, STALE_BUFFER_SIZE, head, tail, process);
    custom_ds = stale_list;
  }
#elif TEST_TYPE == LF_HASHMAP_TEST
  using HashMapType = HashMap<uint64_t, uint64_t, LockFreeSortedList>;
  // using HashMapType = HashMap<uint64_t, uint64_t, LockListBigSorted, RWLock>;
  HashMapType *hashmap;
  if (!is_slave) {
    hashmap = new HashMapType();
    custom_ds = hashmap->hashmap_;
  }
  if (is_slave) {
    hashmap = new HashMapType((void **)custom_ds);
  }
#endif

  if (is_slave) {
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, monitor_shutdown, NULL);
    pthread_detach(thread_id);
  }
  if (!is_slave) {
    shutdown_slave =
        (volatile int *)memkind_calloc(memkind_pool, NUM_CLIENTS, sizeof(int));
    ready =
        (volatile int *)memkind_calloc(memkind_pool, NUM_CLIENTS, sizeof(int));
    void *buffer;
    cacheable.clalign(memkind_pool, &buffer,
                   STALE_BUFFER_SIZE + (2 + NUM_CLIENTS) * CACHE_LINE_SIZE);
    create_stale_list(buffer, STALE_BUFFER_SIZE);
    memset(buffer, 0, STALE_BUFFER_SIZE + (2 + NUM_CLIENTS) * CACHE_LINE_SIZE);
    start_slave_machine(argc, argv);
  }
  pthread_t stale_apply;
  pthread_t stale_recycle;
  pthread_create(&stale_apply, NULL, process_stale, NULL);
  if (!is_slave) {
    pthread_create(&stale_recycle, NULL, recycle_stale, NULL);
  }

#ifdef CROSS_MACHINE_LOCK_DELE
  pthread_key_create(&CAS_notify, notify_destruct);
#endif
#if TEST_TYPE == LINKED_LIST_TEST
  test_linkedlist(custom_ds);
#elif TEST_TYPE == STALE_CL_TEST
  pthread_t stale_apply;
  pthread_t stale_recycle;
  pthread_create(&stale_apply, NULL, process_stale, NULL);
  if (!is_slave) {
    pthread_create(&stale_recycle, NULL, recycle_stale, NULL);
  }
  int i;
  for (i = 0; i < 1000; i++) {
    add_stale(&i);
  }
  pthread_join(stale_apply, NULL);
  pthread_join(stale_recycle, NULL);
#elif TEST_TYPE == LF_HASHMAP_TEST
  HashMapTest<HashMapType> test(10000, 0.7, hashmap);
  ready[SimThreadInfo::worker_machine_id] |= 1;
  while (true) {
    bool all_ready = true;
    for (int i = 0; i < NUM_CLIENTS; i++) {
      if ((ready[i] & 1) == 0) {
        all_ready = false;
        break;
      }
    }
    if (all_ready) {
      break;
    }
  }

  std::chrono::steady_clock::time_point begin =
      std::chrono::steady_clock::now();

  // 执行写操作
  test.perform_inserts();
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  std::cout << "Write = "
            << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                     begin)
                   .count()
            << "[µs]" << std::endl;

  // 执行读操作
  begin = std::chrono::steady_clock::now();
  test.perform_reads();
  end = std::chrono::steady_clock::now();
  std::cout << "Read = "
            << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                     begin)
                   .count()
            << "[µs]" << std::endl;

  // // 销毁哈希表
  // destroy_hashmap();
//   while (true);
#endif
}