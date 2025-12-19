#include "db/utils.h"
#include <signal.h>
#include <sys/mman.h>

#include <fstream>

void hex_dump(const char *filename, const void* addr, int len) {
    std::ofstream out(filename);
    const int BYTES_PER_LINE = 16;
    const uint8_t* pc = reinterpret_cast<const uint8_t*>(addr);
    
    if (addr == nullptr) {
        out << "(null)" << std::endl;
        return;
    }

    for (int i = 0; i < len; i += BYTES_PER_LINE) {
        out << std::hex << std::setw(8) << std::setfill('0') << i << ": ";

        for (int j = 0; j < BYTES_PER_LINE; ++j) {
            if (i + j < len) {
                out << std::hex << std::setw(2) << std::setfill('0') 
                          << static_cast<unsigned int>(pc[i + j]) << " ";
            } else {
                out << "   ";
            }
        }

        out << " ";
        for (int j = 0; j < BYTES_PER_LINE; ++j) {
            if (i + j < len) {
                char c = static_cast<char>(pc[i + j]);
                out << (std::isprint(static_cast<unsigned char>(c)) ? c : '.');
            }
        }
        out << std::endl;
    }
}

void watch_handler(int sig, siginfo_t* info, void* ctx) {
    fprintf(stderr, "memory accessed\n");
}

void watch(void* addr, size_t len) {
    uintptr_t page = (uintptr_t)addr & ~(4095);

    mprotect((void*)page, len, PROT_READ);

    struct sigaction sa = {};
    sa.sa_sigaction = watch_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
}
