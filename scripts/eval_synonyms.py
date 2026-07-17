#!/usr/bin/env python3
"""Synonym-stress eval for hash-sha256-v1.

Docs use wording A; queries use wording B (little token overlap).
Low recall here is expected for a hash embedder — use it to decide ONNX.

Exit 1 if recall@k < EVAL_THRESHOLD (default 0.4). Always prints a report.
"""
from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = Path(os.environ.get("BIN", ROOT / "machin-rag"))
COLL = os.environ.get("EVAL_COLL", "eval_syn")
TOP_K = int(os.environ.get("EVAL_K", "3"))
THRESHOLD = float(os.environ.get("EVAL_THRESHOLD", "0.4"))

# query (synonym-ish) → needles that should appear in a retrieved doc
CASES = [
    ("system design overview", ["architecture", "blueprint", "topology"]),
    ("how fast is it", ["latency", "throughput", "speed"]),
    ("login and permissions", ["authentication", "bearer", "401"]),
    ("local vector store", ["vector database", "cosine", "offline"]),
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
    fix = ROOT / "testdata" / "eval-synonyms"
    docs = sorted(fix.glob("*.md"))
    if not docs:
        print(f"no fixtures in {fix}", file=sys.stderr)
        return 2

    run(["status"])
    for d in docs:
        run(["ingest", "-c", COLL, "-f", str(d)])

    hits_ok = 0
    print(f"synonym-stress provider=hash-sha256-v1 top_k={TOP_K} threshold={THRESHOLD}")
    for q, needles in CASES:
        data = run(["query", "-c", COLL, "-q", q, "-k", str(TOP_K)])
        texts = [h.get("payload", {}).get("text", "") for h in data["data"]["result"]]
        blob = "\n".join(texts).lower()
        ok = any(n.lower() in blob for n in needles)
        hits_ok += int(ok)
        mark = "PASS" if ok else "FAIL"
        top = (texts[0][:90] + "…") if texts else ""
        print(f"  [{mark}] q={q!r} needles={needles}")
        print(f"         top1={top!r}")

    recall = hits_ok / len(CASES)
    print(f"synonym_recall@{TOP_K}={recall:.2f} ({hits_ok}/{len(CASES)})")
    if recall < THRESHOLD:
        print(
            "Hash struggling on synonyms — candidate signal to prototype ONNX "
            "(see docs/embedder-roadmap.md).",
            file=sys.stderr,
        )
        return 1
    print("Hash still OK on this synonym suite — ONNX stays paused.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
