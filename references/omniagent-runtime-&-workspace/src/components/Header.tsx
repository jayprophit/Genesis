import React from 'react';
import { 
  Cpu, 
  ShieldCheck, 
  ShieldAlert, 
  Shield, 
  Laptop, 
  Cloud, 
  Terminal, 
  Layers, 
  PhoneCall, 
  Video,
  Sliders, 
  Sun, 
  Moon, 
  Sparkles, 
  Smartphone, 
  Tablet, 
  Monitor, 
  ChevronDown,
  RefreshCw,
  Zap,
  Globe
} from 'lucide-react';
import { RuntimeMode, TrustBoundary, ThemeMode, DeviceView, ModelAdapterInfo, DesktopBridgeState } from '../types';

interface HeaderProps {
  mode: RuntimeMode;
  onSelectMode: (mode: RuntimeMode) => void;
  activeModel: ModelAdapterInfo;
  models: ModelAdapterInfo[];
  onSelectModel: (modelId: string) => void;
  trustBoundary: TrustBoundary;
  onSelectBoundary: (boundary: TrustBoundary) => void;
  desktopBridge: DesktopBridgeState;
  onToggleBridge: () => void;
  theme: ThemeMode;
  onSelectTheme: (theme: ThemeMode) => void;
  deviceView: DeviceView;
  onSelectDeviceView: (device: DeviceView) => void;
  onOpenVoiceCall: () => void;
  onOpenVideoCall?: () => void;
  onOpenSetup: () => void;
}

