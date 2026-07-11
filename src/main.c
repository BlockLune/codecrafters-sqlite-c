#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    // Skip the first 16 bytes of the header, which is "SQLite format 3" + null
    // terminator
    fseek(database_file, 16, SEEK_SET);
    unsigned char buffer[2];
    fread(buffer, 1, 2, database_file);
    uint16_t page_size = (buffer[0] << 8) | buffer[1];
    printf("database page size: %u\n", page_size);

    // The `sqlite_schema` page is always page 1
    // First 100 bytes of page 1 is database header, so the actual btree page
    // starts from 100
    const size_t SQLITE_SCHEMA_BTREE_OFFSET = 100;
    const size_t N_CELLLS_OFFSET = SQLITE_SCHEMA_BTREE_OFFSET + 3;
    fseek(database_file, N_CELLLS_OFFSET, SEEK_SET);
    fread(buffer, 1, 2, database_file);
    uint16_t n_cells = (buffer[0] << 8) | buffer[1];
    printf("number of tables: %u\n", n_cells);

    fclose(database_file);
  } else {
    fprintf(stderr, "Unknown command %s\n", command);
    return 1;
  }

  return 0;
}
