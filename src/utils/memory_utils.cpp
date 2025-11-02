#include "utils/memory_utils.h"
#include "utils/logger.h"

// 安全的内存分配
uint8_t* MemoryManager::safeMalloc(size_t size, const char* name) {
    if (size == 0) {
        LOG_MEMORY_WARN("Attempting to allocate 0 bytes for %s", name);
        return nullptr;
    }
    
    // 检查可用内存
    size_t freeHeap = ESP.getFreeHeap();
    size_t maxAlloc = ESP.getMaxAllocHeap();
    
    if (size > maxAlloc) {
        LOG_MEMORY_ERROR("Request %u bytes for %s, but max allocable is %u", 
                     size, name, maxAlloc);
        return nullptr;
    }
    
    if (size > freeHeap * 0.8) {  // 如果请求超过80%的可用内存，给出警告
        LOG_MEMORY_WARN("Large allocation %u bytes for %s (%.1f%% of free heap)", 
                     size, name, (float)size / freeHeap * 100);
    }
    
    uint8_t* ptr = (uint8_t*)malloc(size);
    if (!ptr) {
        LOG_MEMORY_ERROR("Failed to allocate %u bytes for %s", size, name);
        LOG_MEMORY_ERROR("Available: Free=%u, MaxAlloc=%u", freeHeap, maxAlloc);
    } else {
        LOG_MEMORY_VERBOSE("Allocated %u bytes for %s (Free: %u -> %u)", 
                     size, name, freeHeap, ESP.getFreeHeap());
    }
    
    return ptr;
}

// 安全的内存释放
void MemoryManager::safeFree(uint8_t*& ptr) {
    if (ptr != nullptr) {
        size_t freeBefore = ESP.getFreeHeap();
        free(ptr);
        ptr = nullptr;
        LOG_MEMORY_VERBOSE("Memory freed (Free: %u -> %u)", 
                     freeBefore, ESP.getFreeHeap());
    }
}

// RAII内存管理类实现
MemoryManager::SafeBuffer::SafeBuffer(size_t size, const char* name) 
    : buffer_(nullptr), size_(size), name_(name) {
    buffer_ = MemoryManager::safeMalloc(size, name);
}

MemoryManager::SafeBuffer::~SafeBuffer() {
    if (buffer_) {
        LOG_MEMORY_VERBOSE("SafeBuffer '%s' destructor - freeing %u bytes", name_, size_);
        MemoryManager::safeFree(buffer_);
    }
}

// 获取当前可用堆内存
size_t MemoryManager::getFreeHeap() {
    return ESP.getFreeHeap();
}

// 获取最大可分配的连续内存块
size_t MemoryManager::getMaxAllocHeap() {
    return ESP.getMaxAllocHeap();
}

// 打印内存使用情况
void MemoryManager::printMemoryInfo(const char* context) {
    LOG_MEMORY_INFO_MSG("%s - Free: %u bytes, Max Alloc: %u bytes, Min Free: %u bytes",
                 context, ESP.getFreeHeap(), ESP.getMaxAllocHeap(), ESP.getMinFreeHeap());
}