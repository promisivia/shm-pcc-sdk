#include "utils/init.h"

#include <iostream>

#include "msg/msg_queue.h"
#include "shm/mempool.h"
#include "utils/config.h"
#include "shm/mm.h"

void initialize_shm_related(int machine_no) {
  // initialize_shm();
  // init_cacheable_allocator(machine_no);
  std::cout << "initialize shm and memkind finished" << std::endl;

#ifdef USE_COMM_MSG_QUEUE
  init_msg_queue();
  initialize_comm(machine_no);
  std::cout << "initialize msg queue finished" << std::endl;
#endif
  return;
}