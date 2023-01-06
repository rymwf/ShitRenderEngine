#pragma once
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <functional>
#include <variant>
#include <string_view>
#include <span>
#include <ranges>
#include <unordered_map>
#include "predefines.hpp"

namespace ranges = std::ranges;
namespace views = std::views;

template <class... Args>
constexpr void myprint(std::ostream &os, Args &&...args) noexcept
{
	((os << std::forward<Args>(args) << " "), ...);
}

#ifdef NDEBUG
#define LOG(...)
#define LOG_VAR(str)
#else
#define LOG(...)                                         \
	{                                                    \
		LOG_DEST << __FILE__ << " " << __LINE__ << ": "; \
		myprint(LOG_DEST, __VA_ARGS__);                  \
		LOG_DEST << std::endl;                           \
	}
#define LOG_VAR(str) \
	LOG_DEST << __FILE__ << " " << __LINE__ << ":  " << #str << ": " << str << std::endl;
#endif

#define THROW(...)                                 \
	{                                              \
		std::stringstream ss;                      \
		ss << __FILE__ << " " << __LINE__ << ": "; \
		myprint(ss, __VA_ARGS__);                  \
		ss << std::endl;                           \
		throw std::runtime_error(ss.str());        \
	}

#ifdef __cpp_lib_nonmember_container_access
#define ARR_SIZE(x) std::size(x)
#else
template <class T, std::size_t N>
constexpr std::size_t mysize(const T (&array)[N]) noexcept
{
	return N;
}
template <class C>
constexpr auto mysize(const C &c) -> decltype(c.size())
{
	return c.size();
}
#define ARR_SIZE(x) mysize(x)
#endif

std::string_view trimL(std::string_view str);
std::string_view trimR(std::string_view str);
std::string_view trim(std::string_view str);

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
float intToFloat(T value)
{
	return value;
}
template <typename T, typename U = float, std::enable_if_t<std::is_integral_v<T>, bool> = true>
U intToFloat(T value)
{
	return (std::max)(static_cast<U>(value) / std::numeric_limits<std::decay_t<T>>::max(), static_cast<U>(-1.f));
}
std::vector<std::string_view> split(std::string_view str, std::string_view delim);

template <typename T>
T clamp(const T &v, const T &minVal, const T &maxVal)
{
	auto &&a = v < minVal ? minVal : v;
	return a > maxVal ? maxVal : a;
}
std::string_view getRelativePath(std::string_view rootPath, std::string_view fullpath);

#if defined(_MSC_VER)
#define FUNCTIONAL_HASH_ROTL32(x, r) _rotl(x, r)
#else
#define FUNCTIONAL_HASH_ROTL32(x, r) (x << r) | (x >> (32 - r))
#endif

// from boost
template <typename T>
void hashCombineImpl(T &seed, T value)
{
	seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline void hashCombineImpl(uint32_t &h1, uint32_t k1)
{
	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;

	k1 *= c1;
	k1 = FUNCTIONAL_HASH_ROTL32(k1, 15);
	k1 *= c2;

	h1 ^= k1;
	h1 = FUNCTIONAL_HASH_ROTL32(h1, 13);
	h1 = h1 * 5 + 0xe6546b64;
}

constexpr void hashCombineImpl(uint64_t &h, uint64_t k)
{
	const uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
	const int r = 47;

	k *= m;
	k ^= k >> r;
	k *= m;

	h ^= k;
	h *= m;

	// Completely arbitrary number, to prevent 0's
	// from hashing to 0.
	h += 0xe6546b64;
}

template <typename T>
void hashCombine(std::size_t &seed, const T &v)
{
	hashCombineImpl(seed, std::hash<T>{}(v));
}

void parseResourceName(std::string_view resourceName, std::string_view &archiveName, std::string_view &childItemName);
