#include "mango_pool.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sys/mman.h>

namespace mango_pool {

MangoPool::MangoPool(size_t capacity)
    : free_list_(nullptr), pool_start_(nullptr), pool_capacity_(0) {
    // 容量太小，连一个头部都放不下，直接当作空池
    if (capacity < sizeof(BlockHeader)) {
        return;
    }

    // 匿名映射，不需要文件描述符
    void* mem = mmap(nullptr,
                     capacity,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1,
                     0);
    if (mem == MAP_FAILED) {
        return;
    }

    pool_start_ = mem;
    pool_capacity_ = capacity;

    // 整块内存初始化为一个大的空闲块
    BlockHeader* block = reinterpret_cast<BlockHeader*>(mem);
    block->size = capacity;
    block->is_free = true;
    block->next = nullptr;
    free_list_ = block;
}

MangoPool::~MangoPool() {
    if (pool_start_ != nullptr) {
        munmap(pool_start_, pool_capacity_);
        pool_start_ = nullptr;
        free_list_ = nullptr;
        pool_capacity_ = 0;
    }
}

void* MangoPool::allocate(size_t bytes) {
    if (bytes == 0) return nullptr;

    const size_t total_need = bytes + sizeof(BlockHeader);

    BlockHeader** pcur = &free_list_;
    while (*pcur != nullptr) {
        BlockHeader* cur = *pcur;
        if (cur->size >= total_need) {
            size_t remain_size = cur->size - total_need;

            // 剩余空间还够再放一个头部，则切分成新空闲块
            if (remain_size > sizeof(BlockHeader)) {
                BlockHeader* new_block = reinterpret_cast<BlockHeader*>(
                    reinterpret_cast<char*>(cur) + total_need);
                new_block->size = remain_size;
                new_block->is_free = true;
                new_block->next = cur->next;

                cur->size = total_need;
                cur->next = new_block;
            }

            // 从空闲链表中摘除 cur
            *pcur = cur->next;
            cur->is_free = false;
            return cur->data();
        }
        pcur = &cur->next;
    }

    return nullptr;
}

void MangoPool::deallocate(void* p) {
    if (p == nullptr) return;

    // 由用户数据区反推出块头部
    BlockHeader* block = reinterpret_cast<BlockHeader*>(
        reinterpret_cast<char*>(p) - sizeof(BlockHeader));

    block->is_free = true;

    // 按地址顺序把 block 插入到 free_list_ 中（保持链表按地址有序）
    BlockHeader** pcur = &free_list_;
    BlockHeader* prev = nullptr;
    while (*pcur != nullptr && *pcur < block) {
        prev = *pcur;
        pcur = &(*pcur)->next;
    }

    block->next = *pcur;
    *pcur = block;

    // 与后一个块合并
    if (block->next != nullptr) {
        char* block_end = reinterpret_cast<char*>(block) + block->size;
        if (block_end == reinterpret_cast<char*>(block->next)) {
            block->size += block->next->size;
            block->next = block->next->next;
        }
    }

    // 与前一个块合并
    if (prev != nullptr) {
        char* prev_end = reinterpret_cast<char*>(prev) + prev->size;
        if (prev_end == reinterpret_cast<char*>(block)) {
            prev->size += block->size;
            prev->next = block->next;
        }
    }
}

} // namespace mango_pool