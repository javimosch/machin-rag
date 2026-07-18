# Cerise landing corpus (backtest)

Source folder: `Documents/projects/doc site internet Cerise/`

| File | Role |
|------|------|
| `kb-cards.md` | Curated `##` service cards (FAQ retrieval backbone) |
| `texte-site-internet.clean.md` | ODT → markdown (main copy) |
| `texte site internet.docx` | LibreOffice convert of the ODT |
| `photos/*.jpg.md` | Phase-0 image sidecars (no LLM); stub `.jpg` for path tests |
| `modele-site-internet.pdf` | Optional local Canva mockup (large; not required for `eval cerise`) |

```bash
./machin-rag kb create -c cerise-landing
./machin-rag ingest -c cerise-landing -f testdata/cerise-landing/kb-cards.md
./machin-rag ingest -c cerise-landing -f testdata/cerise-landing/photos/
./machin-rag query -c cerise-landing -q "photo retraite haseya" -k 3

# Acceptance gate (TDD, machin-native)
./machin-rag eval cerise
```
