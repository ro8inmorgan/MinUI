// core/str.h — type-safe bounded string helpers (C++, header-only).
//
// The point of these over raw snprintf/strcpy: the destination is taken by
// reference to a fixed-size char array, so its size N is deduced at compile
// time. You physically cannot call these on a decayed char* pointer — that's a
// compile error, not a silent 8-byte "size" like snprintf(ptr, sizeof(ptr), …)
// would give you. So they can't overflow and can't be mis-sized.
//
// For the cases where the destination genuinely is a pointer with a runtime
// length (a function param, or a cursor into a larger buffer), keep using
// snprintf directly with the real remaining size — these helpers are for the
// common "write into a local char buf[N]" case.
//
// Usable from any C++ translation unit (nextui, settings, and minarch once its
// modules are C++). Include as: #include "core/str.h"

#ifndef NEXTUI_CORE_STR_H
#define NEXTUI_CORE_STR_H

#include <cstddef>
#include <cstdio>
#include <cstdarg>

namespace core {

// Bounded copy of src into dst[N]. Truncates rather than overflowing. A null
// src yields an empty string. Returns true if the whole source fit.
template <size_t N>
inline bool copy(char (&dst)[N], const char* src) {
	int n = snprintf(dst, N, "%s", src ? src : "");
	return n >= 0 && static_cast<size_t>(n) < N;
}

// Bounded printf-style format into dst[N]. Truncates rather than overflowing.
// Returns true if the whole formatted string fit.
template <size_t N>
inline bool format(char (&dst)[N], const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(dst, N, fmt, ap);
	va_end(ap);
	return n >= 0 && static_cast<size_t>(n) < N;
}

// Bounded append of src onto the string already in dst[N] (like strlcat).
// Truncates rather than overflowing. Returns true if the whole source fit.
template <size_t N>
inline bool append(char (&dst)[N], const char* src) {
	size_t len = 0;
	while (len < N && dst[len] != '\0') len++;
	if (len >= N) return false; // not null-terminated within the buffer
	int n = snprintf(dst + len, N - len, "%s", src ? src : "");
	return n >= 0 && static_cast<size_t>(len) + static_cast<size_t>(n) < N;
}

} // namespace core

#endif // NEXTUI_CORE_STR_H
