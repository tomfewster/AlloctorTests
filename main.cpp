#include <iostream>

#include <chrono>
#include <memory>
#include <cstdlib>
#include <vector>
#include <ctime>
#include <iomanip>

#include "fast_linear_allocator.h"
#include "fast_linear_allocator2.h"
#include "performance.h"
#include "short_alloc.h"
#include <boost/pool/pool_alloc.hpp>

template <typename A> void testSimpleAllocateDeallocate(A &allocator) {
    for (int i = 0; i < 1000000; ++i) {
        auto ptr = std::allocator_traits<A>::allocate(allocator, 100);
        std::allocator_traits<A>::deallocate(allocator, ptr, 100);
    }
}

template <typename A> void testSimpleRandomAllocateDeallocate(A &allocator) {
    std::srand(std::time(0));
    std::vector<typename std::allocator_traits<A>::pointer> m_allocations;

    for (int i = 0; i < 1000000; ++i) {
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

template <typename A> void runTests(A &allocator) {

    std::cout << std::left << std::setw(80) << std::string(typeid(A).name()).substr(0, 80);

    auto logger = [&](const std::chrono::microseconds &time) {
        std::cout << std::setw(20) << std::setprecision(3) << std::right << time.count() << " us";
    };

    logger(tf::measure<std::chrono::microseconds>::execution([&]() { testSimpleAllocateDeallocate(allocator); }));
    logger(tf::measure<std::chrono::microseconds>::execution([&]() { testSimpleRandomAllocateDeallocate(allocator); }));

    std::cout << std::endl;
}

template <typename T> void testForType(const char *type) {

    const std::size_t pre_alloc_size = 1024 * 1024;

    std::cout << " Testing " << type << std::endl;
    std::cout << "=====================" << std::endl;

    {
        std::allocator<T> allocator;
        runTests(allocator);
    }

    {
        typename tf::linear_allocator<T>::arena_type arena(pre_alloc_size);
        typename tf::linear_allocator<T> allocator(arena);
        runTests(allocator);
    }

    {
        typename tf::linear_allocator2<T>::arena_type arena(pre_alloc_size);
        typename tf::linear_allocator2<T> allocator(arena);
        runTests(allocator);
    }

    {
        typename short_alloc<T, 4096>::arena_type  arena;
        short_alloc<T, 4096> allocator(arena);
        runTests(allocator);
    }

    {
        boost::fast_pool_allocator<T, boost::default_user_allocator_new_delete, boost::details::pool::null_mutex> allocator;
        runTests(allocator);
    }

    {
        boost::fast_pool_allocator<T> allocator;
        runTests(allocator);
    }
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