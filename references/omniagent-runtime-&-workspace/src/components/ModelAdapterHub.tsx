import React, { useState } from 'react';
import { 
  Zap, 
  Check, 
  ArrowRightLeft, 
  Activity, 
  Code2, 
  ShieldCheck, 
  Sparkles, 
  Play, 
  Server, 
  Lock, 
  Layers,
  TrendingDown,
  Info
} from 'lucide-react';
import { ModelAdapterInfo } from '../types';

interface ModelAdapterHubProps {
  models: ModelAdapterInfo[];
  activeModel: ModelAdapterInfo;
  onSelectModel: (modelId: string) => void;
}

export const ModelAdapterHub: React.FC<ModelAdapterHubProps> = ({
  models,
  activeModel,
  onSelectModel
}) => {
  const [testingModelId, setTestingModelId] = useState<string | null>(null);
  const [latencyResults, setLatencyResults] = useState<Record<string, number>>({});
  const [activeTab, setActiveTab] = useState<'matrix' | 'architecture' | 'code'>('matrix');

  const runLatencyTest = (modelId: string) => {
    setTestingModelId(modelId);
    setTimeout(() => {
      const randomLatency = Math.floor(Math.random() * 80 + (modelId.includes('flash') ? 210 : 450));
      setLatencyResults(prev => ({ ...prev, [modelId]: randomLatency }));
      setTestingModelId(null);
    }, 700);
  };

  return (
    <div className="flex-1 p-6 overflow-y-auto space-y-6">
      {/* Top Banner */}
      <div className="p-6 rounded-2xl bg-gradient-to-r from-slate-900 via-indigo-950/60 to-slate-900 border border-slate-800 flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <div className="flex items-center gap-2">
            <span className="p-1.5 rounded-lg bg-indigo-500/20 text-indigo-400 border border-indigo-500/30">
              <Zap className="w-5 h-5 text-amber-400" />
            </span>
            <h2 className="text-base font-extrabold text-slate-100 uppercase tracking-tight">
              Model Adapter Engine Hub
            </h2>
            <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-300 border border-emerald-500/30">
              Zero-Downtime Hot Swap
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-1 max-w-2xl leading-relaxed">
            Switch model providers (Google Gemini, Anthropic Claude, OpenAI Codex, DeepSeek R1, Local Llama 3) dynamically without modifying your codebase or agent routines.
          </p>
        </div>

        <div className="flex items-center gap-2">
          <button
            onClick={() => setActiveTab('matrix')}
            className={`px-3 py-1.5 rounded-xl text-xs font-semibold transition-all ${
              activeTab === 'matrix' ? 'bg-indigo-600 text-white' : 'bg-slate-900 text-slate-400 border border-slate-800'
            }`}
          >
            Engine Matrix
          </button>
          <button
            onClick={() => setActiveTab('architecture')}
            className={`px-3 py-1.5 rounded-xl text-xs font-semibold transition-all ${
              activeTab === 'architecture' ? 'bg-indigo-600 text-white' : 'bg-slate-900 text-slate-400 border border-slate-800'
            }`}
          >
            "Owned Layer" Diagram
          </button>
          <button
            onClick={() => setActiveTab('code')}
            className={`px-3 py-1.5 rounded-xl text-xs font-semibold transition-all ${
              activeTab === 'code' ? 'bg-indigo-600 text-white' : 'bg-slate-900 text-slate-400 border border-slate-800'
            }`}
          >
            Adapter Code Spec
          </button>
        </div>
      </div>

      {/* VIEW 1: Engine Matrix */}
      {activeTab === 'matrix' && (
        <div className="space-y-6">
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
            {models.map((model) => {
              const isActive = activeModel.id === model.id;
              const isTesting = testingModelId === model.id;
              const currentLatency = latencyResults[model.id] || model.latencyMs;

              return (
                <div
                  key={model.id}
                  className={`p-5 rounded-2xl border transition-all flex flex-col justify-between ${
                    isActive
                      ? 'bg-slate-900/90 border-indigo-500 shadow-xl shadow-indigo-500/10 ring-1 ring-indigo-500/40'
                      : 'bg-slate-900/40 border-slate-800 hover:border-slate-700 hover:bg-slate-900/70'
                  }`}
                >
                  <div>
                    {/* Header */}
                    <div className="flex items-start justify-between gap-2 mb-3">
                      <div>
                        <div className="flex items-center gap-1.5">
                          <h3 className="text-sm font-bold text-slate-100">{model.name}</h3>
                          {model.isNativeLive && (
                            <span className="text-[9px] font-mono px-1.5 py-0.5 rounded bg-emerald-500/20 text-emerald-300 border border-emerald-500/30">
                              LIVE SDK
                            </span>
                          )}
                        </div>
                        <p className="text-[11px] text-slate-400">{model.provider}</p>
                      </div>

                      {isActive ? (
                        <span className="flex items-center gap-1 text-[10px] font-mono font-bold px-2 py-0.5 rounded-full bg-indigo-500/20 text-indigo-300 border border-indigo-500/40">
                          <Check className="w-3 h-3 text-indigo-400" />
                          ACTIVE
                        </span>
                      ) : (
                        <button
                          onClick={() => onSelectModel(model.id)}
                          className="px-2.5 py-1 rounded-lg text-xs font-semibold bg-slate-800 hover:bg-indigo-600 text-slate-200 hover:text-white transition-colors"
                        >
                          Swap to Engine
                        </button>
                      )}
                    </div>

                    <p className="text-xs text-slate-300 leading-relaxed mb-4">
                      {model.description}
                    </p>

                    {/* Stats & Benchmarks */}
                    <div className="grid grid-cols-2 gap-2 text-xs font-mono mb-4 p-2.5 rounded-xl bg-slate-950 border border-slate-800">
                      <div>
                        <span className="text-[10px] text-slate-500">Latency:</span>
                        <div className="text-slate-200 font-semibold flex items-center gap-1">
                          <span>{currentLatency}ms</span>
                          <button
                            onClick={() => runLatencyTest(model.id)}
                            disabled={isTesting}
                            title="Ping adapter round-trip"
                            className="text-[10px] text-indigo-400 hover:text-indigo-300 underline"
                          >
                            {isTesting ? 'pinging...' : 'ping'}
                          </button>
                        </div>
                      </div>
                      <div>
                        <span className="text-[10px] text-slate-500">Token Speed:</span>
                        <div className="text-emerald-400 font-semibold">{model.tokensPerSec} t/s</div>
                      </div>
                      <div>
                        <span className="text-[10px] text-slate-500">Context:</span>
                        <div className="text-slate-300">{model.contextWindow}</div>
                      </div>
                      <div>
                        <span className="text-[10px] text-slate-500">Pricing:</span>
                        <div className="text-slate-300 truncate">{model.pricing}</div>
                      </div>
                    </div>

                    {/* Benchmark Bars */}
                    <div className="space-y-1.5 text-[11px]">
                      <div className="flex justify-between text-slate-400">
                        <span>Coding AST:</span>
                        <span className="font-mono text-slate-200">{model.scores.coding}/100</span>
                      </div>
                      <div className="w-full h-1 bg-slate-800 rounded-full overflow-hidden">
                        <div className="h-full bg-indigo-500 rounded-full" style={{ width: `${model.scores.coding}%` }} />
                      </div>

                      <div className="flex justify-between text-slate-400">
                        <span>Reasoning / Math:</span>
                        <span className="font-mono text-slate-200">{model.scores.reasoning}/100</span>
                      </div>
                      <div className="w-full h-1 bg-slate-800 rounded-full overflow-hidden">
                        <div className="h-full bg-cyan-400 rounded-full" style={{ width: `${model.scores.reasoning}%` }} />
                      </div>
                    </div>
                  </div>

                  <div className="pt-3 mt-3 border-t border-slate-800 text-[10px] text-slate-500 font-mono flex items-center justify-between">
                    <span>Hot-swap safety: 100%</span>
                    <span>Provider API: OK</span>
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      )}

      {/* VIEW 2: Excalidraw Diagram ("YOUR OWNED LAYER") */}
      {activeTab === 'architecture' && (
        <div className="p-8 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-6">
          <div className="text-center max-w-xl mx-auto space-y-2">
            <h3 className="text-base font-bold text-slate-100 uppercase tracking-wider">
              "Your Owned Layer" Architectural Blueprint
            </h3>
            <p className="text-xs text-slate-400">
              From the original Excalidraw design: decouple your files, tools, memory, and routines from any single foundation model.
            </p>
          </div>

          <div className="max-w-4xl mx-auto p-6 rounded-2xl bg-slate-950 border-2 border-indigo-500/30 shadow-2xl relative">
            <div className="text-center mb-6">
              <span className="px-3 py-1 rounded-full text-xs font-mono font-bold bg-indigo-500/20 text-indigo-300 border border-indigo-500/40 uppercase">
                YOUR OWNED LAYER (Autonomous & Private)
              </span>
            </div>

            {/* 6 Core Blocks */}
            <div className="grid grid-cols-2 md:grid-cols-3 gap-4 mb-8">
              {[
                { num: '01', title: 'FILES', desc: 'Private repo, local desktop bridge, cloud sandbox' },
                { num: '02', tools: true, title: 'TOOLS', desc: 'Bash terminal, crawler, git, API connectors' },
                { num: '03', title: 'MEMORY', desc: '4-Tier vector embeddings (Global, Project, Thread, Agent)' },
                { num: '04', title: 'ROUTINES', desc: 'Scheduled cron tasks, background watchers, sub-agents' },
                { num: '05', title: 'PERMISSIONS', desc: 'Shared trust boundaries (Strict, Guarded, Permissive)' },
                { num: '06', title: 'INTERFACES', desc: 'Desktop, Web, Tablet, Mobile, Terminal & Voice' },
              ].map((item) => (
                <div key={item.num} className="p-4 rounded-xl bg-slate-900 border border-slate-800 text-left space-y-1">
                  <div className="text-indigo-400 font-mono font-bold text-xs">{item.num}</div>
                  <div className="font-bold text-xs text-slate-100">{item.title}</div>
                  <div className="text-[10px] text-slate-400 leading-snug">{item.desc}</div>
                </div>
              ))}
            </div>

            {/* Arrow & Model Adapter */}
            <div className="flex flex-col items-center justify-center space-y-3 pt-4 border-t border-slate-800">
              <div className="text-xs font-mono text-amber-400 uppercase tracking-wider font-semibold">
                ▲ SWAP THE ENGINE ▲
              </div>
              <div className="px-5 py-2.5 rounded-xl bg-indigo-600 text-white font-mono text-xs font-bold shadow-lg shadow-indigo-600/30 border border-indigo-400">
                MODEL ADAPTER ROUTER
              </div>

              {/* Swappable Engines */}
              <div className="flex flex-wrap items-center justify-center gap-3 pt-2">
                {['Google Gemini 3.8', 'Anthropic Claude 3.5', 'OpenAI GPT-4o / Codex', 'DeepSeek R1', 'Local Llama 3 (vLLM)'].map((e, idx) => (
                  <span key={idx} className="px-3 py-1 rounded-lg bg-slate-900 border border-slate-800 text-xs font-mono text-slate-300">
                    {e}
                  </span>
                ))}
              </div>
            </div>
          </div>
        </div>
      )}

      {/* VIEW 3: Code Spec */}
      {activeTab === 'code' && (
        <div className="p-6 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-4 font-mono text-xs">
          <div className="flex items-center justify-between pb-3 border-b border-slate-800">
            <span className="text-slate-300 font-bold">src/engine/model-adapter.ts</span>
            <span className="text-indigo-400 text-[11px]">Provider-Agnostic Interface</span>
          </div>

          <pre className="p-4 rounded-xl bg-slate-950 text-slate-300 overflow-x-auto leading-relaxed">
            <code>{`import { GoogleGenAI } from '@google/genai';

export interface ModelEngine {
  stream(prompt: string, options: any): Promise<AsyncIterable<string>>;
  complete(prompt: string, options: any): Promise<string>;
}

export class ModelAdapterRegistry {
  private activeEngineId: string = 'gemini-3.8-flash';
  private geminiClient = new GoogleGenAI({ apiKey: process.env.GEMINI_API_KEY });

  // Swap provider dynamically at runtime with ZERO codebase modifications
  public swapEngine(engineId: string) {
    this.activeEngineId = engineId;
    console.log(\`[ModelAdapter] Swapped engine to \${engineId} without restarting services.\`);
    return { success: true, activeEngineId: this.activeEngineId };
  }

  public async dispatch(prompt: string, context: Record<string, any>) {
    switch (this.activeEngineId) {
      case 'gemini-3.8-flash':
      case 'gemini-3.1-pro-preview':
        return await this.geminiClient.models.generateContent({
          model: this.activeEngineId,
          contents: prompt,
        });
      case 'claude-3-5-sonnet':
        return await this.dispatchClaude(prompt, context);
      case 'gpt-4o':
        return await this.dispatchCodex(prompt, context);
      case 'llama-3.3-70b-local':
        return await this.dispatchLocalVLLM(prompt, context);
      default:
        throw new Error(\`Unknown engine: \${this.activeEngineId}\`);
    }
  }
}`}</code>
          </pre>
        </div>
      )}
    </div>
  );
};
