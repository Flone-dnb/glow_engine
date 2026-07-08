// Include this file in your main.cpp to enable memory leak checks for new/delete.
// Call `start_mem_leak_checks` at the beginning of your main and `finish_mem_leak_checks` in the end of your main.

#if defined(DEBUG) && defined(WIN32)
// silly memory leak checks for windows
// ------------------------------------------------------------------------------------------------
#include <new>
#include <cstddef>
#include <unordered_map>
#include <mutex>
#include <io/log.h>

#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

#define MEM_INFO_FUNC_NAME_SIZE 128
struct mem_info {
    size_t size;
    char func[MEM_INFO_FUNC_NAME_SIZE];
};

static std::unordered_map<void*, mem_info> allocated;
static std::mutex mem_mtx;
static bool mem_check_ignore = true;

void
start_mem_leak_checks() {
    mem_check_ignore = false;

    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
}

void
finish_mem_leak_checks() {
    mem_mtx.lock();
    mem_check_ignore = true;
    if (!allocated.empty()) {
        ge_log_error("");
        ge_log_error("");
        ge_log_error("MEMORY LEAKS DETECTED");
        size_t idx = 1;
        for (auto it = allocated.begin(); it != allocated.end(); it++, idx++) {
            ge_log_error_fmt("%zu. ptr with data of size %zu bytes leaked from %s", idx, it->second.size, it->second.func);
        }
        ge_log_error("");
    } else {
        ge_log_info("no memory leaks detected");
    }
    mem_mtx.unlock();
    mem_check_ignore = true; // don't care about memory after leaving main

    HANDLE process = GetCurrentProcess();
    SymCleanup(process);
}

void*
myalloc(std::size_t size) {
    void* ptr = malloc(size);
    if (ptr != nullptr && !mem_check_ignore) {
        mem_check_ignore = true;
        mem_mtx.lock();

        if (allocated.find(ptr) != allocated.end()) {
            ge_log_error("unexpected state in mem leak checker");
            abort();
        }

        mem_info info;
        info.size = size;
        memset(info.func, 0, MEM_INFO_FUNC_NAME_SIZE);

        void* stack;
        HANDLE process = GetCurrentProcess();
        USHORT frames = CaptureStackBackTrace(2, 1, &stack, NULL);
        SYMBOL_INFO* symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char));
        symbol->MaxNameLen = 255;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        if (frames > 0) {
            DWORD64 address = (DWORD64)(stack);
            SymFromAddr(process, address, 0, symbol);
            memcpy(
                info.func, symbol->Name,
                sizeof(char) * (std::max(symbol->NameLen, (unsigned long)(MEM_INFO_FUNC_NAME_SIZE - 1))));
        }
        free(symbol);

        allocated[ptr] = std::move(info);

        mem_mtx.unlock();
        mem_check_ignore = false;
    }
    return ptr;
}

void
mydealloc(void* ptr) {
    if (ptr != nullptr && !mem_check_ignore) {
        mem_check_ignore = true;
        mem_mtx.lock();

        bool found = false;
        for (auto it = allocated.begin(); it != allocated.end(); it++) {
            if (it->first != ptr) {
                continue;
            }
            found = true;
            allocated.erase(it);
            break;
        }
        if (!found) {
            ge_log_error("unknown pointer in delete");
            abort();
        }

        mem_mtx.unlock();
        mem_check_ignore = false;
    }
    free(ptr);
}

void*
operator new(std::size_t size) {
    void* ptr = myalloc(size);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void*
operator new(std::size_t size, const std::nothrow_t&) noexcept {
    return myalloc(size);
}

void
operator delete(void* ptr) noexcept {
    mydealloc(ptr);
}

#endif