#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "workload.h"

static int load_twitter_workload(char* workload_name,
                                 int num_load_ops,
                                 uint32_t server_id,
                                 uint32_t all_client_num,
                                 RealWorkload* wl) {
  char wl_fname[128];
  sprintf(wl_fname, "../workloads/twitter/%s", workload_name);
  FILE* f = fopen(wl_fname, "r");
  assert(f != NULL);
  printf("client %d loading %s", server_id, workload_name);

  std::vector<std::string> wl_list;
  char buf[2048];
  int ts;
  uint32_t key_size;
  uint32_t val_size;
  uint32_t cid;
  char keybuf[128];
  char opbuf[64];
  int ttl;
  while (fgets(buf, 2048, f) == buf) {
    if (buf[0] == '\n')
      continue;
    sscanf(buf, "%d %s %d %d %d %s %d", &ts, keybuf, &key_size, &val_size, &cid,
           opbuf, &ttl);
    if ((cid % all_client_num) + 1 != server_id) {
      continue;
    }
    wl_list.emplace_back(buf);
  }

  if (num_load_ops == -1) {
    wl->num_ops = wl_list.size();
  } else {
    wl->num_ops = num_load_ops;
  }
  wl->key_buf = malloc(128 * wl->num_ops);
  wl->val_buf = malloc(120 * wl->num_ops);
  wl->key_size_list = (uint32_t*)malloc(sizeof(uint32_t) * wl->num_ops);
  wl->val_size_list = (uint32_t*)malloc(sizeof(uint32_t) * wl->num_ops);
  wl->op_list = (uint8_t*)malloc(sizeof(uint8_t) * wl->num_ops);

  printf("Client %d loading %ld ops\n", server_id, wl_list.size());
  for (int i = 0; i < wl->num_ops; i++) {
    sscanf(wl_list[i].c_str(), "%d %s %d %d %d %s %d", &ts, keybuf, &key_size,
           &val_size, &cid, opbuf, &ttl);
    memcpy((void*)((uint64_t)wl->key_buf + i * 128), keybuf, 128);
    memcpy((void*)((uint64_t)wl->val_buf + i * 120), &i, sizeof(int));
    wl->key_size_list[i] = strlen(keybuf) > 128 ? 128 : strlen(keybuf);
    wl->val_size_list[i] = sizeof(int);
    if (strcmp(opbuf, "get") == 0 || strcmp(opbuf, "gets") == 0) {
      wl->op_list[i] = GET;
    } else {
      wl->op_list[i] = SET;
    }
  }
  return 0;
}

int load_workload(char* workload_name,
                  int num_load_ops,
                  uint32_t server_id,
                  uint32_t all_client_num,
                  RealWorkload* wl) {
  if (memcmp(workload_name, "twitter", strlen("twitter")) == 0) {
    load_twitter_workload(workload_name, num_load_ops, server_id,
                          all_client_num, wl);
  } else {
    printf("Unknown workload %s\n", workload_name);
    return -1;
  }
  return 0;
}

void free_workload(RealWorkload* wl) {
  free(wl->key_buf);
  free(wl->val_buf);
  free(wl->key_size_list);
  free(wl->val_size_list);
  free(wl->op_list);
}