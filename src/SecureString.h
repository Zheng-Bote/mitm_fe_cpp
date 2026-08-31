/**
 * SPDX-FileComment: SecureString
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <sodium.h>

namespace mitm::crypto {

template <typename T>
struct SecureAllocator {
    using value_type = T;

    SecureAllocator() = default;

    template <typename U>
    constexpr SecureAllocator(const SecureAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        if (n > std::size_t(-1) / sizeof(T)) throw std::bad_alloc();
        // sodium_malloc guarantees allocation and memory protection
        if (auto p = static_cast<T*>(sodium_malloc(n * sizeof(T)))) return p;
        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t /*n*/) noexcept {
        // sodium_free zeroes out the memory before freeing
        sodium_free(p);
    }

    bool operator==(const SecureAllocator&) const { return true; }
    bool operator!=(const SecureAllocator&) const { return false; }
};

using SecureString = std::basic_string<char, std::char_traits<char>, SecureAllocator<char>>;

} // namespace mitm::crypto
