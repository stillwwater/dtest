// Copyright (c) 2023 stillwwater@gmail.com
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#ifndef TEST_H
#define TEST_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(BENCHMARK) && !defined(TEST)
#define TEST
#endif

#ifndef TEST_MIN_CYCLES
#define TEST_MIN_CYCLES 3000000000ULL // 1 second on a 3GHz cpu
#endif

#ifndef TEST_MAX_ITER
#define TEST_MAX_ITER 1000000000ULL
#endif

#ifndef TEST_ESCAPE_FALLBACK
#define TEST_ESCAPE_FALLBACK(ptr) (void)ptr
#endif

struct test_info {
    const char *name;
    void (*run)(struct test_info *);
    struct test_info *next;
    uint64_t elapsed;
    uint64_t iter;
    int fail, measure;
};

extern struct test_info *test_list;

#define TEST_ID_(A, B) A ## _ ## B ## _
#define TEST_ID(A, B) TEST_ID_(A, B)

#if defined(__GNUC__) || defined(__clang__)
#define TEST_UNUSED __attribute__((unused))
#elif __cplusplus >= 201703L
#define TEST_UNUSED [[maybe_unused]]
#else
#define TEST_UNUSED
#endif

#if defined(__GNUC__) || defined(__clang__)
#define test_escape(ptr) __asm__ volatile("" : : "g"(ptr) : "memory")
#define test_barrier() __asm__ volatile("" : : : "memory")
#define TEST_CTOR(id) __attribute__((constructor))
#elif defined(_MSC_VER)
#include <intrin.h>
#define test_escape(ptr)                                                     \
    do { TEST_ESCAPE_FALLBACK(ptr); _ReadWriteBarrier(); } while (0)
#define test_barrier() _ReadWriteBarrier()
#pragma section(".CRT$XCU", read)
#define TEST_CTOR(id)                                                        \
    static void TEST_ID(test_ctor, id)(void);                                \
    __declspec(allocate(".CRT$XCU")) static                                  \
    void (*TEST_ID(test_pctor, id))(void) = TEST_ID(test_ctor, id);
#else
#error dtest requires a compatible C/C++ compiler.
#endif

#define TEST_DEF(name, id)                                                   \
    static void TEST_ID(test, id)(struct test_info *_t);                     \
    TEST_CTOR(id) static void TEST_ID(test_ctor, id)(void)                   \
    {                                                                        \
        static struct test_info _t                                           \
            = {name, TEST_ID(test, id), 0, 0, 0, 0, 0};                      \
        _t.next = test_list;                                                 \
        test_list = &_t;                                                     \
    }                                                                        \
    static void TEST_ID(test, id)(TEST_UNUSED struct test_info *_t)

#ifdef TEST
#define test(name) TEST_DEF(name, __COUNTER__)
#else
#define test(name) TEST_UNUSED static void                                   \
    TEST_ID(test, __COUNTER__)(TEST_UNUSED struct test_info *_t)
#endif // TEST

#define expect(expr)                                                         \
    do {                                                                     \
        if (!(expr)) {                                                       \
            printf("FAIL \"%s\"\n    %s:%d: %s\n",                           \
                    _t->name, __FILE__, __LINE__, #expr);                    \
            _t->fail = 1;                                                    \
            return;                                                          \
        }                                                                    \
    } while (0)

#define benchmark if ((_t->measure = 1) && _t->iter)                         \
    for (uint64_t _start = test_rdtsc(), _it = _t->iter;                     \
    _it || ((_t->elapsed = (test_rdtsc() - _start)) && 0);                   \
    --_it)

#ifdef BENCHMARK
#define test_benchmark_section                                               \
    printf("\n%-40s| %-20s| it\n", "benchmark", "cy/it");                    \
    puts("----------------------------------------"                          \
        "|---------------------|------------");                              \
    for (struct test_info *t = test_list; t; t = t->next) {                  \
        if (t->measure && (!run_test || !strcmp(run_test, t->name))) {       \
            for (t->iter = 1; t->iter < TEST_MAX_ITER; t->iter *= 10) {      \
                t->run(t);                                                   \
                if (t->elapsed >= TEST_MIN_CYCLES) break;                    \
            }                                                                \
            printf("%-40s| %-20.0f| %lld\n", t->name,                        \
                    t->elapsed / (double)t->iter, (long long)t->iter);       \
        }                                                                    \
    }
#else
#define test_benchmark_section
#endif

#define test_main                                                            \
    struct test_info *test_list;                                             \
    int                                                                      \
    main(int argc, char *argv[])                                             \
    {                                                                        \
        int pass = 0, skip = 0, count = 0;                                   \
        char *run_test = argc > 1 ? argv[1] : NULL;                          \
        for (struct test_info *t = test_list; t; t = t->next) {              \
            if ((!run_test || !strcmp(run_test, t->name)) && ++count) {      \
                t->run(t);                                                   \
                if (!t->fail && ++pass) printf("PASS \"%s\"\n", t->name);    \
                continue;                                                    \
            }                                                                \
            ++skip;                                                          \
        }                                                                    \
        if (!count) {                                                        \
            if (run_test) {                                                  \
                printf("test \"%s\" not found.\n", run_test);                \
                return 2;                                                    \
            }                                                                \
            puts("no tests found.");                                         \
            return 2;                                                        \
        }                                                                    \
        test_benchmark_section                                               \
        if (skip) {                                                          \
            printf("%d/%d tests passed; %d skipped.\n", pass, count, skip);  \
            return pass != count;                                            \
        }                                                                    \
        printf("%d/%d tests passed.\n", pass, count);                        \
        return pass != count;                                                \
    }

inline uint64_t
test_rdtsc(void)
{
#if defined(__amd64__) || defined(__x86_64__)
    uint64_t l, h;
    __asm__ volatile("rdtsc" : "=a"(l), "=d"(h));
    return (h << 32) | l;
#elif defined(__aarch64__)
    int64_t t;
    __asm__ volatile("mrs %0, CNTVCT_EL0" : "=r"(t));
    return t;
#elif defined(_M_AMD64)
    return __rdtsc();
#elif defined(BENCHMARK)
#error dtest does not support benchmarking on this cpu architecture.
#else
    return 0;
#endif
}

#if defined(TEST) && !defined(TEST_HAVE_MAIN)
test_main
#endif

#endif // TEST_H
