#pragma once

#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cstdlib>
#include <new>

#ifndef __FUNCTION_NAME__
#   ifdef WIN32   //WINDOWS
#       define __FUNCTION_NAME__   __FUNCTION__  
#   else          //*NIX
#       define __FUNCTION_NAME__   __func__ 
#endif
#endif

#define RMEM_NEW(_alloc, _type) static_cast<_type*>(_alloc.alloc(sizeof(_type)).ptr)
#define RMEM_MAKE(_type, _name, _alloc) _type _name = *(static_cast<_type*>(_alloc.alloc(sizeof(_type)).ptr))
#define RMEM_DEL(_name, _alloc) { reyes::mem::blk _blk(static_cast<void*>(&_name),sizeof(_name)); _alloc.free(_blk); }

namespace reyes
{
    namespace mem
    {
        /* Memory block sturcture. */
        struct blk
        {
            void* ptr;
            size_t size;
            blk() : ptr(nullptr), size(0) {}
            blk(void* p, size_t s = 0) : ptr(p), size(s) {}

            template<class ptr_t>
            ptr_t to() { return static_cast<ptr_t>(ptr); }
        };

        /* Allocator interface. */
        class AllocatorI
        {
        public:
            virtual bool owns(blk&) = 0;
            virtual blk alloc(size_t size) = 0;                                // TODO: add align
            // virtual blk realloc(blk&, size_t);                           // TODO: add realloc
            virtual void free(blk&) = 0;
        };

        /* Standard malloc/free allocator on heap. */
        class CAllocator : public AllocatorI
        {
            size_t total;
        public:
            CAllocator() : total(0) {}
            bool owns(blk&) { return true; }
            blk alloc(size_t size)
            {
                blk blk;
                blk.ptr = ::malloc(size);
                blk.size = size;
                total += size;
                return blk;
            }
            void free(blk& mem)
            {
                total -= mem.size;
                ::free(mem.ptr);
                mem.ptr = 0;
            }
            ~CAllocator()
            {
                assert(total == 0);
            }
        };

        /* Standard malloc/free allocator on heap. */
        typedef CAllocator mAllocator;

        /* Null allocator, everything always fails. */
        class NullAllocator : public AllocatorI
        {
        public:
            bool owns(const blk&) { return false; }
            blk alloc(size_t size) { return{ nullptr, 0 }; }
            void free(blk& mem) { assert(!mem.ptr && !mem.size); }
        };

        template <size_t S>
        /* Stack allocator, compile time known size (in bytes). */
        class StackAllocator : public AllocatorI
        {
            char data[S];
            char* top;
        public:
            StackAllocator() : top(data) {}
            bool owns(blk& mem)
            {
                return (mem.ptr >= data && mem.ptr<data + S);
            }
            blk alloc(size_t size)
            {
                if (size > data + S - top) return{ nullptr, 0 };
                blk mem = { top, size };
                top += size;
                return mem;
            }
            void free(blk& mem)
            {
                if (static_cast<char*>(mem.ptr) + mem.size == top)
                    top = static_cast<char*>(mem.ptr);
            }
            ~StackAllocator()
            {
                assert(data == top);
            }
        };

        template <class P, class F>
        /* Fallback allocator: use Primary first, Fallback otherwise. */
        class FallbackAllocator : private P, private F
        {
        public:
            bool owns(blk& mem)
            {
                return P::owns(mem) || F::owns(mem);
            }
            blk alloc(size_t size)
            {
                blk mem = P::alloc(size);
                if (!mem.ptr) mem = F::alloc(size);
                return mem;
            }
            void free(blk& mem)
            {
                if (P::owns(mem))
                    P::free(mem);
                else
                    F::free(mem);
            }
        };

        template <size_t threshold, class SmallAllocator, class LargeAllocator>
        /* Uses small allocator for allocs below threshold, otherwise it uses large allocator. */
        class SegregatorAllocator : private SmallAllocator, private LargeAllocator, public AllocatorI
        {
        public:
            blk alloc(size_t size)
            {
                return (size <= threshold)
                    ? SmallAllocator::alloc(size)
                    : LargeAllocator::alloc(size);
            }
            void free(blk& mem)
            {
                if (!mem.ptr) return;
                if (mem.size <= threshold) return SmallAllocator::free(mem);
                return LargeAllocator::free(mem);
            }
            bool owns(blk& mem)
            {
                return (mem.size <= threshold)
                    ? SmallAllocator::owns(mem)
                    : LargeAllocator::owns(mem);
            }
        };

