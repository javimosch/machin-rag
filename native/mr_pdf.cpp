/* mr_pdf.cpp — PDF text-layer (+ optional page raster) via poppler-cpp.
 * Links: -lpoppler-cpp -lstdc++
 */
#include "poppler-document.h"
#include "poppler-page.h"
#include "poppler-page-renderer.h"
#include "poppler-image.h"

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

/* Rasterize page to PNG for OCR. Returns 1 on success, -1 on failure. */
extern "C" int64_t mr_pdf_page_png(const char *path, int64_t page0, const char *out_png, double dpi) {
    if (!path || !out_png || page0 < 0) return -1;
    if (dpi < 36.0) dpi = 72.0;
    if (dpi > 300.0) dpi = 300.0;
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
    poppler::page_renderer r;
    r.set_render_hint(poppler::page_renderer::antialiasing, true);
    r.set_render_hint(poppler::page_renderer::text_antialiasing, true);
    poppler::image img = r.render_page(p, dpi, dpi);
    delete p;
    delete doc;
    if (!img.is_valid()) return -1;
    if (!img.save(out_png, "png")) return -1;
    return 1;
}

extern "C" int64_t mr_pdf_can_render(void) {
    return poppler::page_renderer::can_render() ? 1 : 0;
}
