#pragma once

#include "uxs/config.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

#if !defined(NOEXCEPT)
#    if defined(_MSC_VER) && _MSC_VER <= 1800
#        define NOEXCEPT throw()
#        define NOEXCEPT_IF(x)
#    else  // defined(_MSC_VER) && _MSC_VER <= 1800
#        define NOEXCEPT       noexcept
#        define NOEXCEPT_IF(x) noexcept(x)
#    endif  // defined(_MSC_VER) && _MSC_VER <= 1800
#endif      // !defined(NOEXCEPT)

#if !defined(CONSTEVAL)
#    if __cplusplus >= 202002L && \
        (__GNUC__ >= 10 || __clang_major__ >= 12 || (_MSC_VER >= 1920 && defined(__cpp_consteval)))
#        define CONSTEVAL     consteval
#        define HAS_CONSTEVAL 1
#    else
#        define CONSTEVAL inline
#    endif
#endif  // !defined(CONSTEVAL)

#if !defined(CONSTEXPR)
#    if __cplusplus < 201703L
#        define CONSTEXPR inline
#    else  // __cplusplus < 201703L
#        define CONSTEXPR constexpr
#    endif  // __cplusplus < 201703L
#endif      // !defined(CONSTEXPR)

#if !defined(NODISCARD)
#    if __cplusplus < 201703L
#        define NODISCARD
#    else  // __cplusplus < 201703L
#        define NODISCARD [[nodiscard]]
#    endif  // __cplusplus < 201703L
#endif      // !defined(NODISCARD)

#if !defined(UNREACHABLE_CODE)
#    if defined(_MSC_VER)
#        define UNREACHABLE_CODE __assume(false)
#    elif defined(__GNUC__)
#        define UNREACHABLE_CODE __builtin_unreachable()
#    else
#        define UNREACHABLE_CODE
#    endif
#endif  // !defined(UNREACHABLE_CODE)

#if !defined(NOALIAS)
#    if defined(_MSC_VER)
#        define NOALIAS __declspec(noalias)
#    elif defined(__GNUC__)
#        define NOALIAS __attribute__((pure))
#    else
#        define NOALIAS
#    endif
#endif  // !defined(NOALIAS)
