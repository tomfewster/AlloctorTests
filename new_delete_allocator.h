//
// Created by Tom Fewster on 21/04/2016.
//

#ifndef ALLOCTORTESTS_NEW_DELETE_ALLOCATOR_H
#define ALLOCTORTESTS_NEW_DELETE_ALLOCATOR_H

#include <cstddef>

template <typename T> class new_delete_allocator {
public:
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

private:

    typedef char* storage_type;

public:
    template<typename U> struct rebind {
        typedef new_delete_allocator<U> other;
    };

    inline pointer allocate(const std::size_t size) noexcept {
        return new T[size];
    }

    inline void deallocate(T* p, std::size_t size) noexcept {
        delete [] p;
    }
};

template <class T, class U> bool operator==(const new_delete_allocator<T>&, const new_delete_allocator<U>&);
template <class T, class U> bool operator!=(const new_delete_allocator<T>&, const new_delete_allocator<U>&);


#endif //ALLOCTORTESTS_NEW_DELETE_ALLOCATOR_H
