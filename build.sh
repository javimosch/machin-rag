#!/usr/bin/env bash
# Build machin-rag: embed Vue UI, compose MFL sources, compile to one native binary.
set -euo pipefail
cd "$(dirname "$0")"

MACHIN="${MACHIN:-machin}"
command -v "$MACHIN" >/dev/null 2>&1 || { echo "error: '$MACHIN' not found (set MACHIN=/path/to/machin)"; exit 1; }

# 1. Embed ui/index.html as an MFL function (JSON string escaping == MFL literals).
python3 - <<'PY' > src/ui_gen.src
import json
html = open('ui/index.html').read()
print('func index_html() (h) { h = ' + json.dumps(html) + ' }')
PY

# 2. Compose sources (order does not matter for encode) then 3. compile.
"$MACHIN" encode \
  src/machweb.src src/flags.src src/embed.src src/config.src \
  src/parsers.src src/chunk.src src/qdrant.src src/api.src src/main.src src/ui_gen.src \
  > app.mfl
"$MACHIN" build app.mfl -o machin-rag

echo "built ./machin-rag"
echo "  docker compose up -d          # start Qdrant on :6333"
echo "  mkdir -p ~/.machin-rag && cp config.example.json ~/.machin-rag/config.json"
echo "  ./machin-rag status            # JSON reachability"
echo "  ./machin-rag serve             # API + Vue UI on :7091"
