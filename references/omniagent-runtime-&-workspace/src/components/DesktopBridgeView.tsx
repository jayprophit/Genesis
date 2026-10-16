import React, { useState } from 'react';
import { 
  Laptop, 
  CheckCircle2, 
  FolderGit2, 
  HardDrive, 
  Lock, 
  ShieldCheck, 
  RefreshCw, 
  Copy, 
  Check, 
  ArrowRightLeft, 
  Plus, 
  Trash2, 
  AlertCircle,
  ExternalLink,
  Terminal
} from 'lucide-react';
import { DesktopBridgeState } from '../types';

interface DesktopBridgeViewProps {
  bridgeState: DesktopBridgeState;
  onToggleBridge: () => void;
  onAddPath: (path: string) => void;
  onRemovePath: (path: string) => void;
}

export const DesktopBridgeView: React.FC<DesktopBridgeViewProps> = ({
  bridgeState,
  onToggleBridge,
  onAddPath,
  onRemovePath
}) => {
  const [newPathInput, setNewPathInput] = useState('');
  const [isSyncing, setIsSyncing] = useState(false);
  const [copiedToken, setCopiedToken] = useState(false);

  const handleCopyCli = () => {
    navigator.clipboard.writeText('npx @omniagent/bridge-daemon --token=omni_sec_89f01ab982e74 --port=9876');
    setCopiedToken(true);
    setTimeout(() => setCopiedToken(false), 2000);
  };

  const handleSyncNow = () => {
    setIsSyncing(true);
    setTimeout(() => {
      setIsSyncing(false);
    }, 800);
  };

  const handleAdd = (e: React.FormEvent) => {
    e.preventDefault();
    if (!newPathInput.trim()) return;
    onAddPath(newPathInput.trim());
    setNewPathInput('');
  };

  return (
    <div className="flex-1 p-6 overflow-y-auto space-y-6">
      {/* Top Banner */}
      <div className="p-6 rounded-2xl bg-gradient-to-r from-slate-900 via-slate-900 to-indigo-950/40 border border-slate-800 flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <div className="flex items-center gap-2">
            <span className={`p-1.5 rounded-lg border ${
              bridgeState.connected
                ? 'bg-emerald-500/20 text-emerald-400 border-emerald-500/30'
                : 'bg-slate-800 text-slate-400 border-slate-700'
            }`}>
              <Laptop className="w-5 h-5" />
            </span>
            <h2 className="text-base font-extrabold text-slate-100 uppercase tracking-tight">
              Desktop Bridge & Local System Integration
            </h2>
            <span className={`text-[10px] font-mono px-2 py-0.5 rounded border ${
              bridgeState.connected
                ? 'bg-emerald-500/10 text-emerald-300 border-emerald-500/30'
                : 'bg-slate-800 text-slate-400 border-slate-700'
            }`}>
              {bridgeState.connected ? 'DAEMON CONNECTED' : 'DISCONNECTED'}
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-1 max-w-2xl leading-relaxed">
            The Desktop Bridge links your cloud agent sandbox to approved directories and local developer tools (VS Code, terminal, PostgreSQL, Docker) on your workstation via encrypted RPC.
          </p>
        </div>

        <div className="flex items-center gap-2.5">
          <button
            onClick={handleSyncNow}
            disabled={isSyncing || !bridgeState.connected}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-semibold border border-slate-700 transition-all disabled:opacity-50"
          >
            <RefreshCw className={`w-3.5 h-3.5 ${isSyncing ? 'animate-spin' : ''}`} />
            <span>{isSyncing ? 'Scanning Tree...' : 'Force Sync'}</span>
          </button>

          <button
            onClick={onToggleBridge}
            className={`px-3 py-1.5 rounded-xl text-xs font-semibold transition-all shadow ${
              bridgeState.connected
                ? 'bg-rose-500/20 text-rose-300 border border-rose-500/30 hover:bg-rose-500/30'
                : 'bg-emerald-600 hover:bg-emerald-500 text-white'
            }`}
          >
            {bridgeState.connected ? 'Disconnect Bridge' : 'Connect Bridge Daemon'}
          </button>
        </div>
      </div>

      {/* Bridge Diagnostics Card */}
      <div className="grid grid-cols-1 md:grid-cols-4 gap-4">
        <div className="p-4 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-1">
          <span className="text-[10px] text-slate-400 font-mono">Workstation Host:</span>
          <div className="text-xs font-bold text-slate-200">{bridgeState.agentHost}</div>
          <div className="text-[10px] text-emerald-400">HMAC Handshake Valid</div>
        </div>

        <div className="p-4 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-1">
          <span className="text-[10px] text-slate-400 font-mono">Bridge Version:</span>
          <div className="text-xs font-bold text-slate-200">{bridgeState.bridgeVersion}</div>
          <div className="text-[10px] text-slate-400">Node v22 / Go runtime</div>
        </div>

        <div className="p-4 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-1">
          <span className="text-[10px] text-slate-400 font-mono">Sync Latency:</span>
          <div className="text-xs font-bold text-indigo-400">8ms (Localhost RPC)</div>
          <div className="text-[10px] text-slate-400">0 dropped packets</div>
        </div>

        <div className="p-4 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-1">
          <span className="text-[10px] text-slate-400 font-mono">Security Model:</span>
          <div className="text-xs font-bold text-emerald-400 flex items-center gap-1">
            <Lock className="w-3 h-3" />
            <span>Opt-In Sandboxed</span>
          </div>
          <div className="text-[10px] text-slate-400">No raw root filesystem access</div>
        </div>
      </div>

      {/* Main Grid: Approved Paths & Daemon CLI */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Left 2 Cols: Approved Local Paths */}
        <div className="lg:col-span-2 p-5 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-2">
              <FolderGit2 className="w-4 h-4 text-indigo-400" />
              <h3 className="text-xs font-bold text-slate-100 uppercase tracking-wider">
                Approved Local Directory Roots
              </h3>
            </div>
            <span className="text-[10px] font-mono text-slate-400">
              {bridgeState.approvedPaths.length} mounted roots
            </span>
          </div>

          <div className="space-y-2">
            {bridgeState.approvedPaths.map((path) => (
              <div
                key={path}
                className="p-3 rounded-xl bg-slate-950 border border-slate-800 text-xs font-mono flex items-center justify-between"
              >
                <div className="flex items-center gap-2.5">
                  <HardDrive className="w-4 h-4 text-indigo-400 shrink-0" />
                  <span className="text-slate-200">{path}</span>
                </div>
                <div className="flex items-center gap-2">
                  <span className="text-[10px] px-2 py-0.5 rounded bg-emerald-500/10 text-emerald-300 border border-emerald-500/20">
                    Read/Write/Git
                  </span>
                  <button
                    onClick={() => onRemovePath(path)}
                    className="p-1 text-slate-400 hover:text-rose-400"
                    title="Revoke access"
                  >
                    <Trash2 className="w-3.5 h-3.5" />
                  </button>
                </div>
              </div>
            ))}
          </div>

          {/* Add Path Form */}
          <form onSubmit={handleAdd} className="flex gap-2 pt-2 border-t border-slate-800">
            <input
              type="text"
              value={newPathInput}
              onChange={(e) => setNewPathInput(e.target.value)}
              placeholder="e.g. /Users/dev/Desktop/my-new-service or C:\Projects\FintechApp"
              className="flex-1 bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-xs text-slate-100 placeholder-slate-500 focus:outline-none focus:border-indigo-500 font-mono"
            />
            <button
              type="submit"
              disabled={!newPathInput.trim()}
              className="flex items-center gap-1.5 px-3 py-2 rounded-xl bg-indigo-600 hover:bg-indigo-500 text-white text-xs font-semibold shadow disabled:opacity-50"
            >
              <Plus className="w-3.5 h-3.5" />
              <span>Approve Path</span>
            </button>
          </form>
        </div>

        {/* Right Col: Local Terminal Daemon Setup */}
        <div className="p-5 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-4">
          <div className="flex items-center justify-between">
            <h3 className="text-xs font-bold text-slate-100 uppercase tracking-wider">
              Workstation CLI Setup
            </h3>
            <Terminal className="w-4 h-4 text-slate-400" />
          </div>

          <p className="text-xs text-slate-300 leading-relaxed">
            Run this single command in your local macOS, Linux, or Windows terminal to mount your chosen folder directly to the cloud agent runtime:
          </p>

          <div className="p-3 rounded-xl bg-slate-950 border border-slate-800 font-mono text-[11px] text-slate-300 space-y-2">
            <div className="flex items-center justify-between text-[10px] text-slate-500 pb-1 border-b border-slate-800">
              <span>Terminal Command</span>
              <button
                onClick={handleCopyCli}
                className="flex items-center gap-1 text-indigo-400 hover:text-indigo-300"
              >
                {copiedToken ? <Check className="w-3 h-3 text-emerald-400" /> : <Copy className="w-3 h-3" />}
                <span>{copiedToken ? 'Copied' : 'Copy'}</span>
              </button>
            </div>
            <p className="break-all text-indigo-300">
              npx @omniagent/bridge-daemon --token=omni_sec_89f01ab982e74 --port=9876
            </p>
          </div>

          <div className="space-y-2 text-xs text-slate-400">
            <div className="flex items-center gap-2">
              <CheckCircle2 className="w-3.5 h-3.5 text-emerald-400 shrink-0" />
              <span>Zero cloud exposure: files stream over TLS tunnel</span>
            </div>
            <div className="flex items-center gap-2">
              <CheckCircle2 className="w-3.5 h-3.5 text-emerald-400 shrink-0" />
              <span>Auto-detects git diffs & uncommitted changes</span>
            </div>
            <div className="flex items-center gap-2">
              <CheckCircle2 className="w-3.5 h-3.5 text-emerald-400 shrink-0" />
              <span>Binds local localhost:3000 / localhost:5432 ports</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
