/* mr_pdf.cpp — PDF text-layer extract via poppler-cpp (C ABI for machin).
 * No OCR: only embedded text. Links: -lpoppler-cpp -lstdc++
 */
#include "poppler-document.h"
#include "poppler-page.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using poppler::document;
using poppler::page;
using poppler::ustring;

static std::string u8(const ustring &u) {
    poppler::byte_array b = u.to_utf8();
    return std::string(b.begin(), b.end());
}

extern "C" int64_t mr_pdf_page_count(const char *path) {
    if (!path) return -1;
    document *doc = document::load_from_file(path);
    if (!doc || doc->is_locked()) {
        delete doc;
        return -1;
    }
    int64_t n = doc->pages();
    delete doc;
    return n;
}

extern "C" int64_t mr_pdf_title(const char *path, char *out, int64_t out_cap) {
    if (!path || !out || out_cap < 1) return -1;
    out[0] = '\0';
    document *doc = document::load_from_file(path);
    if (!doc || doc->is_locked()) {
        delete doc;
        return -1;
    }
    std::string t = u8(doc->get_title());
    delete doc;
    if ((int64_t)t.size() >= out_cap) return -1;
    memcpy(out, t.data(), t.size());
    out[t.size()] = '\0';
    return (int64_t)t.size();
}

/* Extract text for 0-based page. Returns byte length, or -1. */
extern "C" int64_t mr_pdf_page_text(const char *path, int64_t page0, char *out, int64_t out_cap) {
    if (!path || !out || out_cap < 1 || page0 < 0) return -1;
    out[0] = '\0';
    document *doc = document::load_from_file(path);
    if (!doc || doc->is_locked()) {
        delete doc;
        return -1;
    }
    if (page0 >= doc->pages()) {
        delete doc;
        return -1;
    }
    page *p = doc->create_page((int)page0);
    if (!p) {
        delete doc;
        return -1;
    }
    std::string t = u8(p->text());
    delete p;
    delete doc;
    if ((int64_t)t.size() >= out_cap) return -1;
    memcpy(out, t.data(), t.size());
    out[t.size()] = '\0';
    return (int64_t)t.size();
}
