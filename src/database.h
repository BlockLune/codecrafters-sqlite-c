#ifndef DATABASE_H
#define DATABASE_H

#include <stddef.h>
#include <stdint.h>

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

typedef struct {
  int fd;
  const uint8_t *data;
  size_t size;
} Database;

int database_open(Database *const db, const char *const path);

void database_close(Database *const db);

int database_page_size(const Database *const db, size_t *const page_size);

int database_cells_count(const Database *const db, uint16_t *const cells_count);

int database_tbl_name(const Database *const db, const size_t offset,
                      const size_t size, char *const dest);

#endif
