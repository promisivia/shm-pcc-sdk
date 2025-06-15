#pragma once

namespace ycsbc {
class TransactionThreadController;
}

struct GlobalVariables {
  ycsbc::TransactionThreadController *trx_thread_controller;
};

extern GlobalVariables *g_var_struct;