#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#define LOG(fmt, ...) fprintf(stderr, "[LOG] " fmt "\n", ##__VA_ARGS__)

int calc_serial_type_size(const size_t serial_type, size_t *const content_size);

#endif
