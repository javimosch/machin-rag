# Vision path — images in the KB **without LLMs**

## Do we even need image descriptions?

**It depends on the embedder.**

| Setup | Need prose captions? |
|-------|----------------------|
| **Text-only embedder** (hash / ONNX text) | Something textual must enter the index — but that can be a **sidecar**, **OCR**, or just **folder + filename**. Full “this photo shows…” prose is optional. |
| **Multimodal embedder** (CLIP / SigLIP) | **No.** Pixels → vector; the query is embedded as text and matched in the same space. Descriptions become optional UX labels, not a retrieval requirement. |

For Cerise-style landing RAG (FAQ + “montre la photo retraite”), **folder taxonomy + short human sidecars** usually beats auto-generated captions. Atmosphere photos rarely OCR well; CLIP is the escape hatch only if path/sidecar recall fails.

## How to “describe” images without LLMs

1. **Human / agent sidecars** — `photo.jpg.md` written from the brief (best quality, zero models).
2. **Path taxonomy** — `retraite/haseya.jpg` → `dossier: retraite · fichier: haseya` (shipped as `vision: path`).
3. **OCR** — text *in* the image (flyers, slides); useless for most portraits/nature.
4. **Design alt text** — export from Figma/Canva if it already exists.
5. **CLIP** — skip language entirely; embed the bitmap.

**Not required for a solid KB:** VLM/LLM captioning. Prefer structure (folders, names, sidecars) over generated prose.

## Ladder (shipped → later)

### Phase 0 — Sidecar captions — **shipped**

`foo.jpg.md` / `.txt` → chunk with `payload.image`, `vision: sidecar`.

### Phase 0b — Path taxonomy — **shipped**

No sidecar? Index folder + filename (`vision: path`). Always works offline.

### Phase 1 — OCR — **shipped**

If `ocr.enabled` and Tesseract available, prefer OCR text over path-only when non-empty.

```bash
./machin-rag ingest -c kb -f path/to/photos/     # walk depth≤2
./machin-rag ingest -c kb -f photo.jpg           # sidecar → OCR → path
```

### Phase 2 — CLIP / SigLIP — deferred

Image → vector, no caption LLM. Only if photo-*intent* queries still miss after sidecars + path.

## Out of default path

| Approach | Why deferred |
|----------|----------------|
| Cloud vision APIs | BYOK creep |
| Local VLM captioners | LLM-shaped deps |
| Bytes in Qdrant payloads | Keep paths + sha256 |
| Face / biometric indexing | Non-goal |

## Cerise

- FAQ backbone: `testdata/cerise-landing/kb-cards.md`
- CI stubs: `testdata/cerise-landing/photos/*.jpg.md`
- Live tree: sidecars next to assets under `Documents/.../doc site internet Cerise/`
- Gate: `./machin-rag eval cerise` (includes `image-sidecar` + `contact-top1`)
