/* mr_ocr.c — optional Tesseract OCR via dlopen (soft dependency).
 * Binary builds/runs without libtesseract; OCR fails clearly when missing.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct TessBaseAPI TessBaseAPI;

typedef TessBaseAPI *(*fn_create)(void);
typedef void (*fn_delete)(TessBaseAPI *);
typedef int (*fn_init3)(TessBaseAPI *, const char *, const char *);
typedef int (*fn_process)(TessBaseAPI *, const char *, const char *, int, void *);
typedef char *(*fn_get_text)(TessBaseAPI *);
typedef void (*fn_end)(TessBaseAPI *);
typedef void (*fn_free)(void *);

static void *g_tess = NULL;
static fn_create g_create = NULL;
static fn_delete g_delete = NULL;
static fn_init3 g_init3 = NULL;
static fn_process g_process = NULL;
static fn_get_text g_get_text = NULL;
static fn_end g_end = NULL;
static fn_free g_delete_text = NULL;
static int g_tried = 0;
static int g_ok = 0;

static int load_tess(void) {
    if (g_tried) return g_ok;
    g_tried = 1;
    const char *cands[] = {
        "libtesseract.so.4",
        "libtesseract.so.5",
        "libtesseract.so",
        NULL
    };
    for (int i = 0; cands[i]; i++) {
        g_tess = dlopen(cands[i], RTLD_LAZY | RTLD_LOCAL);
        if (g_tess) break;
    }
    if (!g_tess) return 0;
    g_create = (fn_create)dlsym(g_tess, "TessBaseAPICreate");
    g_delete = (fn_delete)dlsym(g_tess, "TessBaseAPIDelete");
    g_init3 = (fn_init3)dlsym(g_tess, "TessBaseAPIInit3");
    g_process = (fn_process)dlsym(g_tess, "TessBaseAPIProcessPages");
    g_get_text = (fn_get_text)dlsym(g_tess, "TessBaseAPIGetUTF8Text");
    g_end = (fn_end)dlsym(g_tess, "TessBaseAPIEnd");
    g_delete_text = (fn_free)dlsym(g_tess, "TessDeleteText");
    if (!g_create || !g_delete || !g_init3 || !g_process || !g_get_text || !g_end) {
        dlclose(g_tess);
        g_tess = NULL;
        return 0;
    }
    g_ok = 1;
    return 1;
}

/* 1 if libtesseract is loadable, else 0. */
int64_t mr_ocr_available(void) {
    return load_tess() ? 1 : 0;
}

static const char *tessdata_path(void) {
    const char *env = getenv("TESSDATA_PREFIX");
    if (env && env[0]) return env;
    /* common Ubuntu paths (parent of tessdata/) */
    if (access("/usr/share/tesseract-ocr/5/tessdata/eng.traineddata", R_OK) == 0)
        return "/usr/share/tesseract-ocr/5/";
    if (access("/usr/share/tesseract-ocr/4.00/tessdata/eng.traineddata", R_OK) == 0)
        return "/usr/share/tesseract-ocr/4.00/";
    if (access("/usr/share/tessdata/eng.traineddata", R_OK) == 0)
        return "/usr/share/";
    return "/usr/share/tesseract-ocr/4.00/";
}

/* OCR an image file (png/jpg/tif…). Returns length, or -1. */
int64_t mr_ocr_file(const char *image_path, const char *lang, char *out, int64_t out_cap) {
    if (!image_path || !out || out_cap < 1) return -1;
    out[0] = '\0';
    if (!load_tess()) return -1;
    const char *lg = (lang && lang[0]) ? lang : "eng";
    TessBaseAPI *api = g_create();
    if (!api) return -1;
    if (g_init3(api, tessdata_path(), lg) != 0) {
        g_delete(api);
        return -1;
    }
    if (!g_process(api, image_path, NULL, 0, NULL)) {
        g_end(api);
        g_delete(api);
        return -1;
    }
    char *text = g_get_text(api);
    g_end(api);
    g_delete(api);
    if (!text) return -1;
    size_t n = strlen(text);
    if ((int64_t)n >= out_cap) {
        if (g_delete_text) g_delete_text(text);
        else free(text);
        return -1;
    }
    memcpy(out, text, n);
    out[n] = '\0';
    if (g_delete_text) g_delete_text(text);
    else free(text);
    return (int64_t)n;
}
