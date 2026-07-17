# machin-rag

Agent-first **local RAG** for agents — single native binary ([machin](https://github.com/javimosch/machin)/MFL), **Qdrant**, **tiny local hash embedder** (no BYOK, no model download). Optional Vue 3 CDN UI for humans; CLI is JSON-only for agents.

Part of [**awesome-machin**](https://github.com/javimosch/awesome-machin). Docs: **[▶ landing](https://javimosch.github.io/machin-rag/)**.

## Why

Agents need **ingest → query → JSON** with free local embeddings and no panel tax. Pairs with [machin-colibri](https://github.com/javimosch/machin-colibri) / [bossless](https://github.com/javimosch/bossless) for retrieve→generate fully offline.

## Quick start

```bash
# needs: machin on PATH, docker, a C compiler + OpenSSL headers
docker compose up -d
./build.sh

mkdir -p ~/.machin-rag && cp config.example.json ~/.machin-rag/config.json   # optional

./machin-rag status
./machin-rag ingest -c docs -t "machin compiles MFL through C to one native binary"
./machin-rag ingest -c docs -f notes.md
./machin-rag ingest -c docs -f rows.csv
./machin-rag query  -c docs -q "native binary" -k 3
./machin-rag serve            # http://localhost:7091  (Vue UI + /api/*)
```

Config: `~/.machin-rag/config.json` → env → CLI. Env: `QDRANT_URL` (default `http://127.0.0.1:6333`), `PORT` (serve, default `7091`).

## Formats (v0.2)

| Format | Detection | Chunking |
|--------|-----------|----------|
| `md` | `.md` / headings | by `#` sections + size/overlap |
| `txt` | default / `-t` | size/overlap |
| `csv` | `.csv` | one chunk per row (header → fields) |
| `json` | `.json` | array of objects or single object |
| `ndjson` | `.ndjson` / `.jsonl` | one object per line |

Force with `-F` / `--format` or API `"format"`. PDF/DOCX not yet.

## Agent contract

| | |
|---|---|
| stdout | JSON `{"ok":true,"version","data"}` |
| stderr | JSON errors `{"ok":false,"error":{code,type,message}}` |
| exit | `0` ok · `82` validation · `85` bad arg · `106` qdrant/integration |
| discovery | `./machin-rag help-json` |

Commands: `status` · `collections` · `ingest` · `query` · `serve` · `version` · `help` · `help-json`.

### HTTP

| Method | Path | Body |
|--------|------|------|
| GET | `/api/health` | |
| GET | `/api/status` | |
| GET | `/api/collections` | |
| POST | `/api/ingest` | `{"collection","text"|"file","format?"}` |
| POST | `/api/query` | `{"collection","q","top_k?"}` |
| GET | `/` | Vue 3 UI |

## Embedder (honest)

MVP uses **feature hashing** over `sha256` tokens → 64-d cosine vectors (`hash-sha256-v1`). Zero cost, offline, keyword-ish quality. Same API surface later can point at an ONNX sidecar without changing agents.

## Layout

```
machin-rag/
├── src/                 # MFL: config, parsers, chunk, embed, qdrant, api, main
├── ui/index.html        # Vue 3 CDN (embedded at build)
├── config.example.json  # → ~/.machin-rag/config.json
├── compose.yml          # Qdrant only
├── build.sh
└── AGENTS.md
```

## Build

```bash
MACHIN=~/ai/machin/machin ./build.sh   # if machin not on PATH
```
