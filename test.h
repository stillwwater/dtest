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

#ifndef TEST_H_
#define TEST_H_

#include <stdio.h>
#include <string.h>

struct test_t {
    const char *name;
    void (*run)(test_t *);
    test_t *next;
    bool fail;
};

extern test_t *test_list;

#define TEST_NAME_(A, B) A ## _ ## B ## _
#define TEST_NAME(A, B) TEST_NAME_(A, B)

#define TEST_DEF(name, id)                                                   \
    static void TEST_NAME(test, id)(test_t *);                               \
    static test_t TEST_NAME(testd, id)                                       \
        = {name, TEST_NAME(test, id), test_list, false};                     \
    static test_t *TEST_NAME(testp, id) = test_list = &TEST_NAME(testd, id); \
    static void TEST_NAME(test, id)([[maybe_unused]] test_t *t_)

#ifdef TEST
#define test(name) TEST_DEF(name, __COUNTER__)
#else
#define test(name) [[maybe_unused]]                                          \
    static void TEST_NAME(test, __COUNTER__)([[maybe_unused]] test_t *t_)
#endif // TEST

#define expect(expr)                                                         \
    do {                                                                     \
        if (!(expr)) {                                                       \
            printf("FAIL \"%s\"\n    %s:%d: %s\n",                           \
                    t_->name, __FILE__, __LINE__, #expr);                    \
            t_->fail = true;                                                 \
            return;                                                          \
        }                                                                    \
    } while (0)

#define test_main                                                            \
    test_t *test_list;                                                       \
    int main(int argc, char *argv[])                                         \
    {                                                                        \
        int pass = 0, skip = 0, count = 0;                                   \
        char *run_test = argc > 1 ? argv[1] : NULL;                          \
        for (test_t *t = test_list; t; t = t->next) {                        \
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
        if (skip) {                                                          \
            printf("%d/%d tests passed; %d skipped.\n", pass, count, skip);  \
            return pass != count;                                            \
        }                                                                    \
        printf("%d/%d tests passed.\n", pass, count);                        \
        return pass != count;                                                \
    }

#if defined(TEST) && !defined(TEST_HAVE_MAIN)
test_main
#endif

#endif // TEST_H_
