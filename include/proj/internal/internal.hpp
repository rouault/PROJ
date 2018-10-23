/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  ISO19111:2018 implementation
 * Author:   Even Rouault <even dot rouault at spatialys dot com>
 *
 ******************************************************************************
 * Copyright (c) 2018, Even Rouault <even dot rouault at spatialys dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef FROM_PROJ_CPP
#error This file should only be included from a PROJ cpp file
#endif

#ifndef INTERNAL_HH_INCLUDED
#define INTERNAL_HH_INCLUDED

#if !(__cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1900))
#error Must have C++11 or newer.
#endif

#include <cassert>
#include <memory>
#include <string>
#ifndef DOXYGEN_ENABLED
#include <type_traits> // for std::is_base_of
#endif
#include <vector>

#include "proj/util.hpp"

//! @cond Doxygen_Suppress

#if ((defined(__clang__) &&                                                    \
      (__clang_major__ > 3 ||                                                  \
       (__clang_major__ == 3 && __clang_minor__ >= 7))) ||                     \
     __GNUC__ >= 7)
/** Macro for fallthrough in a switch case construct */
#define PROJ_FALLTHROUGH [[clang::fallthrough]];
#else
/** Macro for fallthrough in a switch case construct */
#define PROJ_FALLTHROUGH
#endif

#if defined(__clang__) || defined(_MSC_VER)
#define COMPILER_WARNS_ABOUT_ABSTRACT_VBASE_INIT
#endif

NS_PROJ_START

namespace operation {
class OperationParameterValue;
} // namespace operation

namespace internal {

/** Use cpl::down_cast<Derived*>(pointer_to_base) as equivalent of
 * static_cast<Derived*>(pointer_to_base) with safe checking in debug
 * mode.
 *
 * Only works if no virtual inheritance is involved.
 *
 * @param f pointer to a base class
 * @return pointer to a derived class
 */
template <typename To, typename From> inline To down_cast(From *f) {
    static_assert(
        (std::is_base_of<From, typename std::remove_pointer<To>::type>::value),
        "target type not derived from source type");
    assert(f == nullptr || dynamic_cast<To>(f) != nullptr);
    return static_cast<To>(f);
}

/* Borrowed from C++14 */
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

PROJ_DLL std::string replaceAll(const std::string &str,
                                const std::string &before,
                                const std::string &after);

size_t ci_find(const std::string &osStr, const std::string &needle,
               size_t startPos = 0);

bool starts_with(const std::string &str, const std::string &prefix);

bool starts_with(const std::string &str, const char *prefix);

bool ci_starts_with(const std::string &str, const std::string &prefix);

bool ends_with(const std::string &str, const std::string &suffix);

PROJ_DLL std::string tolower(const std::string &osStr);

std::string toupper(const std::string &osStr);

PROJ_DLL std::vector<std::string> split(const std::string &osStr,
                                        char separator);

PROJ_DLL bool ci_equal(const std::string &a, const char *b);

PROJ_DLL bool ci_equal(const std::string &a, const std::string &b);

std::string stripQuotes(const std::string &osStr);

PROJ_DLL double
c_locale_stod(const std::string &s); // throw(std::invalid_argument)

} // namespace internal

NS_PROJ_END

//! @endcond

#endif // INTERNAL_HH_INCLUDED
