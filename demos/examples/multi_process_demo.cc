#include "pcc/btree.h"
#include "pcc/init.h"

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

void process_creator() {
    std::cout << "[Creator Process " << getpid() << "] Starting..." << std::endl;
    
    // Initialize shared memory
    pcc::ShmConfig config;
    config.shm_path = "/dev/dax1.0";
    config.shm_id = "multi_proc_demo";
    config.shm_size = 1024ULL * 1024 * 1024; // 1GB
    config.thread_num = 1;
    config.is_creator = true;
    
    if (!pcc::init_shm(config)) {
        std::cerr << "Failed to initialize shared memory" << std::endl;
        return;
    }
    
    // Create BwTree
    std::string tree_id = pcc::get_shm_id("bwtree", 789);
    pcc::btree* tree = pcc::btree::init(tree_id, true, 1);
    
    if (!tree) {
        std::cerr << "Failed to create BwTree" << std::endl;
        pcc::cleanup_shm();
        return;
    }
    
    tree->thread_init(0);
    
    // Insert initial data
    std::cout << "[Creator] Inserting initial data..." << std::endl;
    for (int64_t i = 1; i <= 5; ++i) {
        tree->insert(i, i * 10);
        std::cout << "[Creator] Inserted (" << i << ", " << (i * 10) << ")" << std::endl;
    }
    
    // Wait a bit for other process to attach
    sleep(2);
    
    // Insert more data
    std::cout << "[Creator] Inserting more data..." << std::endl;
    for (int64_t i = 6; i <= 10; ++i) {
        tree->insert(i, i * 10);
        std::cout << "[Creator] Inserted (" << i << ", " << (i * 10) << ")" << std::endl;
    }
    
    // Wait for other process to finish
    sleep(2);
    
    // Read all data
    std::cout << "[Creator] Reading all data..." << std::endl;
    for (int64_t i = 1; i <= 15; ++i) {
        int64_t value;
        if (tree->find(i, value)) {
            std::cout << "[Creator] Key " << i << " -> Value " << value << std::endl;
        }
    }
    
    // Cleanup
    delete tree;
    pcc::cleanup_shm();
    
    std::cout << "[Creator] Done" << std::endl;
}

void process_attacher() {
    std::cout << "[Attacher Process " << getpid() << "] Starting..." << std::endl;
    
    // Wait a bit for creator to initialize
    sleep(1);
    
    // Initialize shared memory (as attacher) - use same DAX device
    pcc::ShmConfig config;
    config.shm_path = "/dev/dax1.0";  // Use same DAX device as creator
    config.shm_id = "multi_proc_demo";
    config.shm_size = 1024ULL * 1024 * 1024; // 1GB
    config.thread_num = 1;
    config.is_creator = false;
    
    if (!pcc::init_shm(config)) {
        std::cerr << "Failed to initialize shared memory" << std::endl;
        return;
    }
    
    // Attach to existing BwTree
    std::string tree_id = pcc::get_shm_id("bwtree", 789);
    pcc::btree* tree = pcc::btree::init(tree_id, false, 1);
    
    if (!tree) {
        std::cerr << "Failed to attach to BwTree" << std::endl;
        pcc::cleanup_shm();
        return;
    }
    
    std::cout << "[Attacher] Calling thread_init..." << std::endl;
    tree->thread_init(0);
    std::cout << "[Attacher] thread_init completed" << std::endl;
    
    // Wait a bit for creator to finish inserting initial data
    std::cout << "[Attacher] Waiting for creator to finish inserting..." << std::endl;
    sleep(1);
    
    // Read existing data
    std::cout << "[Attacher] Reading existing data..." << std::endl;
    std::cout.flush(); // Ensure output is flushed
    for (int64_t i = 1; i <= 5; ++i) {
        int64_t value;
        std::cout << "[Attacher] Trying to find key " << i << "..." << std::endl;
        std::cout.flush();
        if (tree->find(i, value)) {
            std::cout << "[Attacher] Key " << i << " -> Value " << value << std::endl;
            std::cout.flush();
        } else {
            std::cout << "[Attacher] Key " << i << " not found" << std::endl;
            std::cout.flush();
        }
    }
    
    // Insert new data
    std::cout << "[Attacher] Inserting new data..." << std::endl;
    for (int64_t i = 11; i <= 15; ++i) {
        tree->insert(i, i * 10);
        std::cout << "[Attacher] Inserted (" << i << ", " << (i * 10) << ")" << std::endl;
    }
    
    // Cleanup
    delete tree;
    pcc::cleanup_shm();
    
    std::cout << "[Attacher] Done" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "attacher") {
        process_attacher();
    } else {
        // Fork a child process
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process - attacher
            process_attacher();
            exit(0);
        } else if (pid > 0) {
            // Parent process - creator
            process_creator();
            wait(nullptr);
        } else {
            std::cerr << "Failed to fork process" << std::endl;
            return 1;
        }
    }
    
    return 0;
}

