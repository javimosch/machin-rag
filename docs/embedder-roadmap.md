# Embedder roadmap — ONNX paused

**Status (2026-07):** stay on `hash-sha256-v2` for now.

## Current hash (`hash-sha256-v2`)

Same 64-d feature hashing as v1, plus:

- FR/EN stopword drop (`pour`, `dans`, `the`, …)
- light plural stem (trailing `s`)
- punctuation strip on tokens

**Re-ingest after upgrade** — vectors are not compatible with v1 collections.

## Why pause ONNX

- Lexical eval (`./machin-rag eval hash`) hits recall@3 = 1.0 on keyword-overlapping queries.
- Cerise landing gate (`./machin-rag eval cerise`) drives real French FAQ ranking without a model download.
- ONNX adds ~10–100MB weights, ABI/deps (onnxruntime), and a Qdrant dim migration.
- Agent notes/docs MVP is mostly keyword retrieval; hash is honest and free.

## When to unpause

Revisit ONNX only if the **synonym corpus** fails hard:

```bash
./machin-rag eval synonyms   # soft gate: min_pass=2/4
```

Rule of thumb:

- synonym pass ≥ 2/4 → keep hash (ONNX paused)
- synonym pass ≪ 2/4 on real queries → prototype ONNX behind the same `embed_json(text)` API
- `./machin-rag eval cerise` failing after cards + v2 → hybrid BM25 re-rank or ONNX
