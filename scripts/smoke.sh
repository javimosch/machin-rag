#!/usr/bin/env bash
# Smoke: md/csv/json/docx/pdf/image-sidecar ingest + query against local Qdrant.
# Asserts use jq only (no Python).
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
if ! command -v jq >/dev/null 2>&1; then
  echo "error: jq required for smoke asserts (apt install jq)" >&2
  exit 1
fi

# Qdrant reachability
if ! curl -sf "$QDRANT_URL/readyz" >/dev/null && ! curl -sf "$QDRANT_URL/" >/dev/null; then
  echo "error: Qdrant not reachable at $QDRANT_URL (docker compose up -d)" >&2
  exit 1
fi

echo "== status =="
"$BIN" status | tee /tmp/mr-smoke-status.json
jq -e '.ok == true and .data.qdrant == "up"' /tmp/mr-smoke-status.json >/dev/null
echo "formats $(jq -c '.data.formats' /tmp/mr-smoke-status.json)"
echo "ocr $(jq -c '.data.ocr' /tmp/mr-smoke-status.json)"

echo "== ingest fixtures =="
"$BIN" ingest -c "$COLL" -f "$FIX/notes.md"
"$BIN" ingest -c "$COLL" -f "$FIX/rows.csv"
"$BIN" ingest -c "$COLL" -f "$FIX/items.json"
"$BIN" ingest -c "$COLL" -f "$FIX/sample.docx"
"$BIN" ingest -c "$COLL" -f "$FIX/sample.pdf"
# Phase-0 image sidecar (no OCR / no LLM)
"$BIN" ingest -c "$COLL" -f "$ROOT/testdata/cerise-landing/photos/retraite-haseya.jpg"

echo "== query =="
"$BIN" query -c "$COLL" -q "native binary" -k 3 | tee /tmp/mr-smoke-query.json
jq -e '.ok == true and (.data.result | length) >= 1' /tmp/mr-smoke-query.json >/dev/null
blob="$(jq -r '[.data.result[].payload.text] | join(" ") | ascii_downcase' /tmp/mr-smoke-query.json)"
if [[ "$blob" != *native* && "$blob" != *binary* && "$blob" != *machin* ]]; then
  echo "error: query blob missing expected tokens: $blob" >&2
  exit 1
fi
echo "smoke OK $(jq '.data.result | length' /tmp/mr-smoke-query.json) hits"

echo "== multi-KB isolation =="
KB_A="${COLL}_a"
KB_B="${COLL}_b"
"$BIN" kb create -c "$KB_A"
"$BIN" kb create -c "$KB_B"
"$BIN" ingest -c "$KB_A" -t "Cerise landing doula retreats water"
"$BIN" ingest -c "$KB_B" -t "Unrelated rocket telemetry firmware"
"$BIN" query -c "$KB_A" -q "doula" -k 3 | tee /tmp/mr-smoke-kba.json
"$BIN" query -c "$KB_B" -q "doula" -k 3 | tee /tmp/mr-smoke-kbb.json
jq -e '.ok == true' /tmp/mr-smoke-kba.json >/dev/null
jq -e '.ok == true' /tmp/mr-smoke-kbb.json >/dev/null
ta="$(jq -r '[.data.result[].payload.text] | join(" ") | ascii_downcase' /tmp/mr-smoke-kba.json)"
tb="$(jq -r '[.data.result[].payload.text] | join(" ") | ascii_downcase' /tmp/mr-smoke-kbb.json)"
if [[ "$ta" != *doula* && "$ta" != *cerise* ]]; then
  echo "error: KB_A missing doula/cerise: $ta" >&2
  exit 1
fi
if [[ "$tb" != *rocket* && "$tb" != *telemetry* && "$tb" != *firmware* ]]; then
  echo "error: KB_B missing rocket terms: $tb" >&2
  exit 1
fi
if [[ "$ta" == *rocket* ]]; then
  echo "error: KB_A leaked rocket text" >&2
  exit 1
fi
echo "multi-KB OK"

echo "== dir walk (Phase-0 sidecars) =="
"$BIN" ingest -c "$COLL" -f "$ROOT/testdata/cerise-landing/photos" | tee /tmp/mr-smoke-dir.json
jq -e '.ok == true' /tmp/mr-smoke-dir.json >/dev/null
"$BIN" query -c "$COLL" -q "haseya montagne" -k 3 | tee /tmp/mr-smoke-img.json
jq -e '.ok == true' /tmp/mr-smoke-img.json >/dev/null
imgblob="$(jq -r '[.data.result[].payload.text] | join(" ") | ascii_downcase' /tmp/mr-smoke-img.json)"
if [[ "$imgblob" != *haseya* && "$imgblob" != *retraite* ]]; then
  echo "error: dir-walk query missed image caption: $imgblob" >&2
  exit 1
fi
echo "dir-walk OK"

"$BIN" kb delete -c "$KB_A" -y >/dev/null
"$BIN" kb delete -c "$KB_B" -y >/dev/null
"$BIN" kb delete -c "$COLL" -y >/dev/null || true
