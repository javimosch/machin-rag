/* mr_zip.c — minimal ZIP reader (store + deflate) for DOCX/OOXML.
 * Linked into machin-rag via extern cflags. Depends on zlib (-lz).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <openssl/sha.h>

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static uint8_t *slurp(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 22) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    *out_len = (size_t)sz;
    return buf;
}

/* Find EOCD (signature 0x06054b50) scanning backward from end. */
static const uint8_t *find_eocd(const uint8_t *data, size_t len) {
    if (len < 22) return NULL;
    size_t max_back = len > 65557 ? 65557 : len;
    size_t i = len - 22;
    for (;;) {
        if (rd32(data + i) == 0x06054b50u) return data + i;
        if (i == len - max_back) break;
        i--;
    }
    return NULL;
}

static int inflate_raw(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len) {
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    strm.next_in = (Bytef *)src;
    strm.avail_in = (uInt)src_len;
    strm.next_out = dst;
    strm.avail_out = (uInt)dst_len;
    /* -MAX_WBITS = raw DEFLATE (ZIP method 8), no zlib/gzip wrapper */
    if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) return -1;
    int rc = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);
    if (rc != Z_STREAM_END) return -1;
    if (strm.total_out != dst_len) return -1;
    return 0;
}

typedef struct {
    uint32_t comp_size;
    uint32_t uncomp_size;
    uint16_t method;
    uint16_t name_len;
    uint16_t extra_len;
    uint16_t comment_len;
    uint32_t local_off;
    const uint8_t *name;
} ZipEntry;

static int cd_entry_at(const uint8_t *data, size_t len, size_t off, ZipEntry *e) {
    if (off + 46 > len) return -1;
    if (rd32(data + off) != 0x02014b50u) return -1;
    e->method = rd16(data + off + 10);
    e->comp_size = rd32(data + off + 20);
    e->uncomp_size = rd32(data + off + 24);
    e->name_len = rd16(data + off + 28);
    e->extra_len = rd16(data + off + 30);
    e->comment_len = rd16(data + off + 32);
    e->local_off = rd32(data + off + 42);
    if (off + 46 + e->name_len > len) return -1;
    e->name = data + off + 46;
    return 0;
}

static int extract_entry(const uint8_t *data, size_t len, const ZipEntry *e,
                         uint8_t **out, size_t *out_len) {
    size_t lo = (size_t)e->local_off;
    if (lo + 30 > len) return -1;
    if (rd32(data + lo) != 0x04034b50u) return -1;
    uint16_t lname = rd16(data + lo + 26);
    uint16_t lextra = rd16(data + lo + 28);
    size_t data_off = lo + 30 + lname + lextra;
    if (data_off + e->comp_size > len) return -1;
    const uint8_t *payload = data + data_off;
    uint8_t *dst = (uint8_t *)malloc(e->uncomp_size ? e->uncomp_size : 1);
    if (!dst) return -1;
    if (e->method == 0) {
        if (e->comp_size != e->uncomp_size) { free(dst); return -1; }
        memcpy(dst, payload, e->uncomp_size);
    } else if (e->method == 8) {
        if (inflate_raw(payload, e->comp_size, dst, e->uncomp_size) != 0) {
            free(dst); return -1;
        }
    } else {
        free(dst); return -1;
    }
    *out = dst;
    *out_len = e->uncomp_size;
    return 0;
}

/* Copy entry bytes into caller buffer; NUL-terminate if room. Returns length, or -1. */
int64_t mr_zip_read(const char *zip_path, const char *entry_name, char *out, int64_t out_cap) {
    if (!zip_path || !entry_name || !out || out_cap < 1) return -1;
    size_t zlen = 0;
    uint8_t *data = slurp(zip_path, &zlen);
    if (!data) return -1;
    const uint8_t *eocd = find_eocd(data, zlen);
    if (!eocd) { free(data); return -1; }
    uint16_t nent = rd16(eocd + 10);
    uint32_t cd_size = rd32(eocd + 12);
    uint32_t cd_off = rd32(eocd + 16);
    if ((size_t)cd_off + cd_size > zlen) { free(data); return -1; }

    size_t want = strlen(entry_name);
    size_t off = cd_off;
    int64_t result = -1;
    for (uint16_t i = 0; i < nent; i++) {
        ZipEntry e;
        if (cd_entry_at(data, zlen, off, &e) != 0) break;
        size_t next = off + 46 + e.name_len + e.extra_len + e.comment_len;
        if (e.name_len == want && memcmp(e.name, entry_name, want) == 0) {
            uint8_t *raw = NULL;
            size_t raw_len = 0;
            if (extract_entry(data, zlen, &e, &raw, &raw_len) == 0) {
                if ((int64_t)raw_len < out_cap) {
                    memcpy(out, raw, raw_len);
                    out[raw_len] = '\0';
                    result = (int64_t)raw_len;
                } else if ((int64_t)raw_len == out_cap) {
                    memcpy(out, raw, raw_len);
                    result = (int64_t)raw_len; /* full, no NUL */
                }
                free(raw);
            }
            break;
        }
        off = next;
    }
    free(data);
    return result;
}

