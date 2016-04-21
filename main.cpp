#include <iostream>

#include <chrono>
#include <memory>
#include <cstdlib>
#include <vector>
#include <ctime>
#include <iomanip>

#include "fast_linear_allocator.h"
#include "arena_unoptimised.h"
#include "new_arena.h"
#include "performance.h"
#include "short_alloc.h"
#include "new_delete_allocator.h"
//#include <boost/pool/pool_alloc.hpp>

static const std::size_t iterations = 1000000;

template <typename A> void testSimpleAllocateDeallocate(A &allocator) {
    for (std::size_t i = 0; i < iterations; ++i) {
        auto ptr = std::allocator_traits<A>::allocate(allocator, 100);
        std::allocator_traits<A>::deallocate(allocator, ptr, 100);
    }
}

template <typename A> void testSimpleRandomAllocateDeallocate(A &allocator) {
    std::srand(std::time(0));
    std::vector<typename std::allocator_traits<A>::pointer> m_allocations;

    for (std::size_t i = 0; i < iterations; ++i) {
        int add_remove = std::rand();
        if (add_remove % 2) {
            m_allocations.push_back(std::allocator_traits<A>::allocate(allocator, 100));
        } else if (m_allocations.size() != 0) {
            size_t index = static_cast<size_t>((static_cast<double>(std::rand()) / RAND_MAX) * m_allocations.size());
            std::allocator_traits<A>::deallocate(allocator, m_allocations[index], 100);
            m_allocations.erase(m_allocations.begin() + index);
        }
    }
}

template <typename A> void testAllocateDeallocateRandomSize(A &allocator) {
    std::srand(std::time(0));
    std::vector<std::pair<std::size_t, typename std::allocator_traits<A>::pointer>> m_allocations;

    for (std::size_t i = 0; i < iterations; ++i) {
        int add_remove = std::rand();
        // create anything between 0 and 1k
        std::size_t size = static_cast<size_t>((static_cast<double>(std::rand()) / RAND_MAX) * 1024);
        if (add_remove % 2) {
            m_allocations.emplace_back(size, std::allocator_traits<A>::allocate(allocator, size));
        } else if (m_allocations.size() != 0) {
            size_t index = static_cast<size_t>((static_cast<double>(std::rand()) / RAND_MAX) * m_allocations.size());
            auto m = m_allocations[index];
            std::allocator_traits<A>::deallocate(allocator, m.second, m.first);
            m_allocations.erase(m_allocations.begin() + index);
        }
    }
}

template <typename A> void runTests(A &allocator) {

    std::cout << std::left << std::setw(80) << std::string(typeid(A).name()).substr(0, 80);

    auto logger = [&](const std::chrono::microseconds &time) {
        std::cout << std::setw(20) << std::setprecision(3) << std::right << time.count() << " us";
    };

    logger(tf::measure<std::chrono::microseconds>::execution([&]() { testSimpleAllocateDeallocate(allocator); }));
    logger(tf::measure<std::chrono::microseconds>::execution([&]() { testSimpleRandomAllocateDeallocate(allocator); }));
    logger(tf::measure<std::chrono::microseconds>::execution([&]() { testAllocateDeallocateRandomSize(allocator); }));

    std::cout << std::endl;
}

template <typename T> void testForType(const char *type) {

    const std::size_t pre_alloc_size = 1024 * 1024;

    std::cout << std::endl << "=====================" << std::endl;
    std::cout << " Testing " << type << " (" << sizeof(T) << ")"<< std::endl;
    std::cout << "=====================" << std::endl;

    {
        std::allocator<T> allocator;
        runTests(allocator);
    }

    {
        new_delete_allocator<T> allocator;
        runTests(allocator);
    }

    {
        typename tf::linear_allocator<T>::arena_type arena(pre_alloc_size);
        typename tf::linear_allocator<T> allocator(arena);
        runTests(allocator);
    }

    {
        typename tf::linear_allocator<T, tf::arena_unoptimised>::arena_type arena(pre_alloc_size);
        typename tf::linear_allocator<T, tf::arena_unoptimised> allocator(arena);
        runTests(allocator);
    }

    {
        typename tf::linear_allocator<T, tf::new_arena>::arena_type arena(pre_alloc_size);
        typename tf::linear_allocator<T, tf::new_arena> allocator(arena);
        runTests(allocator);
    }

    {
        typename short_alloc<T, 4096>::arena_type  arena;
        short_alloc<T, 4096> allocator(arena);
        runTests(allocator);
    }

//    {
//        boost::fast_pool_allocator<T, boost::default_user_allocator_new_delete, boost::details::pool::null_mutex> allocator;
//        runTests(allocator);
//    }
//
//    {
//        boost::fast_pool_allocator<T> allocator;
//        runTests(allocator);
////    }
}

#define TEST(x) testForType<x>(#x)

int main() {

    struct small_obj {
        char data[200];
        int a;
        bool b;
        float c;
    };

    struct large_obj {
        char data[3456];
        int data2[1234];
    };

    TEST(char);
    TEST(uint32_t);
    TEST(uint64_t);
    TEST(double);
    TEST(small_obj);
    TEST(large_obj);

    return 0;
}
