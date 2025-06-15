// #pragma once
// #include "connection/establish.h"
// #include "stale.h"

// void test_lock_free_ring_buffer(void *custom_ds) {
//   buffer_req *req = (buffer_req *)custom_ds;
//   create_stale_list(req->buffer, req->size, req->head, req->tail);
//   if (SimThreadInfo::worker_machine_id == 0) {
//     for (int i = 0; i < 100; i++) {
//       add_stale((void *)i);
//     }
//   } else {
//   }
// }