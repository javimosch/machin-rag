# OCR policy — document modes

machin-rag must ingest DOCX/PDF in three shapes. Decision for **v0.6**:

| Mode | How detected | Default behavior | OCR (`ocr.enabled`) |
|------|----------------|------------------|---------------------|
| **text-only** | Enough text (`>= ocr.min_text_chars`), no/empty media | Text layer only | Never runs |
| **mixed** | Enough text **and** images/empty pages | Text primary; image stubs (alt/sha256) | OCR images when `ocr.images=when_empty_alt` (alt empty) or `always` |
| **images-only** | Little/no text + images (or PDF pages with no text layer) | **Refuse** unless OCR on | Required — OCR **all** images/empty pages (ignores alt); fails if Tesseract missing |

## Why OCR is opt-in (not always-on)

- Keeps the default binary path tiny/fast (hash embed + text parsers).
- Tesseract is a **soft dependency** (`dlopen`) — build/run without it.
- Scanned/image-only docs are real, but most agent notes are text-only or mixed with alt text.

## Config (`~/.machin-rag/config.json`)

```json
"ocr": {
  "enabled": false,
  "lang": "eng",
  "min_text_chars": 40,
  "images": "when_empty_alt"
}
```

- `images`: `when_empty_alt` | `always` | `never`
- Env: `TESSDATA_PREFIX` — prefer the `tessdata` directory itself  
  (e.g. `/usr/share/tesseract-ocr/4.00/tessdata`), or its parent.

## Runtime packages (Ubuntu)

```bash
# PDF text-layer (hard dependency of the PDF code path)
sudo apt install libpoppler-cpp0v5

# OCR (optional soft dependency)
sudo apt install libtesseract4 tesseract-ocr-eng
```

`./machin-rag status` reports `ocr.enabled` / `ocr.available`.
