#!/usr/bin/env python3
"""Eval hash-sha256-v1 recall@k on a tiny synonym-ish corpus.

Exit 0 always prints a report; exit 1 if recall@k below threshold.
Requires: ./machin-rag, Qdrant up, fixtures in testdata/eval/.
"""
from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = Path(os.environ.get("BIN", ROOT / "machin-rag"))
COLL = os.environ.get("EVAL_COLL", "eval_hash")
TOP_K = int(os.environ.get("EVAL_K", "3"))
THRESHOLD = float(os.environ.get("EVAL_THRESHOLD", "0.5"))

# query → any of these substrings must appear in a top-k hit text
CASES = [
    ("native binary", ["native binary", "compiles mfl", "machin compiles"]),
    ("cosine similarity", ["cosine", "qdrant", "vectors"]),
    ("embeddings", ["embedding", "embeddings", "hash"]),
    ("offline rag", ["offline", "rag", "agent"]),
]


def run(args: list[str]) -> dict:
    p = subprocess.run([str(BIN), *args], capture_output=True, text=True)
    if p.returncode != 0:
        print(p.stderr or p.stdout, file=sys.stderr)
        raise SystemExit(f"command failed: {args} exit={p.returncode}")
    return json.loads(p.stdout)


def main() -> int:
    if not BIN.exists():
        print(f"missing binary {BIN}", file=sys.stderr)
        return 2
    fix = ROOT / "testdata" / "eval"
    docs = sorted(fix.glob("*"))
    if not docs:
        print(f"no fixtures in {fix}", file=sys.stderr)
        return 2

    run(["status"])
    for d in docs:
        if d.suffix.lower() in {".md", ".txt", ".csv", ".json", ".docx", ".pdf"}:
            run(["ingest", "-c", COLL, "-f", str(d)])

    hits_ok = 0
    print(f"provider=hash-sha256-v1 top_k={TOP_K} threshold={THRESHOLD}")
    for q, needles in CASES:
        data = run(["query", "-c", COLL, "-q", q, "-k", str(TOP_K)])
        texts = [h.get("payload", {}).get("text", "") for h in data["data"]["result"]]
        blob = "\n".join(texts).lower()
        ok = any(n.lower() in blob for n in needles)
        hits_ok += int(ok)
        mark = "PASS" if ok else "FAIL"
        print(f"  [{mark}] q={q!r} needles={needles} top1={(texts[0][:80] + '…') if texts else ''!r}")

    recall = hits_ok / len(CASES)
    print(f"recall@{TOP_K}={recall:.2f} ({hits_ok}/{len(CASES)})")
    if recall < THRESHOLD:
        print(
            "NOTE: hash embedder is lexical — low synonym recall is expected. "
            "Consider ONNX only if this blocks real workloads.",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
