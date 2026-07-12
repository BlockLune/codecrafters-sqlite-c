#include "cursor.h"
#include <stdint.h>
#include <string.h>

#define SQLITE_VARINT_MAX_LEN 9

void cursor_init(Cursor *const cursor, const uint8_t *const data,
                 const size_t size, const size_t offset) {
  cursor->data = data;
  cursor->size = size;
  cursor->offset = offset;
}

int cursor_seek(Cursor *const cursor, const size_t offset) {
  if (offset > cursor->size) {
    return -1;
  }
  cursor->offset = offset;
  return 0;
}

int cursor_read(Cursor *const cursor, void *const dest, const size_t size) {
  if (cursor->offset > cursor->size || size > cursor->size - cursor->offset) {
    return -1;
  }
  void *src = (void *)cursor->data + cursor->offset;
  memcpy(dest, src, size);
  cursor->offset += size;
  return 0;
}

int cursor_read_u8(Cursor *const cursor, uint8_t *const value) {
  return cursor_read(cursor, value, sizeof(uint8_t));
}

int cursor_read_u16_be(Cursor *const cursor, uint16_t *const value) {
  uint8_t buf[2];
  if (cursor_read(cursor, buf, sizeof(buf)) != 0) {
    return -1;
  }
  *value = ((uint16_t)buf[0] << 8) | buf[1];
  return 0;
}

int cursor_read_sqlite_varint(Cursor *const cursor, uint64_t *const value) {
  *value = 0;

  size_t consumed = 0;
  while (consumed < SQLITE_VARINT_MAX_LEN) {
    uint8_t buf;
    if (cursor_read_u8(cursor, &buf) != 0) {
      return -1;
    }
    ++consumed;

    if (consumed == SQLITE_VARINT_MAX_LEN) {
      // The ninth byte of a SQLite varint contributes all eight bits.
      *value = (*value << 8) | buf;
      return 0;
    }
    *value = (*value << 7) | (buf & 0x7f);
    if ((buf & 0x80) == 0) {
      return 0;
    }
  }
  return -1;
}
