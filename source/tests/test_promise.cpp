
#include <iostream>

#include "core/cpp/promise.hpp"

using namespace MetaEngine::Promise;

#define output_func_name()                                       \
    do {                                                         \
        printf("in function %s, line %d\n", __func__, __LINE__); \
    } while (0)

void test_promise_all() {
    std::vector<Promise> ds = {newPromise([](Defer d) {
                                   output_func_name();
                                   d.resolve(1);
                                   // d.reject(1);
                               }),
                               newPromise([](Defer d) {
                                   output_func_name();
                                   d.resolve(2);
                               })};

    all(ds).then(
            []() {
                output_func_name();
                printf("=====2333");
            },
            []() { output_func_name(); });
}

int main() {
    handleUncaughtException([](Promise &d) { d.fail([](long n, int m) { printf("UncaughtException parameters = %d %d\n", (int)n, m); }).fail([]() { printf("UncaughtException\n"); }); });

    test_promise_all();

    return 0;
}