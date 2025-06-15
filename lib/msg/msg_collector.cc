#include "msg/msg_collector.h"

MsgCollector* g_collector[MSG_TYPE_NUM][NUM_CLIENTS];

MsgCollector::MsgCollector(void* q_addr) : queue(q_addr) {
    // Initialize any necessary variables or data structures
}

MsgCollector::~MsgCollector() {}

void MsgCollector::sendMsg(msg_node_t* node) {
    q_lock.lock();
    queue.enqueue_msg(node);
    q_lock.unlock();
}
