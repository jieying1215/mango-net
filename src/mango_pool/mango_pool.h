#ifndef MANGO_POOL_H
#define MANGO_POOL_H
#include<mutex>
#include <cstddef>  // size_t

namespace mango_pool {

// 内存块头部：每个内存块前面都带一个元数据头
struct BlockHeader {
    size_t size;          // 整个块的总大小（包含头部自身）
    bool is_free;         // 块是否处于空闲状态
    BlockHeader* next;    // 下一个块的指针，单向链表

    // 工具函数：返回用户可用的数据区起始地址
    char* data() {
        return reinterpret_cast<char*>(this) + sizeof(BlockHeader);
    }
};

// 内存池主类
class MangoPool {
private:
    BlockHeader* free_list_;  // 空闲链表头指针
    void* pool_start_;        // 整个内存池的起始地址
    size_t pool_capacity_;    // 内存池总容量
    mutable std::mutex mutex_; //新增:保护空闲链表

public:
    // 构造：传入内存池总大小，一次性向OS申请
    explicit MangoPool(size_t capacity);
    // 析构：把整块内存归还OS
    ~MangoPool();

    // 分配内存，返回数据区指针；失败返回nullptr
    void* allocate(size_t bytes);
    // 释放内存，传入数据区指针
    void deallocate(void* p);

    // 禁止拷贝，避免 mmap 的指针被双重释放
    MangoPool(const MangoPool&) = delete;
    MangoPool& operator=(const MangoPool&) = delete;

    //新增，统计信息，方便压测和调试
    size_t free_bytes() const;
    size_t capacity() const;
};

} // namespace mango_pool

#endif // MANGO_POOL_H