/* Uncompressed size of entry, or -1. */
int64_t mr_zip_entry_size(const char *zip_path, const char *entry_name) {
    if (!zip_path || !entry_name) return -1;
    size_t zlen = 0;
    uint8_t *data = slurp(zip_path, &zlen);
    if (!data) return -1;
    const uint8_t *eocd = find_eocd(data, zlen);
    if (!eocd) { free(data); return -1; }
    uint16_t nent = rd16(eocd + 10);
    uint32_t cd_off = rd32(eocd + 16);
    size_t want = strlen(entry_name);
    size_t off = cd_off;
    int64_t result = -1;
    for (uint16_t i = 0; i < nent; i++) {
        ZipEntry e;
        if (cd_entry_at(data, zlen, off, &e) != 0) break;
        size_t next = off + 46 + e.name_len + e.extra_len + e.comment_len;
        if (e.name_len == want && memcmp(e.name, entry_name, want) == 0) {
            result = (int64_t)e.uncomp_size;
            break;
        }
        off = next;
    }
    free(data);
    return result;
}

/* SHA-256 hex (64 chars + NUL) of an entry's uncompressed bytes. Returns 64, or -1. */
int64_t mr_zip_sha256_hex(const char *zip_path, const char *entry_name, char *out, int64_t out_cap) {
    if (!zip_path || !entry_name || !out || out_cap < 65) return -1;
    size_t zlen = 0;
    uint8_t *data = slurp(zip_path, &zlen);
    if (!data) return -1;
    const uint8_t *eocd = find_eocd(data, zlen);
    if (!eocd) { free(data); return -1; }
    uint16_t nent = rd16(eocd + 10);
    uint32_t cd_off = rd32(eocd + 16);
    size_t want = strlen(entry_name);
    size_t off = cd_off;
    int64_t result = -1;
    for (uint16_t i = 0; i < nent; i++) {
        ZipEntry e;
        if (cd_entry_at(data, zlen, off, &e) != 0) break;
        size_t next = off + 46 + e.name_len + e.extra_len + e.comment_len;
        if (e.name_len == want && memcmp(e.name, entry_name, want) == 0) {
            uint8_t *raw = NULL;
            size_t raw_len = 0;
            if (extract_entry(data, zlen, &e, &raw, &raw_len) == 0) {
                unsigned char dig[SHA256_DIGEST_LENGTH];
                SHA256(raw, raw_len, dig);
                free(raw);
                static const char *hexd = "0123456789abcdef";
                for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                    out[i * 2] = hexd[(dig[i] >> 4) & 0xf];
                    out[i * 2 + 1] = hexd[dig[i] & 0xf];
                }
                out[64] = '\0';
                result = 64;
            }
            break;
        }
        off = next;
    }
    free(data);
    return result;
}

/* List entry names with given prefix into out as "a\nb\n". Returns count, or -1. */
int64_t mr_zip_list(const char *zip_path, const char *prefix, char *out, int64_t out_cap) {
    if (!zip_path || !prefix || !out || out_cap < 1) return -1;
    out[0] = '\0';
    size_t zlen = 0;
    uint8_t *data = slurp(zip_path, &zlen);
    if (!data) return -1;
    const uint8_t *eocd = find_eocd(data, zlen);
    if (!eocd) { free(data); return -1; }
    uint16_t nent = rd16(eocd + 10);
    uint32_t cd_off = rd32(eocd + 16);
    size_t plen = strlen(prefix);
    size_t off = cd_off;
    int64_t count = 0;
    int64_t used = 0;
    for (uint16_t i = 0; i < nent; i++) {
        ZipEntry e;
        if (cd_entry_at(data, zlen, off, &e) != 0) break;
        size_t next = off + 46 + e.name_len + e.extra_len + e.comment_len;
        if (e.name_len >= plen && memcmp(e.name, prefix, plen) == 0) {
            /* skip directory-only entries ending with / */
            if (e.name_len > 0 && e.name[e.name_len - 1] != '/') {
                if (used + (int64_t)e.name_len + 2 > out_cap) { free(data); return -1; }
                memcpy(out + used, e.name, e.name_len);
                used += e.name_len;
                out[used++] = '\n';
                out[used] = '\0';
                count++;
            }
        }
        off = next;
    }
    free(data);
    return count;
}
