#include "psp_cso.h"
#include <limits.h>
#include <string.h>
#include <zlib.h>

static uint32_t le32(const uint8_t *p) {
 return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint64_t le64(const uint8_t *p) {
 return (uint64_t)le32(p) | (uint64_t)le32(p + 4) << 32;
}
static int at(FILE *f, uint64_t position, void *dst, size_t size) {
 /* A conservative portable stdio bound, including 32-bit long targets. */
 if (position > LONG_MAX || size > (uint64_t)LONG_MAX - position) return -1;
 if (fseek(f, (long)position, SEEK_SET) != 0) return -1;
 return fread(dst, 1, size, f) == size ? 0 : -1;
}
int psp_cso_open(PspCso *cso, const char *path) {
 if (!cso || !path) return -1;
 memset(cso, 0, sizeof(*cso));
 FILE *f = fopen(path, "rb");
 if (!f) return -1;
 uint8_t header[24];
 if (fseek(f, 0, SEEK_END) != 0) goto fail;
 long length = ftell(f);
 if (length < (long)sizeof header || at(f, 0, header, sizeof header) != 0) goto fail;
 uint64_t plain_size = le64(header + 8);
 if (memcmp(header, "CISO", 4) ||
     (le32(header + 4) != 0 && le32(header + 4) != 24) ||
     header[20] != 1 || header[21] > 15 || le32(header + 16) != 2048 ||
     !plain_size || plain_size % 2048 != 0 || plain_size / 2048 > UINT32_MAX)
  goto fail;
 uint64_t sectors = plain_size / 2048;
 uint64_t index_end = 24 + (sectors + 1) * 4;
 if (index_end > (uint64_t)length) goto fail;
 /* Validate terminal index and first block position, without an O(n) allocation. */
 uint8_t entry[4], end[4];
 if (at(f, 24, entry, 4) || at(f, 24 + sectors * 4, end, 4)) goto fail;
 uint64_t first = (uint64_t)(le32(entry) & 0x7fffffffU) << header[21];
 uint64_t last = (uint64_t)(le32(end) & 0x7fffffffU) << header[21];
 if (first < index_end || first > last || last > (uint64_t)length) goto fail;
 cso->file = f;
 cso->sectors = (uint32_t)sectors;
 cso->alignment = header[21];
 cso->file_size = (uint64_t)length;
 cso->index_end = index_end;
 return 0;
fail:
 fclose(f);
 return -1;
}
void psp_cso_close(PspCso *cso) {
 if (!cso) return;
 if (cso->file) fclose(cso->file);
 memset(cso, 0, sizeof(*cso));
}
int psp_cso_read(void *ctx, uint32_t lba, uint8_t *out) {
 PspCso *cso = ctx;
 if (!cso || !cso->file || !out || lba >= cso->sectors) return -1;
 uint8_t entries[8];
 if (at(cso->file, 24 + (uint64_t)lba * 4, entries, sizeof entries)) return -1;
 uint32_t current = le32(entries), next = le32(entries + 4);
 uint64_t start = (uint64_t)(current & 0x7fffffffU) << cso->alignment;
 uint64_t end = (uint64_t)(next & 0x7fffffffU) << cso->alignment;
 if (start < cso->index_end || end < start || end > cso->file_size) return -1;
 if (current & 0x80000000U) {
  if (end - start < 2048) return -1;
  return at(cso->file, start, out, 2048);
 }
 uint64_t bytes = end - start;
 if (!bytes || bytes > 4096) return -1;
 uint8_t compressed[4096];
 if (at(cso->file, start, compressed, (size_t)bytes)) return -1;
 uint8_t decompressed[2048];
 z_stream stream;
 memset(&stream, 0, sizeof stream);
 stream.next_in = compressed;
 stream.avail_in = (uInt)bytes;
 stream.next_out = decompressed;
 stream.avail_out = sizeof decompressed;
 if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) return -1;
 int status = inflate(&stream, Z_FINISH);
 int valid = status == Z_STREAM_END && stream.total_out == sizeof decompressed;
 inflateEnd(&stream);
 if (!valid) return -1;
 memcpy(out, decompressed, sizeof decompressed);
 return 0;
}
int psp_cso_cached(PspCso *cso, PspPerf *perf, uint32_t lba, uint8_t *out) {
 if (!cso || !perf || lba >= cso->sectors) return -1;
 return psp_perf_read_sector(perf, lba, psp_cso_read, cso, out);
}
