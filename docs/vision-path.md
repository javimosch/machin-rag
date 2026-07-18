# Vision path — images in the KB **without LLMs**

Cerise’s tree is mostly photos. Today standalone `.jpg`/`.png` are skipped. Default plan: **no LLM / no VLM captioning**. Images become searchable via human text, OCR, or direct image vectors.

## Default ladder (no LLMs)

### Phase 0 — Sidecar captions (human text)

For `foo.jpg`, ingest `foo.jpg.md` / `foo.jpg.txt` / `captions.jsonl`.

- Chunk text only; payload keeps `image`, `sha256`.
- Zero models. Best fit for a landing brief (“Retraite Haseya — cercle en montagne”).

### Phase 1 — Folder walk + local OCR (Tesseract soft-dep) — **shipped**

```bash
./machin-rag ingest -c kb -f path/to/photos/     # walks images (depth≤2)
./machin-rag ingest -c kb -f photo.jpg           # sidecar .md first, else OCR
```

- Sidecar wins (Phase 0). Else `ocr.enabled` + Tesseract → `vision: ocr`.
- Empty OCR still indexes `[image] path` so filename search works.
- Refuse with a clear hint if no sidecar and OCR off/unavailable.

### Phase 2 — Multimodal embeddings (CLIP / SigLIP) — still no caption LLM

- Image → vector **directly** (no generated description).
- Qdrant named vectors (`text` + `image`) or a sibling KB.
- Query: embed the question with the **text** tower; search image vectors; RRF/merge with text hits.
- Local ONNX weights only — not chat/VLM, not BYOK cloud vision.

## Explicitly out of the default path

| Approach | Why deferred |
|----------|----------------|
| Cloud vision APIs | BYOK creep |
| Local VLM captioners (Moondream, LLaVA, …) | LLM-shaped deps; optional later only if Phase 0+1+CLIP still fail |
| Storing image bytes in Qdrant payloads | Use paths + sha256 |
| Face / biometric indexing | Non-goal |

## Product shape

```bash
# Phase 0
./machin-rag ingest -c cerise-landing -f photos/captions.jsonl -F json

# Phase 1 (future)
./machin-rag ingest -c cerise-landing -f photos/retraite/haseya.jpg
# → payload { text, image, sha256, vision: "ocr"|"sidecar" }
```

## Cerise order of work

1. Keep `kb-cards.md` as FAQ backbone.
2. **Done — Phase 0:** `testdata/cerise-landing/photos/*.jpg.md`
3. **Done — Phase 1:** directory walk + OCR fallback (`ingest -f dir/`)
4. CLIP only if photo-*intent* queries still miss (“montre la photo retraite”).

Gate: `./machin-rag eval cerise` includes an `image-sidecar` case.
