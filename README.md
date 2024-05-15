dtest
=====

/dəˈtest/

A tiny C/C++ test and benchmark framework.

~~~ cpp
// math_test.cpp
#include "test.h"

test("math") {
    expect(sin(M_PI / 2.0) == 1.0);
}

test("dubious math") {
    expect(sin(M_PI_EQUALS_3 / 2.0) == 1.0);
}
~~~

~~~
clang++ -DTEST math_test.cpp -o math_test
./math_test
FAIL "dubious math"
    math_test.cpp:11: sin(M_PI_EQUALS_3 / 2.0) == 1.0
PASS "math"
1/2 tests passed.
~~~

Usage
-----

There are multiple ways to use dtest depending on how your project is structured.

### Tests in separate files

In the simplest case, if you have `foo.cpp` and `foo_test.cpp` then your test
file looks like this:

~~~ cpp
// foo_test.cpp
#include "foo.h"
#include "test.h"

test("foo returns 5") {
    expect(foo() == 5);
}
~~~

And is compiled with:

~~~
clang++ -DTEST foo.cpp foo_test.cpp -o foo_test
~~~

Run a single test using:

~~~
./foo_test "foo returns 5"
~~~

Including `test.h` will also define a main function. If you want to link
multiple tests together, then you will need to define your own main function
using the `test_main` macro:

~~~ cpp
// test.cpp
#include "test.h"

test_main
~~~

And use `-DTEST_HAVE_MAIN` so `test.h` does not define its own main function:

~~~
clang++ -DTEST -DTEST_HAVE_MAIN foo_test.cpp bar_test.cpp test.cpp -o all_test
~~~

### Tests next to implementation

If you prefer, tests can be defined next to the implementation being tested. If
`-DTEST` is omitted, then your tests will be ommited from the final build.

~~~ cpp
// foo.cpp
#include "test.h"

int foo() { return 5; }

test("foo returns 5") {
    expect(foo() == 5);
}
~~~

~~~
g++ -DTEST foo.cpp -o foo_test
g++ foo.cpp -o foo
~~~

Just as before, if you are linking multiple translation units with tests
defined, you will need to define your own main function and pass
`-DTEST_HAVE_MAIN` when compiling.

### Unity test build

Another option is to have `test_unity.cpp` include all test files. This way all
tests are part of the same translation unit and main will only be defined once.

~~~ cpp
// test_unity.cpp
#include "foo_test.cpp"
#include "bar_test.cpp"
~~~

~~~
cl -DTEST foo.cpp bar.cpp test_unity.cpp
~~~

## Compiler support

Any C99 or C++ 17 compiler that defines `__COUNTER__` will work (msvc, clang, gcc,
icc). dtest makes use of `[[maybe_unused]]` which requires C++17. However,
both clang and gcc are happy to compile this using C++11 (msvc is not). If you
remove `[[maybe_unused]]` dtest will also compile with C++98, but you will get
warnings for empty tests.
