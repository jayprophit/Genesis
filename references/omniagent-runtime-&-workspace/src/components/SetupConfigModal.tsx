import React, { useState } from 'react';
import { 
  Sliders, 
  Key, 
  Server, 
  Terminal, 
  ShieldCheck, 
  Copy, 
  Check, 
  Download, 
  Cloud, 
  Lock,
  ExternalLink,
  Save
} from 'lucide-react';
import { TrustBoundary } from '../types';

interface SetupConfigModalProps {
  isOpen: boolean;
  onClose: () => void;
  trustBoundary: TrustBoundary;
  onUpdateBoundary: (b: TrustBoundary) => void;
}

export const SetupConfigModal: React.FC<SetupConfigModalProps> = ({
  isOpen,
  onClose,
  trustBoundary,
  onUpdateBoundary
}) => {
  const [activeTab, setActiveTab] = useState<'env' | 'docker' | 'serverless' | 'privacy'>('env');
  const [copiedKey, setCopiedKey] = useState<string | null>(null);

  // Editable configuration fields
  const [geminiKeyMasked, setGeminiKeyMasked] = useState('AIzaSyD-••••••••••••••••••••');
  const [bridgeToken, setBridgeToken] = useState('omni_sec_89f01ab982e74d4401c9');
  const [serverPort, setServerPort] = useState('3000');
  const [sandboxMemory, setSandboxMemory] = useState('4096MB');
  const [savedNotice, setSavedNotice] = useState(false);

  if (!isOpen) return null;

  const copyToClipboard = (text: string, key: string) => {
    navigator.clipboard.writeText(text);
    setCopiedKey(key);
    setTimeout(() => setCopiedKey(null), 2000);
  };

  const handleSave = () => {
    setSavedNotice(true);
    setTimeout(() => {
      setSavedNotice(false);
      onClose();
    }, 800);
  };

  const dockerComposeYaml = `version: '3.8'

services:
  omniagent-runtime:
    image: ghcr.io/omniagent/runtime:3.4.0
    restart: always
    ports:
      - "3000:3000"
    environment:
      - NODE_ENV=production
      - PORT=3000
      - GEMINI_API_KEY=\${GEMINI_API_KEY}
      - TRUST_BOUNDARY=${trustBoundary}
      - BRIDGE_RPC_PORT=9876
    volumes:
      - ./data/memory:/app/data/memory
      - ./data/skills:/app/data/skills
      - ./data/audit:/app/data/audit
    security_opt:
      - no-new-privileges:true
    deploy:
      resources:
        limits:
          memory: 4096M
          cpus: '2.0'`;

  const vpsInstallScript = `# Fast-start auto installer for Ubuntu / Debian / Fedora VPS
curl -sSL https://get.omniagent.run | bash -s -- \\
  --mode=oss-runtime \\
  --trust=${trustBoundary} \\
  --port=3000`;

  return (
    <div className="fixed inset-0 bg-slate-950/80 backdrop-blur-sm z-50 flex items-center justify-center p-4">
      <div className="bg-slate-900 border border-slate-800 rounded-3xl w-full max-w-2xl max-h-[90vh] flex flex-col shadow-2xl overflow-hidden">
        {/* Header */}
        <div className="p-5 border-b border-slate-800 flex items-center justify-between shrink-0 bg-slate-950/50">
          <div className="flex items-center gap-2.5">
            <div className="p-2 rounded-xl bg-indigo-500/20 text-indigo-400 border border-indigo-500/30">
              <Sliders className="w-5 h-5" />
            </div>
            <div>
              <h2 className="text-sm font-bold text-slate-100">Setup & Deployment Configuration</h2>
              <p className="text-[11px] text-slate-400">Manage API keys, VPS Docker containers & security boundaries</p>
            </div>
          </div>

          <button
            onClick={onClose}
            className="text-slate-400 hover:text-slate-200 text-xs px-2 py-1 rounded-lg hover:bg-slate-800"
          >
            Esc
          </button>
        </div>

        {/* Tabs Bar */}
        <div className="flex items-center border-b border-slate-800 px-5 bg-slate-950/30 text-xs overflow-x-auto">
          <button
            onClick={() => setActiveTab('env')}
            className={`py-3 px-3 font-semibold border-b-2 transition-colors flex items-center gap-1.5 whitespace-nowrap ${
              activeTab === 'env'
                ? 'border-indigo-500 text-indigo-400'
                : 'border-transparent text-slate-400 hover:text-slate-200'
            }`}
          >
            <Key className="w-3.5 h-3.5" />
            <span>Environment & Secrets</span>
          </button>

          <button
            onClick={() => setActiveTab('docker')}
            className={`py-3 px-3 font-semibold border-b-2 transition-colors flex items-center gap-1.5 whitespace-nowrap ${
              activeTab === 'docker'
                ? 'border-indigo-500 text-indigo-400'
                : 'border-transparent text-slate-400 hover:text-slate-200'
            }`}
          >
            <Server className="w-3.5 h-3.5" />
            <span>VPS & Docker Compose</span>
          </button>

          <button
            onClick={() => setActiveTab('serverless')}
            className={`py-3 px-3 font-semibold border-b-2 transition-colors flex items-center gap-1.5 whitespace-nowrap ${
              activeTab === 'serverless'
                ? 'border-indigo-500 text-indigo-400'
                : 'border-transparent text-slate-400 hover:text-slate-200'
            }`}
          >
            <Cloud className="w-3.5 h-3.5" />
            <span>Cloud Run / Serverless</span>
          </button>

          <button
            onClick={() => setActiveTab('privacy')}
            className={`py-3 px-3 font-semibold border-b-2 transition-colors flex items-center gap-1.5 whitespace-nowrap ${
              activeTab === 'privacy'
                ? 'border-indigo-500 text-indigo-400'
                : 'border-transparent text-slate-400 hover:text-slate-200'
            }`}
          >
            <ShieldCheck className="w-3.5 h-3.5" />
            <span>Trust & Privacy</span>
          </button>
        </div>

        {/* Content Body */}
        <div className="p-6 overflow-y-auto flex-1 space-y-4 text-xs">
          {/* TAB 1: Env & Secrets */}
          {activeTab === 'env' && (
            <div className="space-y-4">
              <div className="p-3 rounded-xl bg-indigo-950/30 border border-indigo-800/40 text-[11px] text-indigo-300 leading-relaxed">
                Environment variables and API keys are injected server-side to prevent client leaks. In AI Studio, secrets are managed securely via the Settings panel.
              </div>

              <div>
                <label className="block text-slate-300 font-semibold mb-1">
                  GEMINI_API_KEY (Server-side model engine)
                </label>
                <div className="flex gap-2">
                  <input
                    type="text"
                    value={geminiKeyMasked}
                    onChange={(e) => setGeminiKeyMasked(e.target.value)}
                    className="flex-1 bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-slate-200 font-mono focus:outline-none focus:border-indigo-500"
                  />
                  <span className="px-2.5 py-2 rounded-xl bg-emerald-500/10 text-emerald-400 border border-emerald-500/20 font-mono text-[10px] flex items-center">
                    Configured
                  </span>
                </div>
              </div>

              <div>
                <label className="block text-slate-300 font-semibold mb-1">
                  DESKTOP_BRIDGE_TOKEN (HMAC cryptographic handshake)
                </label>
                <input
                  type="text"
                  value={bridgeToken}
                  onChange={(e) => setBridgeToken(e.target.value)}
                  className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-slate-200 font-mono focus:outline-none focus:border-indigo-500"
                />
              </div>

              <div className="grid grid-cols-2 gap-3">
                <div>
                  <label className="block text-slate-300 font-semibold mb-1">PORT (Hardcoded Ingress)</label>
                  <input
                    type="text"
                    disabled
                    value={serverPort}
                    className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-slate-400 font-mono cursor-not-allowed"
                  />
                </div>
                <div>
                  <label className="block text-slate-300 font-semibold mb-1">CGroup Memory Cap</label>
                  <input
                    type="text"
                    value={sandboxMemory}
                    onChange={(e) => setSandboxMemory(e.target.value)}
                    className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-slate-200 font-mono focus:outline-none focus:border-indigo-500"
                  />
                </div>
              </div>
            </div>
          )}

          {/* TAB 2: VPS & Docker */}
          {activeTab === 'docker' && (
            <div className="space-y-4">
              <div className="space-y-1.5">
                <div className="flex items-center justify-between text-slate-300 font-semibold">
                  <span>1-Line VPS Auto-Installer:</span>
                  <button
                    onClick={() => copyToClipboard(vpsInstallScript, 'vps')}
                    className="text-indigo-400 hover:text-indigo-300 flex items-center gap-1 text-[11px]"
                  >
                    {copiedKey === 'vps' ? <Check className="w-3 h-3 text-emerald-400" /> : <Copy className="w-3 h-3" />}
                    <span>{copiedKey === 'vps' ? 'Copied' : 'Copy'}</span>
                  </button>
                </div>
                <div className="p-3 rounded-xl bg-slate-950 border border-slate-800 font-mono text-[11px] text-slate-300 overflow-x-auto">
                  <pre>{vpsInstallScript}</pre>
                </div>
              </div>

              <div className="space-y-1.5">
                <div className="flex items-center justify-between text-slate-300 font-semibold">
                  <span>docker-compose.yml (Self-Hosted):</span>
                  <button
                    onClick={() => copyToClipboard(dockerComposeYaml, 'compose')}
                    className="text-indigo-400 hover:text-indigo-300 flex items-center gap-1 text-[11px]"
                  >
                    {copiedKey === 'compose' ? <Check className="w-3 h-3 text-emerald-400" /> : <Copy className="w-3 h-3" />}
                    <span>{copiedKey === 'compose' ? 'Copied' : 'Copy'}</span>
                  </button>
                </div>
                <div className="p-3 rounded-xl bg-slate-950 border border-slate-800 font-mono text-[11px] text-slate-300 max-h-48 overflow-y-auto">
                  <pre>{dockerComposeYaml}</pre>
                </div>
              </div>
            </div>
          )}

          {/* TAB 3: Serverless / Cloud Run */}
          {activeTab === 'serverless' && (
            <div className="space-y-3">
              <div className="p-4 rounded-xl bg-slate-950 border border-slate-800 space-y-2">
                <h4 className="font-bold text-slate-200">Google Cloud Run & Serverless Deployment</h4>
                <p className="text-slate-400 leading-relaxed text-[11px]">
                  Deploy with zero operational overhead. The OmniAgent runtime packages static Vite frontend + Express RPC proxy into a unified container image with port 3000 ingress.
                </p>
                <div className="p-2.5 rounded-lg bg-slate-900 border border-slate-800 font-mono text-[10px] text-indigo-300">
                  gcloud run deploy omniagent-runtime --source . --port 3000 --allow-unauthenticated
                </div>
              </div>
            </div>
          )}

          {/* TAB 4: Trust & Privacy */}
          {activeTab === 'privacy' && (
            <div className="space-y-4">
              <div className="space-y-2">
                <label className="block text-slate-300 font-semibold">Shared Trust Boundary Policy</label>
                {(['strict', 'guarded', 'permissive'] as TrustBoundary[]).map((b) => (
                  <label
                    key={b}
                    onClick={() => onUpdateBoundary(b)}
                    className={`p-3 rounded-xl border flex items-start gap-3 cursor-pointer transition-all ${
                      trustBoundary === b
                        ? 'bg-indigo-600/20 border-indigo-500 text-slate-100'
                        : 'bg-slate-950 border-slate-800 text-slate-400 hover:border-slate-700'
                    }`}
                  >
                    <input
                      type="radio"
                      name="boundary"
                      checked={trustBoundary === b}
                      onChange={() => onUpdateBoundary(b)}
                      className="mt-0.5"
                    />
                    <div>
                      <div className="font-bold capitalize text-slate-200">{b} Mode</div>
                      <div className="text-[11px] text-slate-400">
                        {b === 'strict' && 'Demands explicit user confirmation on every tool call, bash command, and file write.'}
                        {b === 'guarded' && 'Auto-approves harmless reads/scans; gates socket writes, external curl calls, and git pushes.'}
                        {b === 'permissive' && 'Autonomous swarm execution for fast multi-agent pipelines and automated browser crawlers.'}
                      </div>
                    </div>
                  </label>
                ))}
              </div>
            </div>
          )}
        </div>

        {/* Footer */}
        <div className="p-4 border-t border-slate-800 bg-slate-950/70 flex items-center justify-between">
          <span className="text-[11px] text-slate-500 font-mono">
            {savedNotice ? '✓ Configuration saved to runtime state' : 'Runtime daemon listening on port 3000'}
          </span>

          <div className="flex items-center gap-2">
            <button
              onClick={onClose}
              className="px-3 py-1.5 rounded-xl bg-slate-800 text-slate-300 font-medium hover:bg-slate-700 text-xs"
            >
              Cancel
            </button>
            <button
              onClick={handleSave}
              className="px-4 py-1.5 rounded-xl bg-indigo-600 hover:bg-indigo-500 text-white font-semibold text-xs shadow flex items-center gap-1.5"
            >
              <Save className="w-3.5 h-3.5" />
              <span>Apply Configuration</span>
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};
