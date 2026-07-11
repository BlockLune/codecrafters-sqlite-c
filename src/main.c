#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
The `sqlite_schema` table is always stored on page 1.

The first 100 bytes of page 1 contain the database header, so the actual
B-tree page starts at offset 100.

first page:
offset +0    sqlite header                  100 bytes
offset +100  btree header                   8 bytes
offset +108  cell pointer array             2 * cells_count bytes
---
sqlite header:
offset +0    magic string with terminator   16 bytes
offset +16   page size                      2 bytes
offset +18   ...
---
btree header:
offset +0    flags                          1 byte
offset +1    first freeblock                2 bytes
offset +3    number of cells                2 bytes
offset +5    start of cell content          2 bytes
offset +7    fragmented bytes               1 byte
*/

#define PAGE_SIZE_OFFSET 16
#define CELLS_COUNT_OFFSET 103

static int read_u16_be(FILE *file, long offset, uint16_t *value) {
  uint8_t buf[2];
  if (fseek(file, offset, SEEK_SET) != 0) {
    return -1;
  }
  if (fread(buf, sizeof(buf), 1, file) != 1) {
    return -1;
  }
  *value = ((uint16_t)buf[0] << 8) | buf[1];
  return 0;
}

static int get_page_size(FILE *database_file, size_t *page_size) {
  uint16_t raw_page_size;
  if (read_u16_be(database_file, PAGE_SIZE_OFFSET, &raw_page_size) != 0) {
    return -1;
  }
  *page_size = raw_page_size == 1 ? 65536 : raw_page_size;
  return 0;
}

static int get_cells_count(FILE *database_file, uint16_t *cells_count) {
  if (read_u16_be(database_file, CELLS_COUNT_OFFSET, cells_count) != 0) {
    return -1;
  }
  return 0;
}

static int print_dbinfo(const char *database_file_path) {
  FILE *database_file = fopen(database_file_path, "rb");
  if (!database_file) {
    fprintf(stderr, "Failed to open the database file.\n");
    return -1;
  }

  int result = -1;
  size_t page_size;
  if (get_page_size(database_file, &page_size)) {
    fprintf(stderr, "Failed to read page size.\n");
    goto cleanup;
  }
  printf("database page size: %zu\n", page_size);

  uint16_t cells_count;
  if (get_cells_count(database_file, &cells_count)) {
    fprintf(stderr, "Failed to read cells count.\n");
    goto cleanup;
  }
  // In this challenge, we assume that the database contains only tables - no
  // indexes, views, or other objects.
  printf("number of tables: %u\n", cells_count);
  result = 0;

cleanup:
  fclose(database_file);
  return result;
}

static int print_tables(const char *database_file_path) {
  // TODO: implement print_tables
  return -1;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: ./your_program.sh <database path> <command>\n");
    return EXIT_FAILURE;
  }

  fprintf(stderr, "====== CodeCrafters SQLite C ======\n");

  const char *database_file_path = argv[1];
  const char *command = argv[2];

  if (strcmp(command, ".dbinfo") == 0) {
    return print_dbinfo(database_file_path) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  if (strcmp(command, ".tables") == 0) {
    return print_tables(database_file_path) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  fprintf(stderr, "Unknown command %s\n", command);
  return EXIT_FAILURE;
}
