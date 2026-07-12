#include "cursor.h"
#include "database.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int print_dbinfo(const Database *const db) {
  size_t page_size;
  if (database_page_size(db, &page_size) != 0) {
    fprintf(stderr, "Failed to read page size.\n");
    return -1;
  }
  printf("database page size: %zu\n", page_size);

  uint16_t cells_count;
  if (database_cells_count(db, &cells_count) != 0) {
    fprintf(stderr, "Failed to read cells count.\n");
    return -1;
  }
  // In this challenge, we assume that the database contains only tables - no
  // indexes, views, or other objects.
  printf("number of tables: %u\n", cells_count);
  return 0;
}

static int print_tables(const Database *const db) {
  int result = -1;
  uint16_t cells_count;
  if (database_cells_count(db, &cells_count) != 0) {
    fprintf(stderr, "Failed to read cells count.\n");
    goto cleanup;
  }

  uint16_t *cell_offsets = malloc(sizeof(uint16_t) * cells_count);
  if (cell_offsets == NULL) {
    goto cleanup;
  }
  Cursor cell_pointer_array_cursor;
  cursor_init(&cell_pointer_array_cursor, db->data, db->size,
              CELL_POINTER_ARRAY_OFFSET);
  for (uint16_t i = 0; i < cells_count; ++i) {
    if (cursor_read_u16_be(&cell_pointer_array_cursor, &cell_offsets[i]) != 0) {
      fprintf(stderr, "Failed to read cell pointer at index %u.\n", i);
      goto free_cell_offsets;
    }
  }

  char **tbl_names = malloc(sizeof(char *) * cells_count);
  if (tbl_names == NULL) {
    goto free_cell_offsets;
  }
  size_t parsed_tbl_names_count = 0;
  for (uint16_t cell_idx = 0; cell_idx < cells_count; ++cell_idx) {
    uint16_t cell_offset = cell_offsets[cell_idx];
    Cursor cell_cursor;
    cursor_init(&cell_cursor, db->data, db->size, cell_offset);
    size_t record_size;
    if (cursor_read_sqlite_varint(&cell_cursor, (uint64_t *)&record_size) !=
        0) {
      goto free_tbl_names;
    }

    fprintf(stderr, "record size: %zu\n", record_size);

    size_t rowid;
    if (cursor_read_sqlite_varint(&cell_cursor, (uint64_t *)&rowid) != 0) {
      goto free_tbl_names;
    }

    fprintf(stderr, "rowid: %zu\n", rowid);

    // read record header and body
    size_t record_offset = cell_cursor.offset;
    size_t record_header_size;
    if (cursor_read_sqlite_varint(&cell_cursor,
                                  (uint64_t *)&record_header_size) != 0) {
      goto free_tbl_names;
    }

    fprintf(stderr, "record header size: %zu\n", record_header_size);

    // we know here we have 5 columns
    size_t columns[5];
    size_t column_idx = 0;

    // parse header
    while (column_idx < 5) {
      size_t serial_type;
      if (cursor_read_sqlite_varint(&cell_cursor, (uint64_t *)&serial_type) !=
          0) {
        goto free_tbl_names;
      }
      size_t content_size;
      if (calc_serial_type_size(serial_type, &content_size) != 0) {
        goto free_tbl_names;
      }

      // save content_size
      columns[column_idx] = content_size;
      ++column_idx;

      fprintf(stderr, "column idx: %zu, serial type: %zu, content size: %zu\n",
              column_idx, serial_type, content_size);
    }

    // parse body
    size_t tbl_name_offset = columns[0] + columns[1];
    size_t tbl_name_size = columns[2];
    char *tbl_name = malloc(sizeof(char) * (tbl_name_size + 1));
    if (tbl_name == NULL) {
      goto free_tbl_names;
    }
    if (database_tbl_name(db,
                          record_offset + record_header_size + tbl_name_offset,
                          tbl_name_size, tbl_name) != 0) {
      free(tbl_name);
      goto free_tbl_names;
    }
    tbl_names[cell_idx] = tbl_name;
    ++parsed_tbl_names_count;
  }

  for (uint16_t cell_idx = 0; cell_idx < cells_count; ++cell_idx) {
    printf("%s ", tbl_names[cell_idx]);
  }
  printf("\n");
  result = 0;

free_tbl_names:
  for (size_t i = 0; i < parsed_tbl_names_count; ++i) {
    free(tbl_names[i]);
  }
  free(tbl_names);
free_cell_offsets:
  free(cell_offsets);
cleanup:
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

  Database db;
  if (database_open(&db, database_file_path) != 0) {
    fprintf(stderr, "Failed to open the database.\n");
    return EXIT_FAILURE;
  }

  int exit_code = EXIT_FAILURE;
  if (strcmp(command, ".dbinfo") == 0) {
    if (print_dbinfo(&db) == 0) {
      exit_code = EXIT_SUCCESS;
    }
    goto cleanup;
  } else if (strcmp(command, ".tables") == 0) {
    if (print_tables(&db) == 0) {
      exit_code = EXIT_SUCCESS;
    }
    goto cleanup;
  }
  fprintf(stderr, "Unknown command %s\n", command);

cleanup:
  database_close(&db);
  return exit_code;
}
