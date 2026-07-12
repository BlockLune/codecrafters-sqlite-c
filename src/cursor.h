#ifndef CURSOR_H
#define CURSOR_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  const uint8_t *data;
  size_t size;
  size_t offset;
} Cursor;

void cursor_init(Cursor *const cursor, const uint8_t *const data,
                const size_t size, const size_t offset);

int cursor_seek(Cursor *const cursor, const size_t offset);

int cursor_read(Cursor *const cursor, void *const dest, const size_t size);

int cursor_read_u8(Cursor *const cursor, uint8_t *const value);

int cursor_read_u16_be(Cursor *const cursor, uint16_t *const value);

int cursor_read_sqlite_varint(Cursor *const cursor, uint64_t *const value);

#endif