        template <class A, class Prefix, size_t prefix_size, class Sufix = void, size_t sufix_size = 0>
        /* Allocator that wraps other allocator and adds prefix (and sufix) information. */
        class AffixAllocator
        {
            A allocator;
        public:
            Prefix* getPrefixPtr(const blk& mem) const
            {
                return static_cast<Prefix*>(mem.ptr);
            }
            Sufix* getSufixPtr(const blk& mem) const
            {
                return static_cast<Sufix*>(static_cast<char*>(mem.ptr) + mem.size - sufix_size);
            }
            void* getDataPtr(const blk& mem) const
            {
                return static_cast<void*>(static_cast<char*>(mem.ptr) + sufix_size);
            }
            blk getInnerblk(const blk& mem) const
            {
                return{ static_cast<char*>(mem.ptr) + prefix_size, mem.size - prefix_size - sufix_size };
            }
            blk getOuterblk(const blk& mem) const
            {
                return{ static_cast<char*>(mem.ptr) - prefix_size, mem.size + prefix_size + sufix_size };
            }
            bool owns(blk& mem)
            {
                return allocator::owns(mem);
            }
            blk alloc(size_t size)
            {
                blk mem = allocator.alloc(prefix_size + size + sufix_size);
                if (mem.ptr) {
                    if (prefix_size > 0) {
                        new (static_cast<char*>(mem.ptr) - prefix_size) Prefix{};
                    }
                    if (sufix_size > 0) {
                        new (static_cast<char*>(mem.ptr) + prefix_size + size) Sufix{};
                    }
                    getInnerblk(mem);
                }
                return mem;
            }
            void free(blk& mem)
            {
                allocator.free(mem);
            }
        };

        // TODO: alignment
        template<class ObjTy>
        struct ObjectStack
        {
            // ....[ Object_n-1 ][ size_n-1 ][ Object_n ][ size_n ]
            // ....................................................^ top

            size_t capacity;
            char* data;
            char* top;

            ObjectStack(size_t cap=1024)
                : capacity(cap)
                , data(new char[cap])
                , top(data)
            {}
            void* alloc(size_t size)
            {
                if (size + sizeof(size_t) > data + capacity - top)
                    return nullptr;

                void* mem = static_cast<void*>(top);
                size_t* sz_ptr = (size_t*)(top += size);
                *sz_ptr = size;
                top += sizeof(size_t);
                return mem;
            }
            ObjTy* pop(void)
            {
                if (!size()) return 0;
                size_t size = *(size_t*)(top -= sizeof(size_t));
                top -= size;
                return (ObjTy*)(top);
            }
            ObjTy* get(char* ptr)
            {
                if(ptr==data) return 0;
                size_t size = *(size_t*)(ptr -= sizeof(size_t));
                ptr -= size;
                return (ObjTy*)(ptr);
            }
            size_t size() const
            {
                return top - data;
            }
            operator bool(void) const
            {
                return size() > 0;
            }
            ~ObjectStack()
            {
                assert(data == top);
                delete[] data;
            }
        };

        struct ObjectArray
        {
            size_t capacity;
            char* data;
            char* writePtr;
            char* readPtr;
            size_t allocCnt;

            ObjectArray(size_t cap)
                : capacity(cap)
                , data(new char[cap])
                , readPtr(data)
                , writePtr(data)
                , allocCnt(0)
            {}
            ~ObjectArray()
            {
                assert(0 == allocCnt);
                delete[] data;
            }

            mem::blk alloc(size_t size)
            {
                assert(size + sizeof(size_t) <= data + capacity - writePtr);
                allocCnt++;

                size_t* sz_ptr = (size_t*)(writePtr);
                *sz_ptr = size;
                writePtr += sizeof(size);
                void* mem = (void*)(writePtr);
                writePtr += size;
                return {mem, size};
            }

            mem::blk getNext(void)
            {
                assert(allocCnt > 0);
                assert(readPtr < writePtr);
                allocCnt--;

                size_t* sz_ptr = (size_t*)(readPtr);
                readPtr += sizeof(size_t);
                void* objptr = (void*)(readPtr);
                readPtr += *sz_ptr;
                return{objptr,*sz_ptr};
            }
        };

        template<class ItemTy, size_t size>
        struct Stack
        {
            ItemTy data[size];
            size_t top;
            Stack() : top(0) {}
            void push(ItemTy item)
            {
                data[top] = item;
                top++;
                assert(top <= size);
            }
            ItemTy pop()
            {
                assert(top > 0);
                top--;
                return data[top];
            }
            ~Stack()
            {
                assert(top == 0);
            }
            operator bool()
            {
                return top > 0;
            }
        };

        template<class SlotTy, int slotCount>
        struct Pool
        {
            SlotTy data[slotCount];
            int freeList[slotCount]; // can be optimized to reuse
            int freeHead;
            int freeCnt;

            Pool()
                : freeHead(0)
                , freeCnt(slotCount)
            {
                for (int i = 0; i < slotCount - 1; i++)
                    freeList[i] = i + 1;
                freeList[slotCount-1] = -1;
            }

            SlotTy* alloc(void)
            {
                assert(freeHead>=0 && freeHead<slotCount);
                freeCnt--;
                int idx = freeHead;
                freeHead = freeList[freeHead];
                return data + idx;
            }

            void free(SlotTy* ptr)
            {
                freeCnt++;
                int idx = (ptr - data)/sizeof(SlotTy);
                freeList[idx] = freeHead;
                freeHead = idx;
            }

            void free(SlotTy& slot)
            {
                this->free(&slot);
            }

            ~Pool()
            {
                assert(slotCount == freeCnt);
            }
        };
    }
}