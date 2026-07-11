#ifndef _UTILS_H_
#define _UTILS_H_

#include <stddef.h>
#include <stdint.h>

int read_u16_be(const uint8_t *const data, const size_t size,
                const size_t offset, uint16_t *const value);

int read_varint(const uint8_t *const data, const size_t size,
                const size_t offset, size_t *const value,
                size_t *const consumed);

int calc_serial_type_size(const size_t serial_type, size_t *const content_size);

#endif
