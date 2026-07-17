#!/usr/bin/env bash
# Smoke: md/csv/json/docx/pdf ingest + query against local Qdrant.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
BIN="${BIN:-./machin-rag}"
COLL="smoke_$$"
FIX="$ROOT/testdata/smoke"
QDRANT_URL="${QDRANT_URL:-http://127.0.0.1:6333}"

if [[ ! -x "$BIN" ]]; then
  echo "error: missing $BIN — run ./build.sh first" >&2
  exit 1
fi

# Qdrant reachability
if ! curl -sf "$QDRANT_URL/readyz" >/dev/null && ! curl -sf "$QDRANT_URL/" >/dev/null; then
  echo "error: Qdrant not reachable at $QDRANT_URL (docker compose up -d)" >&2
  exit 1
fi

echo "== status =="
"$BIN" status | tee /tmp/mr-smoke-status.json
python3 - <<'PY'
import json
d=json.load(open("/tmp/mr-smoke-status.json"))
assert d["ok"] and d["data"]["qdrant"]=="up"
print("formats", d["data"]["formats"])
print("ocr", d["data"].get("ocr"))
PY

echo "== ingest fixtures =="
"$BIN" ingest -c "$COLL" -f "$FIX/notes.md"
"$BIN" ingest -c "$COLL" -f "$FIX/rows.csv"
"$BIN" ingest -c "$COLL" -f "$FIX/items.json"
"$BIN" ingest -c "$COLL" -f "$FIX/sample.docx"
"$BIN" ingest -c "$COLL" -f "$FIX/sample.pdf"

echo "== query =="
"$BIN" query -c "$COLL" -q "native binary" -k 3 | tee /tmp/mr-smoke-query.json
python3 - <<'PY'
import json
d=json.load(open("/tmp/mr-smoke-query.json"))
assert d["ok"]
hits=d["data"]["result"]
assert len(hits)>=1
blob=" ".join(h.get("payload",{}).get("text","") for h in hits).lower()
assert "native" in blob or "binary" in blob or "machin" in blob
print("smoke OK", len(hits), "hits")
PY
