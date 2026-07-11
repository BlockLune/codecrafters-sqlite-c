#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
The `sqlite_schema` table is always stored on page 1.

The first 100 bytes of page 1 contain the database header, so the actual
B-tree page starts at offset 100.

```
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
```

More resources:
- <https://www.sqlite.org/fileformat.html>
- <https://fly.io/blog/sqlite-internals-btree>
- <https://link.springer.com/content/pdf/10.1007/978-3-030-98467-0_5.pdf>
*/

#define PAGE_SIZE_OFFSET 16
#define CELLS_COUNT_OFFSET 103
#define CELL_POINTER_ARRAY_OFFSET 108

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

static int read_varint(FILE *file, long offset, size_t *value, long *consumed) {
  uint8_t buf;
  if (fseek(file, offset, SEEK_SET) != 0) {
    return -1;
  }

  _Bool more_flag = 1;
  *value = 0;
  *consumed = 0;
  while (more_flag) {
    if (fread(&buf, sizeof(buf), 1, file) != 1) {
      return -1;
    }
    more_flag = (buf & 0x80) == 0x80;
    uint8_t number_part = buf & 0x7f;
    *value = ((*value) << 7) | number_part;
    ++(*consumed);
  }

  return 0;
}

static int calc_serial_type_size(size_t serial_type, size_t *content_size) {
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

static int read_tbl_name(FILE *database_file, long offset, size_t size,
                         char *dest) {
  if (fseek(database_file, offset, SEEK_SET) != 0) {
    return -1;
  }
  if (fread(dest, 1, size, database_file) != size) {
    return -1;
  }
  dest[size] = '\0';
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
  FILE *database_file = fopen(database_file_path, "rb");
  if (!database_file) {
    fprintf(stderr, "Failed to open the database file.\n");
    return -1;
  }

  int result = -1;

  uint16_t cells_count;
  if (get_cells_count(database_file, &cells_count)) {
    fprintf(stderr, "Failed to read cells count.\n");
    goto cleanup;
  }

  uint16_t *cell_offsets = malloc(sizeof(uint16_t) * cells_count);
  for (uint16_t i = 0; i < cells_count; ++i) {
    if (read_u16_be(database_file, CELL_POINTER_ARRAY_OFFSET + i * 2,
                    &cell_offsets[i]) != 0) {
      fprintf(stderr, "Failed to read cell pointer at index %u.\n", i);
      goto free_cell_offsets;
    }
  }

  char **tbl_names = malloc(sizeof(char *) * cells_count);
  for (uint16_t cell_idx = 0; cell_idx < cells_count; ++cell_idx) {
    uint16_t cell_offset = cell_offsets[cell_idx];
    size_t record_size;
    long record_size_consumed;
    read_varint(database_file, cell_offset, &record_size,
                &record_size_consumed); // TODO: if err

    fprintf(stderr, "record size: %zu\n", record_size);

    long rowid_offset = cell_offset + record_size_consumed;
    size_t rowid;
    long rowid_consumed;
    read_varint(database_file, rowid_offset, &rowid,
                &rowid_consumed); // TODO: if err

    fprintf(stderr, "rowid: %zu\n", rowid);

    // read record header and body
    long record_offset = rowid_offset + rowid_consumed;
    size_t record_header_size;
    long record_header_size_consumed;
    read_varint(database_file, record_offset, &record_header_size,
                &record_header_size_consumed); // TODO: if err

    fprintf(stderr, "record header size: %zu\n", record_header_size);

    // we know here we have 5 columns
    size_t columns[5];
    size_t column_idx = 0;

    // parse header
    size_t record_header_consumed = record_header_size_consumed;
    while (record_header_consumed < record_header_size) {
      size_t serial_type;
      long serial_type_consumed;
      read_varint(database_file, record_offset + record_header_consumed,
                  &serial_type,
                  &serial_type_consumed); // TODO: if err
      size_t content_size;
      calc_serial_type_size(serial_type, &content_size); // if err

      // save content_size
      columns[column_idx] = content_size;
      ++column_idx;

      fprintf(stderr, "column idx: %zu, serial type: %zu, content size: %zu\n",
              column_idx, serial_type, content_size);

      record_header_consumed += serial_type_consumed; // ???
    }

    // parse body
    size_t tbl_name_offset = columns[0] + columns[1];
    size_t tbl_name_size = columns[2];
    char *tbl_name = malloc(sizeof(char) * (tbl_name_size + 1));
    read_tbl_name(database_file,
                  record_offset + record_header_size + tbl_name_offset,
                  tbl_name_size, tbl_name);
    tbl_names[cell_idx] = tbl_name;
  }

  for (uint16_t cell_idx = 0; cell_idx < cells_count; ++cell_idx) {
    printf("%s ", tbl_names[cell_idx]);
  }
  printf("\n");
  result = 0;

free_tbl_names:
  free(tbl_names);
free_cell_offsets:
  free(cell_offsets);
cleanup:
  fclose(database_file);
  return result;
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
