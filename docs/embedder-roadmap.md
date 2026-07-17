# Embedder roadmap — ONNX paused

**Status (2026-07):** stay on `hash-sha256-v1` for now.

## Why pause ONNX

- Smoke lexical eval (`scripts/eval_hash.py`) hits recall@3 = 1.0 on keyword-overlapping queries.
- ONNX adds ~10–100MB weights, ABI/deps (onnxruntime), and a Qdrant dim migration.
- Agent notes/docs MVP is mostly keyword retrieval; hash is honest and free.

## When to unpause

Revisit ONNX only if the **synonym corpus** fails hard — see `testdata/eval-synonyms/` + `scripts/eval_synonyms.py`.

Rule of thumb:

- synonym recall@3 ≥ ~0.6 → keep hash
- synonym recall@3 ≪ 0.4 on real queries → prototype ONNX behind the same `embed_json(text)` API
