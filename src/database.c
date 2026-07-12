#include "database.h"
#include "cursor.h"
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int database_open(Database *const db, const char *const path) {
  db->fd = -1;
  db->data = NULL;
  db->size = 0;

  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    return -1;
  }

  struct stat file_stat;
  if (fstat(fd, &file_stat) != 0) {
    close(fd);
    return -1;
  }
  size_t size = file_stat.st_size;
  if (size <= 0) {
    close(fd);
    return -1;
  }
  void *addr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (addr == MAP_FAILED) {
    close(fd);
    return -1;
  }

  db->fd = fd;
  db->data = addr;
  db->size = size;
  return 0;
}

void database_close(Database *const db) {
  if (db->data != NULL) {
    munmap((void *)db->data, db->size);
  }
  if (db->fd != -1) {
    close(db->fd);
  }
}

int database_page_size(const Database *const db, size_t *const page_size) {
  Cursor cursor;
  cursor_init(&cursor, db->data, db->size, PAGE_SIZE_OFFSET);
  uint16_t raw_page_size;
  if (cursor_read_u16_be(&cursor, &raw_page_size) != 0) {
    return -1;
  }
  *page_size = raw_page_size == 1 ? 65536 : raw_page_size;
  return 0;
}

int database_cells_count(const Database *const db,
                         uint16_t *const cells_count) {
  Cursor cursor;
  cursor_init(&cursor, db->data, db->size, CELLS_COUNT_OFFSET);
  if (cursor_read_u16_be(&cursor, cells_count) != 0) {
    return -1;
  }
  return 0;
}

int database_tbl_name(const Database *const db, const size_t offset,
                      const size_t size, char *const dest) {
  if (offset > db->size || size > db->size - offset) {
    return -1;
  }
  memcpy(dest, db->data + offset, size);
  dest[size] = '\0';
  return 0;
}
