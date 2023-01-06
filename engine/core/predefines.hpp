#pragma once

//C++14
#if __cplusplus >= 201402L
#define HAS_CXX14 1
//C++17
#if __cplusplus >= 201703L
#define HAS_CXX17 1
//C++20
#if __cplusplus >= 202002L
#define HAS_CXX20 1
#endif
#endif
#endif

#ifndef HAS_CXX14
#define HAS_CXX14 0
#endif
#ifndef HAS_CXX17
#define HAS_CXX17 0
#endif
#ifndef HAS_CXX20
#define HAS_CXX20 0
#endif

#ifdef __has_cpp_attribute

#if __has_cpp_attribute(carries_dependency)
#define CARRIES_DEPENDENCY [[carries_dependency]]
#endif

#if __has_cpp_attribute(deprecated)
#define DEPRECATED [[deprecated]]
#endif

#if __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#endif

#if __has_cpp_attribute(likely)
#define LIKELY [[likely]]
#endif

#if __has_cpp_attribute(maybe_unused)
#define MAYBE_UNUSED [[maybe_unused]]
#endif

#if __has_cpp_attribute(no_unique_address)
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#if __has_cpp_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#endif

#if __has_cpp_attribute(noreturn)
#define NORETURN [[noreturn]]
#endif

#if __has_cpp_attribute(unlikely)
#define UNLIKELY [[unlikely]]
#endif

#else

#define CARRIES_DEPENDENCY
#define DEPRECATED
#define FALLTHROUGH
#define LIKELY
#define MAYBE_UNUSED
#define NO_UNIQUE_ADDRESS
#define NODISCARD
#define NORETURN
#define UNLIKELY

#endif

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)

#ifdef _WIN32
#define LOG_DEST std::cout
//#define LOG_INIT std::fstream LOG_DEST("log.txt");
#else
#define LOG_DEST std::cout
#define LOG_INIT
#endif

#define PI 3.141592653
#define RAD2DEG(x) x * 57.29578
#define DEG2RAD(x) x / 57.29578