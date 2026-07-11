#include "utils.h"

int read_u16_be(const uint8_t *const data, const size_t size,
                const size_t offset, uint16_t *const value) {
  if (offset >= size || offset + 1 >= size) {
    return -1;
  }
  *value = ((uint16_t)data[offset] << 8) | data[offset + 1];
  return 0;
}

int read_varint(const uint8_t *const data, const size_t size,
                const size_t offset, size_t *const value,
                size_t *const consumed) {
  uint8_t buf;
  _Bool more_flag = 1;
  *value = 0;
  *consumed = 0;
  while (more_flag) {
    size_t idx = offset + (*consumed);
    if (idx >= size) {
      return -1;
    }
    buf = data[idx];
    more_flag = (buf & 0x80) == 0x80;
    uint8_t number_part = buf & 0x7f;
    *value = ((*value) << 7) | number_part;
    ++(*consumed);
  }

  return 0;
}

int calc_serial_type_size(const size_t serial_type,
                          size_t *const content_size) {
  if (serial_type == 0) {
    *content_size = 0;
  }
  if (serial_type == 1) {
    *content_size = 1;
  }
  if (serial_type == 2) {
    *content_size = 2;
  }
  if (serial_type == 3) {
    *content_size = 3;
  }
  if (serial_type == 4) {
    *content_size = 4;
  }
  if (serial_type == 5) {
    *content_size = 6;
  }
  if (serial_type == 6) {
    *content_size = 8;
  }
  if (serial_type == 7) {
    *content_size = 8;
  }
  if (serial_type == 8) {
    *content_size = 0;
  }
  if (serial_type == 9) {
    *content_size = 0;
  }
  if (serial_type == 10 || serial_type == 11) {
    return -1;
  }
  if (serial_type >= 12 && serial_type % 2 == 0) {
    *content_size = (serial_type - 12) / 2;
  }
  if (serial_type >= 13 && serial_type % 2 != 0) {
    *content_size = (serial_type - 13) / 2;
  }
  return 0;
}
