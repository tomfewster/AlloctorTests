/***************************************************************************
                          __FILE__
                          -------------------
    copyright            : Copyright (c) 2004-2016 Tom Fewster
    email                : tom@wannabegeek.com
    date                 : 04/03/2016

 ***************************************************************************/

/***************************************************************************
 * This library is free software; you can redistribute it and/or           *
 * modify it under the terms of the GNU Lesser General Public              *
 * License as published by the Free Software Foundation; either            *
 * version 2.1 of the License, or (at your option) any later version.      *
 *                                                                         *
 * This library is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * Lesser General Public License for more details.                         *
 *                                                                         *
 * You should have received a copy of the GNU Lesser General Public        *
 * License along with this library; if not, write to the Free Software     *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA *
 ***************************************************************************/

#ifndef FASTPATH_FAST_NEW_ARENA_H
#define FASTPATH_FAST_NEW_ARENA_H

#include <cstddef>
#include <cassert>
#include <atomic>
#include "optimize.h"

namespace tf {

    template<std::size_t S = 1024>
    class new_arena {
    public:
        using value_type = unsigned char;
        using pointer = value_type *;
        static constexpr std::size_t initial_size = S;

    private:
        struct alignas(16) slab {
            pointer m_content;
            std::atomic<pointer> m_head;
            std::atomic<slab *> m_next;
            std::size_t m_size;
            std::atomic<std::size_t> m_allocated;

            static inline std::size_t align_up(std::size_t n) noexcept {
                static const size_t alignment = 16 - 1;
                return (n + (alignment)) & ~(alignment);
            }

            inline bool pointer_in_buffer(pointer p) const noexcept {
                return m_content <= p && p <= m_head;
            }

            slab(std::size_t size) noexcept : m_next(nullptr), m_allocated(0) {
                // this will find the next x^2 number larger than the one provided
                m_size = size;
                m_size--;
                m_size |= m_size >> 1;
                m_size |= m_size >> 2;
                m_size |= m_size >> 4;
                m_size |= m_size >> 8;
                m_size |= m_size >> 16;
                m_size++;

//                m_content = std::allocator_traits<allocator_type>::allocate(m_allocator, m_size);
//                m_content = new value_type[size];
                m_content = static_cast<pointer>(::malloc(m_size));
                m_head = m_content;
            }

            ~slab() noexcept {
//                std::allocator_traits<allocator_type>::deallocate(m_allocator, m_content, m_size);
//                delete [] m_content;
                ::free(m_content);
            }

            inline std::size_t free() const noexcept {
                return m_size - std::distance(m_content, m_head.load(std::memory_order_relaxed));
            }

            inline pointer allocate(std::size_t size) noexcept {
                size = align_up(size);
                assert(this->free() >= size);
                pointer p = m_head;
                m_head.fetch_add(size);
                m_allocated.fetch_add(size);
                return p;
            }

            inline void deallocate(pointer ptr, std::size_t size) noexcept {
                assert(pointer_in_buffer(ptr));
                size = align_up(size);
                m_allocated.fetch_sub(size, std::memory_order_acq_rel);
                if ((m_allocated -= size) == 0) {
                    m_head = m_content;
                } else if (ptr + size == m_head) {
                    m_head = ptr;
                }
            }
        };

        static __thread slab *s_root_slab;
        static __thread slab *s_current_slab;

        inline slab *find_slab_with_space(slab *start, std::size_t size) const noexcept {
            if (likely(start->free() >= size)) {
                return start;
            } else if (start->m_next != nullptr) {
                return find_slab_with_space(start->m_next, size);
            }

            return nullptr;
        }

        inline slab *find_slab_containing(slab *start, pointer ptr) const noexcept {
            if (likely(start->pointer_in_buffer(ptr))) {
                return start;
            } else if (start->m_next != nullptr) {
                return find_slab_containing(start->m_next, ptr);
            }

            return nullptr;
        }

    public:
        ~new_arena() {
            slab *s = s_root_slab;
            while (s != nullptr) {
                s = s->m_next;
                delete s;
            }
            s_root_slab = nullptr;
        }

        new_arena() noexcept {
            if (s_root_slab == nullptr) {
                s_root_slab = new slab(initial_size);
                s_current_slab = s_root_slab;
            }
        }

        new_arena(const new_arena &) = delete;
        new_arena &operator=(const new_arena &) = delete;

        new_arena::pointer allocate(std::size_t size) {
            slab *s = nullptr;
            if (likely(s_current_slab->free() >= size)) {
                return s_current_slab->allocate(size);
            } else {
                if ((s = find_slab_with_space(s_root_slab, size)) != nullptr) {
                    return s->allocate(size);
                } else {
                    s_current_slab->m_next = new slab(std::max(size, initial_size));
                    s_current_slab = s_current_slab->m_next;
                    return s_current_slab->allocate(size);
                }
            }
        }

        void deallocate(new_arena::pointer p, std::size_t size) noexcept {
            if (s_current_slab->pointer_in_buffer(p)) {
                s_current_slab->deallocate(p, size);
            } else {
                slab *s = find_slab_containing(s_root_slab, p);
                assert(s != nullptr);
                if (s != nullptr) {
                    s->deallocate(p, size);
                }
            }
        }

        friend std::ostream &operator<<(std::ostream &out, const new_arena &a) {
            std::size_t block_count = 0;
            std::size_t total_free = 0;
            std::size_t total_capacity = 0;
            std::size_t total_allocated = 0;

            std::function<void(const slab *)> totals = [&](const slab *start) {
                block_count++;
                total_free += start->free();
                total_capacity += start->m_size;
                total_allocated += start->m_allocated;
                if (start->m_next != nullptr) {
                    totals(start->m_next);
                }
            };

            totals(a.s_root_slab);

            out << "allocated: " << total_allocated << " capacity: " << total_capacity << " allocatable: " << total_free << " from " << block_count << " blocks";
            return out;
        }
    };

    template<std::size_t S> __thread typename new_arena<S>::slab *new_arena<S>::s_root_slab = nullptr;
    template<std::size_t S> __thread typename new_arena<S>::slab *new_arena<S>::s_current_slab = nullptr;
    template<std::size_t S> constexpr std::size_t new_arena<S>::initial_size;
}
#endif //FASTPATH_FAST_LINEAR_ALLOCATORe_H

