#pragma once

#include <cmath>
#include <iostream>
#include <string>

inline int gTestsRun = 0;
inline int gTestsFailed = 0;

#define EXPECT_TRUE(expr)                                                          \
    do {                                                                           \
        ++gTestsRun;                                                               \
        if (!(expr)) {                                                             \
            ++gTestsFailed;                                                        \
            std::cerr << "FAIL: " << __FILE__ << ":" << __LINE__ << " " << #expr    \
                      << "\n";                                                     \
        }                                                                          \
    } while (0)

#define EXPECT_NEAR(a, b, tol)                                                     \
    do {                                                                           \
        ++gTestsRun;                                                               \
        const double diff = std::fabs(static_cast<double>(a) - static_cast<double>(b)); \
        if (diff > static_cast<double>(tol)) {                                      \
            ++gTestsFailed;                                                        \
            std::cerr << "FAIL: " << __FILE__ << ":" << __LINE__ << " " << #a      \
                      << " ~ " << #b << " (" << (a) << " vs " << (b) << ")\n";     \
        }                                                                          \
    } while (0)

#define RUN_TEST(fn)                                                               \
    do {                                                                           \
        std::cout << "Running " << #fn << "...\n";                                 \
        fn();                                                                      \
    } while (0)
