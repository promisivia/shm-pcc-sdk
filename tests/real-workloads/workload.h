#pragma once

#include <stdint.h>

enum {
    GET,
    SET
};

typedef struct _RealWorkload {
  void* key_buf;
  void* val_buf;
  uint32_t* key_size_list;
  uint32_t* val_size_list;
  uint8_t* op_list;
  uint32_t num_ops;
} RealWorkload;

int load_workload(char* workload_name,
                  int num_load_ops,
                  uint32_t server_id,
                  uint32_t all_client_num,
                  RealWorkload* wl);

void free_workload(RealWorkload* wl);
