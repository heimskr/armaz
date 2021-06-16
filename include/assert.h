#pragma once

void assertion_failed(const char *expr, const char *file, unsigned line);

#define assert(e) (__builtin_expect(!!(e), 1)? (void) 0 : assertion_failed(#e, __FILE__, __LINE__))