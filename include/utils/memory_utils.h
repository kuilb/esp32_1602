#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include "myheader.h"

/**
 * @brief 内存管理辅助类，提供统一的内存分配和错误处理
 */
class MemoryManager {
public:
    /**
     * @brief 安全的内存分配，带有错误处理
     * @param size 要分配的字节数
     * @param name 分配内存的描述（用于调试）
     * @return 分配的内存指针，失败时返回nullptr
     */
    static uint8_t* safeMalloc(size_t size, const char* name = "unknown");
    
    /**
     * @brief 安全的内存释放
     * @param ptr 要释放的内存指针的引用
     */
    static void safeFree(uint8_t*& ptr);
    
    /**
     * @brief RAII方式的内存管理类
     */
    class SafeBuffer {
    private:
        uint8_t* buffer_;
        size_t size_;
        const char* name_;
        
    public:
        SafeBuffer(size_t size, const char* name = "SafeBuffer");
        ~SafeBuffer();
        
        // 禁用拷贝构造和赋值
        SafeBuffer(const SafeBuffer&) = delete;
        SafeBuffer& operator=(const SafeBuffer&) = delete;
        
        // 获取缓冲区指针
        uint8_t* get() const { return buffer_; }
        
        // 检查是否分配成功
        bool isValid() const { return buffer_ != nullptr; }
        
        // 获取大小
        size_t size() const { return size_; }
        
        // 获取描述
        const char* name() const { return name_; }
    };
    
    /**
     * @brief 获取当前可用堆内存
     */
    static size_t getFreeHeap();
    
    /**
     * @brief 获取最大可分配的连续内存块
     */
    static size_t getMaxAllocHeap();
    
    /**
     * @brief 打印内存使用情况
     */
    static void printMemoryInfo(const char* context = "");
};

#endif