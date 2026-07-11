#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_u16_be(FILE *file, long offset, uint16_t *value) {
  uint8_t buf[2];
  if (fseek(file, offset, SEEK_SET) != 0) {
    return -1;
  }
  if (fread(buf, sizeof(buf), 1, file) != 0) {
    return -1;
  }
  *value = ((uint16_t)buf[0] << 8) | buf[1];
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: ./your_program.sh <database path> <command>\n");
    return 1;
  }

  fprintf(stderr, "====== CodeCrafters SQLite C ======\n");

  const char *database_file_path = argv[1];
  const char *command = argv[2];

  if (strcmp(command, ".dbinfo") == 0) {
    FILE *database_file = fopen(database_file_path, "rb");
    if (!database_file) {
      fprintf(stderr, "Failed to open the database file\n");
      return 1;
    }

    // Skip the first 16 bytes of the header, which is "SQLite format 3"
    // followed by a null terminator.
    const size_t SQLITE_PAGE_SIZE_OFFSET = 16;
    uint16_t raw_page_size;
    read_u16_be(database_file, SQLITE_PAGE_SIZE_OFFSET, &raw_page_size);
    size_t page_size = raw_page_size == 1 ? 65535 : raw_page_size;
    printf("database page size: %zu\n", page_size);

    // The `sqlite_schema` table is always stored on page 1.
    // The first 100 bytes of page 1 contain the database header, so the actual
    // B-tree page starts at offset 100.
    const size_t SQLITE_SCHEMA_BTREE_OFFSET = 100;
    const size_t N_CELLLS_OFFSET = SQLITE_SCHEMA_BTREE_OFFSET + 3;
    uint16_t cells_count;
    read_u16_be(database_file, N_CELLLS_OFFSET, &cells_count);
    // In this challenge, we assume that the database contains only tables - no
    // indexes, views, or other objects.
    printf("number of tables: %u\n", cells_count);

    fclose(database_file);
  } else {
    fprintf(stderr, "Unknown command %s\n", command);
    return 1;
  }

  return 0;
}
