# Product Changelog - July 2026

        <div class="feature-card rounded-xl p-6">
          <div class="flex items-start gap-4">
            <div class="w-12 h-12 rounded-lg bg-emerald-500/10 flex items-center justify-center flex-shrink-0">
              <span class="text-2xl">🧠</span>
            </div>
            <div>
              <h3 class="text-xl font-semibold text-white mb-2">Agent-first local RAG</h3>
              <p class="text-white/40 leading-relaxed">Ship a single machin binary that ingest and query text against Qdrant — JSON on stdout, semantic exit codes, <code>help-json</code> discovery. Built for agents, not admin panels.</p>
            </div>
          </div>
        </div>

        <div class="feature-card rounded-xl p-6">
          <div class="flex items-start gap-4">
            <div class="w-12 h-12 rounded-lg bg-sky-500/10 flex items-center justify-center flex-shrink-0">
              <span class="text-2xl">💸</span>
            </div>
            <div>
              <h3 class="text-xl font-semibold text-white mb-2">Zero BYOK embeddings</h3>
              <p class="text-white/40 leading-relaxed">A tiny local hash embedder (<code>hash-sha256-v1</code>, 64-d cosine) means no OpenAI keys and no model downloads. Keyword-ish quality, honest for MVP agent memory and docs.</p>
            </div>
          </div>
        </div>

        <div class="feature-card rounded-xl p-6">
          <div class="flex items-start gap-4">
            <div class="w-12 h-12 rounded-lg bg-violet-500/10 flex items-center justify-center flex-shrink-0">
              <span class="text-2xl">📦</span>
            </div>
            <div>
              <h3 class="text-xl font-semibold text-white mb-2">Qdrant + Vue peek UI</h3>
              <p class="text-white/40 leading-relaxed">Compose brings local Qdrant; <code>serve</code> exposes <code>/api/*</code> plus an embedded Vue 3 CDN UI for humans who want a quick ingest/search console.</p>
            </div>
          </div>
        </div>

        <div class="feature-card rounded-xl p-6">
          <div class="flex items-start gap-4">
            <div class="w-12 h-12 rounded-lg bg-amber-500/10 flex items-center justify-center flex-shrink-0">
              <span class="text-2xl">📄</span>
            </div>
            <div>
              <h3 class="text-xl font-semibold text-white mb-2">Structured ingest (v0.2)</h3>
              <p class="text-white/40 leading-relaxed">Parse <code>md</code>, <code>csv</code>, <code>json</code>, and <code>ndjson</code> into chunks with a declarative normalize template. Config merges <code>~/.machin-rag/config.json</code> → env → CLI.</p>
            </div>
          </div>
        </div>

        <div class="feature-card rounded-xl p-6">
          <div class="flex items-start gap-4">
            <div class="w-12 h-12 rounded-lg bg-rose-500/10 flex items-center justify-center flex-shrink-0">
              <span class="text-2xl">📎</span>
            </div>
            <div>
              <h3 class="text-xl font-semibold text-white mb-2">DOCX + PDF + OCR policy (v0.5→v0.6)</h3>
              <p class="text-white/40 leading-relaxed">Clear modes for text-only / mixed / images-only. Optional Tesseract OCR (soft <code>dlopen</code>) for image-only docs; CI smoke + hash recall eval scripts.</p>
            </div>
          </div>
        </div>