export const Header: React.FC<HeaderProps> = ({
  mode,
  onSelectMode,
  activeModel,
  models,
  onSelectModel,
  trustBoundary,
  onSelectBoundary,
  desktopBridge,
  onToggleBridge,
  theme,
  onSelectTheme,
  deviceView,
  onSelectDeviceView,
  onOpenVoiceCall,
  onOpenVideoCall,
  onOpenSetup
}) => {
  const [modelDropdownOpen, setModelDropdownOpen] = React.useState(false);
  const [boundaryDropdownOpen, setBoundaryDropdownOpen] = React.useState(false);
  const [modeDropdownOpen, setModeDropdownOpen] = React.useState(false);

  const modeDescriptions: Record<RuntimeMode, { label: string; badge: string; icon: any }> = {
    'managed-cloud': { label: 'Managed Cloud Assistant', badge: 'Fastest Start • Cloud VM', icon: Cloud },
    'oss-runtime': { label: 'Hermes Open-Source Runtime', badge: 'DIY • Model Freedom • VPS/Docker', icon: Terminal },
    'cowork-workspace': { label: 'Claude Cowork Workspace', badge: 'Sub-agents • Desktop Bridge', icon: Layers }
  };

  const boundaryConfig: Record<TrustBoundary, { label: string; color: string; icon: any }> = {
    strict: { label: 'Strict Boundary', color: 'text-rose-400 bg-rose-500/10 border-rose-500/30', icon: ShieldAlert },
    guarded: { label: 'Guarded Boundary', color: 'text-amber-400 bg-amber-500/10 border-amber-500/30', icon: ShieldCheck },
    permissive: { label: 'High Autonomy', color: 'text-emerald-400 bg-emerald-500/10 border-emerald-500/30', icon: Shield }
  };

  return (
    <header className="h-16 border-b border-slate-800/80 bg-slate-950/80 backdrop-blur-md px-4 flex items-center justify-between gap-3 sticky top-0 z-30 select-none">
      {/* Brand & Mode Switcher */}
      <div className="flex items-center gap-3">
        <div className="flex items-center gap-2.5">
          <div className="w-9 h-9 rounded-xl bg-gradient-to-tr from-indigo-600 via-violet-500 to-cyan-400 p-[1px] shadow-lg shadow-indigo-500/20">
            <div className="w-full h-full bg-slate-950 rounded-[11px] flex items-center justify-center">
              <Cpu className="w-5 h-5 text-indigo-400" />
            </div>
          </div>
          <div>
            <div className="flex items-center gap-2">
              <span className="font-extrabold tracking-tight text-sm text-slate-100 uppercase">OMNI<span className="text-indigo-400">AGENT</span></span>
              <span className="text-[10px] px-1.5 py-0.5 rounded font-mono bg-indigo-500/20 text-indigo-300 border border-indigo-500/30 font-semibold">
                RUNTIME v3.4
              </span>
            </div>
            <p className="text-[11px] text-slate-400 font-medium hidden sm:block">Unified Agent Runtime & Workspace</p>
          </div>
        </div>

        <div className="h-6 w-[1px] bg-slate-800 mx-1 hidden lg:block" />

        {/* Runtime Mode Selector */}
        <div className="relative hidden md:block">
          <button
            onClick={() => setModeDropdownOpen(!modeDropdownOpen)}
            className="flex items-center gap-2 px-2.5 py-1.5 rounded-lg bg-slate-900 border border-slate-800 hover:border-slate-700 text-xs text-slate-200 transition-colors"
          >
            {React.createElement(modeDescriptions[mode].icon, { className: 'w-3.5 h-3.5 text-indigo-400' })}
            <span className="font-medium">{modeDescriptions[mode].label}</span>
            <span className="text-[10px] text-slate-400 hidden xl:inline">({modeDescriptions[mode].badge})</span>
            <ChevronDown className="w-3 h-3 text-slate-400" />
          </button>

          {modeDropdownOpen && (
            <div className="absolute top-full left-0 mt-1.5 w-72 bg-slate-900 border border-slate-800 rounded-xl shadow-2xl p-1.5 z-50">
              <div className="px-2 py-1 text-[10px] font-semibold text-slate-400 uppercase tracking-wider">
                Select Agent Architecture
              </div>
              {(['managed-cloud', 'oss-runtime', 'cowork-workspace'] as RuntimeMode[]).map((m) => {
                const item = modeDescriptions[m];
                const isSelected = mode === m;
                return (
                  <button
                    key={m}
                    onClick={() => {
                      onSelectMode(m);
                      setModeDropdownOpen(false);
                    }}
                    className={`w-full text-left px-2.5 py-2 rounded-lg text-xs flex items-start gap-2.5 transition-colors ${
                      isSelected ? 'bg-indigo-600/20 text-indigo-200 border border-indigo-500/30' : 'text-slate-300 hover:bg-slate-800/60'
                    }`}
                  >
                    {React.createElement(item.icon, { className: `w-4 h-4 mt-0.5 ${isSelected ? 'text-indigo-400' : 'text-slate-400'}` })}
                    <div>
                      <div className="font-medium text-slate-200">{item.label}</div>
                      <div className="text-[10px] text-slate-400 mt-0.5">{item.badge}</div>
                    </div>
                  </button>
                );
              })}
            </div>
          )}
        </div>
      </div>

      {/* Center Controls: Model Adapter Engine & Trust Boundary */}
      <div className="flex items-center gap-2">
        {/* Model Adapter Switcher (Swap the engine without code changes) */}
        <div className="relative">
          <button
            onClick={() => setModelDropdownOpen(!modelDropdownOpen)}
            className="flex items-center gap-2 px-3 py-1.5 rounded-lg bg-indigo-950/40 border border-indigo-800/50 hover:border-indigo-600/70 text-xs text-indigo-200 transition-all shadow-sm"
          >
            <Zap className="w-3.5 h-3.5 text-amber-400 animate-pulse" />
            <span className="font-semibold">{activeModel.name}</span>
            <span className="text-[10px] px-1.5 py-0.2 rounded bg-indigo-500/20 text-indigo-300 border border-indigo-500/30 font-mono">
              {activeModel.latencyMs}ms
            </span>
            <ChevronDown className="w-3 h-3 text-indigo-300/70" />
          </button>

          {modelDropdownOpen && (
            <div className="absolute top-full left-1/2 -translate-x-1/2 mt-1.5 w-80 bg-slate-900 border border-slate-800 rounded-xl shadow-2xl p-2 z-50">
              <div className="flex items-center justify-between px-2 py-1 border-b border-slate-800 pb-1.5 mb-1">
                <span className="text-[11px] font-semibold text-slate-300">Model Adapter (Engine Swap)</span>
                <span className="text-[10px] text-emerald-400 font-mono">0-code downtime</span>
              </div>
              <div className="space-y-1 max-h-72 overflow-y-auto pr-1">
                {models.map((m) => {
                  const isCurrent = m.id === activeModel.id;
                  return (
                    <button
                      key={m.id}
                      onClick={() => {
                        onSelectModel(m.id);
                        setModelDropdownOpen(false);
                      }}
                      className={`w-full text-left p-2 rounded-lg text-xs flex items-center justify-between transition-all ${
                        isCurrent 
                          ? 'bg-indigo-600/25 border border-indigo-500/40 text-indigo-100' 
                          : 'hover:bg-slate-800/70 text-slate-300'
                      }`}
                    >
                      <div>
                        <div className="flex items-center gap-1.5 font-medium">
                          <span>{m.name}</span>
                          {m.isNativeLive && (
                            <span className="text-[9px] px-1 rounded bg-emerald-500/20 text-emerald-300 font-mono">LIVE API</span>
                          )}
                        </div>
                        <div className="text-[10px] text-slate-400">{m.provider} • {m.contextWindow}</div>
                      </div>
                      <div className="text-right font-mono text-[10px]">
                        <div className="text-slate-200">{m.tokensPerSec} t/s</div>
                        <div className="text-indigo-400">{m.latencyMs}ms</div>
                      </div>
                    </button>
                  );
                })}
              </div>
            </div>
          )}
        </div>

        {/* Trust Boundary Status Pill */}
        <div className="relative hidden sm:block">
          <button
            onClick={() => setBoundaryDropdownOpen(!boundaryDropdownOpen)}
            className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-lg border text-xs font-medium transition-all ${boundaryConfig[trustBoundary].color}`}
          >
            {React.createElement(boundaryConfig[trustBoundary].icon, { className: 'w-3.5 h-3.5' })}
            <span>{boundaryConfig[trustBoundary].label}</span>
            <ChevronDown className="w-3 h-3 opacity-70" />
          </button>

          {boundaryDropdownOpen && (
            <div className="absolute top-full right-0 mt-1.5 w-64 bg-slate-900 border border-slate-800 rounded-xl shadow-2xl p-1.5 z-50">
              <div className="px-2 py-1 text-[10px] font-semibold text-slate-400 uppercase tracking-wider">
                Autonomous Security Boundaries
              </div>
              {(['strict', 'guarded', 'permissive'] as TrustBoundary[]).map((b) => {
                const conf = boundaryConfig[b];
                const active = trustBoundary === b;
                return (
                  <button
                    key={b}
                    onClick={() => {
                      onSelectBoundary(b);
                      setBoundaryDropdownOpen(false);
                    }}
                    className={`w-full text-left px-2.5 py-2 rounded-lg text-xs flex items-center gap-2 transition-colors ${
                      active ? 'bg-slate-800 text-slate-100 font-semibold' : 'text-slate-300 hover:bg-slate-800/60'
                    }`}
                  >
                    {React.createElement(conf.icon, { className: 'w-4 h-4' })}
                    <div>
                      <div className="capitalize">{b} Mode</div>
                      <div className="text-[10px] text-slate-400 font-normal">
                        {b === 'strict' && 'Confirms all file writes & calls'}
                        {b === 'guarded' && 'Auto-runs approved; gates bash/sockets'}
                        {b === 'permissive' && 'Autonomous parallel subagent flows'}
                      </div>
                    </div>
                  </button>
                );
              })}
            </div>
          )}
        </div>
      </div>

      {/* Right Controls: Desktop Bridge, Viewport, Theme, Voice, Setup */}
      <div className="flex items-center gap-2">
        {/* Desktop Bridge Quick Status */}
        <button
          onClick={onToggleBridge}
          title="Toggle Desktop Bridge connection to approved local folders"
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-lg border text-xs transition-all ${
            desktopBridge.connected
              ? 'bg-emerald-500/10 border-emerald-500/30 text-emerald-300 hover:bg-emerald-500/20'
              : 'bg-slate-900 border-slate-800 text-slate-400 hover:bg-slate-800'
          }`}
        >
          <Laptop className="w-3.5 h-3.5" />
          <span className="hidden xl:inline text-[11px] font-medium">Desktop Bridge</span>
          <span className={`w-2 h-2 rounded-full ${desktopBridge.connected ? 'bg-emerald-400 animate-ping' : 'bg-slate-600'}`} />
        </button>

        {/* Video Call Launcher (Face to Face AI as in showcase panel 3) */}
        {onOpenVideoCall && (
          <button
            onClick={onOpenVideoCall}
            className="flex items-center gap-1.5 px-2.5 py-1.5 rounded-lg bg-gradient-to-r from-cyan-600 to-indigo-600 hover:from-cyan-500 hover:to-indigo-500 text-white text-xs font-semibold shadow-md shadow-cyan-600/30 transition-all active:scale-95"
            title="Start Face-to-Face Video Call with AI Avatar"
          >
            <Video className="w-3.5 h-3.5" />
            <span className="hidden lg:inline">Video Call</span>
          </button>
        )}

        {/* Voice Call Launcher (as seen in slide 11 & 12) */}
        <button
          onClick={onOpenVoiceCall}
          className="flex items-center gap-1.5 px-2.5 py-1.5 rounded-lg bg-indigo-600/90 hover:bg-indigo-500 text-white text-xs font-semibold shadow-md shadow-indigo-600/30 transition-all active:scale-95"
          title="Start natural voice call with agent avatar"
        >
          <PhoneCall className="w-3.5 h-3.5" />
          <span className="hidden lg:inline">Voice Call</span>
        </button>

        {/* Device Viewport Preview */}
        <div className="hidden lg:flex items-center bg-slate-900 border border-slate-800 rounded-lg p-0.5">
          <button
            onClick={() => onSelectDeviceView('desktop')}
            className={`p-1.5 rounded ${deviceView === 'desktop' ? 'bg-slate-800 text-indigo-400' : 'text-slate-400 hover:text-slate-200'}`}
            title="Desktop View"
          >
            <Monitor className="w-3.5 h-3.5" />
          </button>
          <button
            onClick={() => onSelectDeviceView('tablet')}
            className={`p-1.5 rounded ${deviceView === 'tablet' ? 'bg-slate-800 text-indigo-400' : 'text-slate-400 hover:text-slate-200'}`}
            title="Tablet / iPad View"
          >
            <Tablet className="w-3.5 h-3.5" />
          </button>
          <button
            onClick={() => onSelectDeviceView('mobile')}
            className={`p-1.5 rounded ${deviceView === 'mobile' ? 'bg-slate-800 text-indigo-400' : 'text-slate-400 hover:text-slate-200'}`}
            title="Mobile View"
          >
            <Smartphone className="w-3.5 h-3.5" />
          </button>
        </div>

        {/* Theme Switcher (from Slide 12: Frost, Aurora, Refined Amber, Dark) */}
        <div className="flex items-center bg-slate-900 border border-slate-800 rounded-lg p-0.5">
          <button
            onClick={() => onSelectTheme('dark')}
            className={`px-2 py-1 text-[11px] rounded font-medium transition-colors ${
              theme === 'dark' ? 'bg-slate-800 text-slate-100' : 'text-slate-400 hover:text-slate-200'
            }`}
            title="Obsidian Dark"
          >
            Dark
          </button>
          <button
            onClick={() => onSelectTheme('frost')}
            className={`px-2 py-1 text-[11px] rounded font-medium transition-colors ${
              theme === 'frost' ? 'bg-sky-500/20 text-sky-300 font-semibold' : 'text-slate-400 hover:text-slate-200'
            }`}
            title="Frost Blue Clean"
          >
            Frost
          </button>
          <button
            onClick={() => onSelectTheme('aurora')}
            className={`px-2 py-1 text-[11px] rounded font-medium transition-colors ${
              theme === 'aurora' ? 'bg-purple-500/20 text-purple-300 font-semibold' : 'text-slate-400 hover:text-slate-200'
            }`}
            title="Aurora Vibrant"
          >
            Aurora
          </button>
          <button
            onClick={() => onSelectTheme('amber')}
            className={`px-2 py-1 text-[11px] rounded font-medium transition-colors ${
              theme === 'amber' ? 'bg-amber-500/20 text-amber-300 font-semibold' : 'text-slate-400 hover:text-slate-200'
            }`}
            title="Refined Amber"
          >
            Amber
          </button>
          <button
            onClick={() => onSelectTheme('forest')}
            className={`px-2 py-1 text-[11px] rounded font-medium transition-colors ${
              theme === 'forest' ? 'bg-emerald-500/20 text-emerald-300 font-semibold' : 'text-slate-400 hover:text-slate-200'
            }`}
            title="Forest Emerald Green"
          >
            Forest
          </button>
        </div>

        {/* Setup & Config Modal Button */}
        <button
          onClick={onOpenSetup}
          className="p-2 rounded-lg bg-slate-900 border border-slate-800 hover:border-slate-700 text-slate-300 hover:text-slate-100 transition-colors"
          title="Auto-setup, Environment Variables, Docker/VPS Deploy & API Keys"
        >
          <Sliders className="w-4 h-4" />
        </button>
      </div>
    </header>
  );
};
