import React, { useState, useRef, useEffect } from 'react';
import { 
  Code2, 
  Video, 
  Globe, 
  Layers, 
  Maximize2, 
  Minimize2, 
  X, 
  Minus, 
  Copy, 
  Check, 
  Play, 
  Pause, 
  RotateCcw, 
  Volume2, 
  VolumeX, 
  WrapText, 
  Download, 
  ExternalLink, 
  RefreshCw, 
  Lock, 
  Search, 
  ArrowLeft, 
  ArrowRight, 
  Plane, 
  Hotel, 
  Calendar, 
  MapPin, 
  Terminal as TerminalIcon, 
  FileCode, 
  Sparkles,
  Sliders,
  ChevronRight,
  ChevronLeft,
  PanelRightOpen,
  PanelRightClose,
  Tv,
  Share2,
  Camera,
  Compass
} from 'lucide-react';
import { sampleWorkspaceFiles } from '../data/mockInitialData';
import tokyoNightImg from '../assets/images/tokyo_neon_night_1790063676819.jpg';

export type RightSurfaceTab = 'code' | 'videos' | 'browser' | 'others';

interface RightSidebarSurfaceProps {
  initialTab?: RightSurfaceTab;
  activeTab?: RightSurfaceTab;
  onSelectTab?: (tab: RightSurfaceTab) => void;
  isCollapsed?: boolean;
  onToggleCollapse?: () => void;
  onClose?: () => void;
  isMaximized?: boolean;
  onToggleMaximize?: () => void;
  selectedFileIndex?: number;
  onSelectFileIndex?: (idx: number) => void;
  onRunCode?: (fileName: string) => void;
}

export const RightSidebarSurface: React.FC<RightSidebarSurfaceProps> = ({
  initialTab = 'code',
  activeTab: controlledTab,
  onSelectTab,
  isCollapsed = false,
  onToggleCollapse,
  onClose,
  isMaximized = false,
  onToggleMaximize,
  selectedFileIndex = 0,
  onSelectFileIndex,
  onRunCode
}) => {
  const [internalTab, setInternalTab] = useState<RightSurfaceTab>(initialTab);
  const activeTab = controlledTab !== undefined ? controlledTab : internalTab;

  const handleTabChange = (tab: RightSurfaceTab) => {
    setInternalTab(tab);
    if (onSelectTab) onSelectTab(tab);
  };

  const [copiedCode, setCopiedCode] = useState(false);
  const [wordWrapEnabled, setWordWrapEnabled] = useState(true);
  const [fileIdx, setFileIdx] = useState(selectedFileIndex);

  // Video Player State
  const [isVideoPlaying, setIsVideoPlaying] = useState(true);
  const [videoProgress, setVideoProgress] = useState(38);
  const [videoVolume, setVideoVolume] = useState(80);
  const [isVideoMuted, setIsVideoMuted] = useState(false);
  const [selectedVideoId, setSelectedVideoId] = useState<'demo' | 'tokyo' | 'webcam' | 'arch'>('demo');
  const [playbackSpeed, setPlaybackSpeed] = useState<'1.0x' | '1.5x' | '2.0x'>('1.0x');

  // Web Browser State
  const [browserUrl, setBrowserUrl] = useState('https://tokyo-travel-guide.internal/itinerary-2025');
  const [browserInputUrl, setBrowserInputUrl] = useState('https://tokyo-travel-guide.internal/itinerary-2025');
  const [isRefreshing, setIsRefreshing] = useState(false);
  const [browserPage, setBrowserPage] = useState<'tokyo' | 'docs' | 'github' | 'map'>('tokyo');

  // Others Sub-Tab State
  const [othersSubTab, setOthersSubTab] = useState<'travel' | 'terminal' | 'artifacts'>('travel');
  const [selectedFlight, setSelectedFlight] = useState(0);

  // Terminal Logs State
  const [terminalLogs, setTerminalLogs] = useState<string[]>([
    '[INIT] Workspace HyperVM container initialized on port 3000',
    '[MESH] Connecting sub-agents: Coder, Architect, Planner, SecurityAuditor',
    '[BRIDGE] Desktop daemon listening on localhost:9124 (RPC active)',
    '[MODEL] Default active adapter: Gemini 2.5 Flash (latency: 18ms)',
    '[DISPATCH] Tokyo travel plan request scheduled with sub-agent Planner',
    '[OK] Workspace assets rendered in cascading 3-panel mode'
  ]);
  const [terminalInput, setTerminalInput] = useState('');

  const currentFile = sampleWorkspaceFiles[fileIdx] || sampleWorkspaceFiles[0];

  const handleCopyCode = () => {
    navigator.clipboard.writeText(currentFile.content);
    setCopiedCode(true);
    setTimeout(() => setCopiedCode(false), 2000);
  };

  const handleRefreshBrowser = () => {
    setIsRefreshing(true);
    setTimeout(() => setIsRefreshing(false), 600);
  };

  const handleNavigateBrowser = (page: 'tokyo' | 'docs' | 'github' | 'map', url: string) => {
    setBrowserPage(page);
    setBrowserUrl(url);
    setBrowserInputUrl(url);
  };

  const handleAddTerminalCmd = (e: React.FormEvent) => {
    e.preventDefault();
    if (!terminalInput.trim()) return;
    const cmd = terminalInput.trim();
    setTerminalLogs(prev => [
      ...prev,
      `$ ${cmd}`,
      `[EXEC] Processed: ${cmd} (returncode: 0, status: OK)`
    ]);
    setTerminalInput('');
  };

  const flights = [
    { airline: 'ANA (All Nippon Airways)', code: 'NH212', route: 'LHR → HND', duration: '13h 25m', price: '£621', best: true },
    { airline: 'British Airways', code: 'BA007', route: 'LHR → HND', duration: '13h 26m', price: '£683', best: false },
    { airline: 'Japan Airlines', code: 'JL044', route: 'LHR → HND', duration: '13h 25m', price: '£701', best: false }
  ];

  const hotels = [
    { name: 'Hotel Gracery Shinjuku', area: 'Shinjuku', price: '£168/night', rating: '4.8 ★', tag: 'Godzilla view' },
    { name: 'Cerulean Tower Tokyu Hotel', area: 'Shibuya', price: '£245/night', rating: '4.9 ★', tag: 'Skyline Panorama' },
    { name: 'The Gate Hotel Kaminarimon', area: 'Asakusa', price: '£182/night', rating: '4.7 ★', tag: 'Temple proximity' }
  ];

  const videoList = [
    {
      id: 'demo' as const,
      title: 'Autonomous Multi-Agent Demo',
      duration: '04:18',
      desc: 'Planner, Coder, and Security Auditor collaborating in real-time',
      category: 'AI Runtime'
    },
    {
      id: 'tokyo' as const,
      title: 'Tokyo 4K Cinematic Walkthrough',
      duration: '08:45',
      desc: 'Shinjuku neon nights, Meiji Shrine, and Shibuya Sky observatory',
      category: 'Travel Guide'
    },
    {
      id: 'webcam' as const,
      title: 'Live Camera AV Telemetry Feed',
      duration: 'LIVE',
      desc: 'Bi-directional vision stream & face mesh tracking pipeline',
      category: 'Vision AI'
    },
    {
      id: 'arch' as const,
      title: 'Tri-Compute & HyperVM Architecture',
      duration: '06:12',
      desc: 'Explaining local RPC, isolated execution, and model fallback',
      category: 'System Design'
    }
  ];

  if (isCollapsed) {
    return (
      <div className="h-full w-14 sm:w-16 flex flex-col items-center bg-slate-950 border-l border-slate-800/90 shadow-2xl py-3 select-none shrink-0 z-20">
        {/* Expand / Open Toggle */}
        <button
          onClick={onToggleCollapse}
          className="p-2.5 rounded-xl bg-slate-900 hover:bg-slate-800 text-indigo-300 hover:text-white border border-slate-800 hover:border-indigo-500/50 transition-all shadow-md group mb-3"
          title="Expand Right Panel (Cascading Tools)"
        >
          <PanelRightOpen className="w-4 h-4 group-hover:scale-110 transition-transform" />
        </button>

        {/* Vertical divider */}
        <div className="w-8 h-px bg-slate-800/80 mb-3" />

        {/* Vertical Tabs: </> Code, Video, Web, Others */}
        <div className="flex-1 flex flex-col items-center gap-3">
          {/* </> Code */}
          <button
            onClick={() => {
              handleTabChange('code');
              if (onToggleCollapse) onToggleCollapse();
            }}
            className={`flex flex-col items-center gap-1 p-2 rounded-xl transition-all relative group ${
              activeTab === 'code'
                ? 'bg-gradient-to-b from-indigo-600 to-violet-600 text-white shadow-lg shadow-indigo-600/30 ring-1 ring-indigo-400'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
            }`}
            title="</> Code - Syntax viewer & file explorer"
          >
            <Code2 className="w-4 h-4" />
            <span className="text-[9px] font-mono font-bold leading-none">&lt;/&gt;</span>
          </button>

          {/* Video */}
          <button
            onClick={() => {
              handleTabChange('videos');
              if (onToggleCollapse) onToggleCollapse();
            }}
            className={`flex flex-col items-center gap-1 p-2 rounded-xl transition-all relative group ${
              activeTab === 'videos'
                ? 'bg-gradient-to-b from-rose-600 to-pink-600 text-white shadow-lg shadow-rose-600/30 ring-1 ring-rose-400'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
            }`}
            title="Video - AI screencasts, demos & streams"
          >
            <Video className="w-4 h-4" />
            <span className="text-[9px] font-mono leading-none">Video</span>
          </button>

          {/* Web */}
          <button
            onClick={() => {
              handleTabChange('browser');
              if (onToggleCollapse) onToggleCollapse();
            }}
            className={`flex flex-col items-center gap-1 p-2 rounded-xl transition-all relative group ${
              activeTab === 'browser'
                ? 'bg-gradient-to-b from-cyan-600 to-teal-600 text-white shadow-lg shadow-cyan-600/30 ring-1 ring-cyan-400'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
            }`}
            title="Web - In-app browser simulator & docs"
          >
            <Globe className="w-4 h-4" />
            <span className="text-[9px] font-mono leading-none">Web</span>
          </button>

          {/* Others */}
          <button
            onClick={() => {
              handleTabChange('others');
              if (onToggleCollapse) onToggleCollapse();
            }}
            className={`flex flex-col items-center gap-1 p-2 rounded-xl transition-all relative group ${
              activeTab === 'others'
                ? 'bg-gradient-to-b from-amber-600 to-orange-600 text-white shadow-lg shadow-amber-600/30 ring-1 ring-amber-400'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
            }`}
            title="Others - Travel planner, autonomous terminal, artifacts"
          >
            <Layers className="w-4 h-4" />
            <span className="text-[9px] font-mono leading-none">Others</span>
          </button>
        </div>

        {/* Pulse status */}
        <div className="pt-2 flex flex-col items-center gap-1">
          <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
          <span className="text-[8px] font-mono text-slate-500">LIVE</span>
        </div>
      </div>
    );
  }

  return (
    <div className={`h-full flex flex-col bg-slate-950/95 border-l border-slate-800/90 shadow-2xl transition-all duration-200 overflow-hidden ${
      isMaximized ? 'w-full' : 'w-full'
    }`}>
      {/* CASCADING RIGHT PANEL HEADER */}
      <div className="h-12 px-3 sm:px-4 bg-slate-900/90 border-b border-slate-800 flex items-center justify-between shrink-0 select-none backdrop-blur-md">
        {/* Left: Collapse toggle + Tabs */}
        <div className="flex items-center gap-1.5 overflow-x-auto no-scrollbar py-1">
          {onToggleCollapse && (
            <button
              onClick={onToggleCollapse}
              className="p-1.5 rounded-lg hover:bg-slate-800 text-slate-400 hover:text-indigo-300 transition-colors mr-1"
              title="Collapse right panel to rail"
            >
              <PanelRightClose className="w-4 h-4" />
            </button>
          )}

          {/* Navigation Tabs for Right Panel: </> Code, Video, Web, Others */}
          <button
            onClick={() => handleTabChange('code')}
            className={`px-3 py-1.5 rounded-lg text-xs font-semibold flex items-center gap-1.5 transition-all whitespace-nowrap ${
              activeTab === 'code'
                ? 'bg-gradient-to-r from-indigo-600 to-violet-600 text-white shadow-sm shadow-indigo-500/20 ring-1 ring-indigo-400/40'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-800/60'
            }`}
            title="Inspect syntax-highlighted code files with word-wrap"
          >
            <Code2 className="w-3.5 h-3.5 text-indigo-300" />
            <span>&lt;/&gt; Code</span>
          </button>

          <button
            onClick={() => handleTabChange('videos')}
            className={`px-3 py-1.5 rounded-lg text-xs font-semibold flex items-center gap-1.5 transition-all whitespace-nowrap ${
              activeTab === 'videos'
                ? 'bg-gradient-to-r from-rose-600 to-pink-600 text-white shadow-sm shadow-rose-500/20 ring-1 ring-rose-400/40'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-800/60'
            }`}
            title="Play AI videos, demo screencasts, and camera feeds"
          >
            <Video className="w-3.5 h-3.5 text-rose-300" />
            <span>Video</span>
          </button>

          <button
            onClick={() => handleTabChange('browser')}
            className={`px-3 py-1.5 rounded-lg text-xs font-semibold flex items-center gap-1.5 transition-all whitespace-nowrap ${
              activeTab === 'browser'
                ? 'bg-gradient-to-r from-cyan-600 to-teal-600 text-white shadow-sm shadow-cyan-500/20 ring-1 ring-cyan-400/40'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-800/60'
            }`}
            title="Simulated web browser with search, docs, and live web preview"
          >
            <Globe className="w-3.5 h-3.5 text-cyan-300" />
            <span>Web</span>
          </button>

          <button
            onClick={() => handleTabChange('others')}
            className={`px-3 py-1.5 rounded-lg text-xs font-semibold flex items-center gap-1.5 transition-all whitespace-nowrap ${
              activeTab === 'others'
                ? 'bg-gradient-to-r from-amber-600 to-orange-600 text-white shadow-sm shadow-amber-500/20 ring-1 ring-amber-400/40'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-800/60'
            }`}
            title="Travel plans, autonomous terminal, and workspace deliverables"
          >
            <Layers className="w-3.5 h-3.5 text-amber-300" />
            <span>Others</span>
          </button>
        </div>

        {/* Panel Controls (Maximize, Minimize, Close) */}
        <div className="flex items-center gap-1.5 text-slate-400 pl-2 shrink-0">
          {onToggleMaximize && (
            <button
              onClick={onToggleMaximize}
              className="p-1.5 rounded-md hover:bg-slate-800 hover:text-slate-200 transition-colors"
              title={isMaximized ? "Restore panel size" : "Maximize panel"}
            >
              {isMaximized ? <Minimize2 className="w-3.5 h-3.5" /> : <Maximize2 className="w-3.5 h-3.5" />}
            </button>
          )}

          {onClose && (
            <button
              onClick={onClose}
              className="p-1.5 rounded-md hover:bg-rose-950/50 hover:text-rose-300 transition-colors"
              title="Close right sidebar"
            >
              <X className="w-3.5 h-3.5" />
            </button>
          )}
        </div>
      </div>

      {/* ========================================================================= */}
      {/* 1. CODE PANEL (With Word-Wrap Toggle, File Switcher, Line Numbers, Copy)   */}
      {/* ========================================================================= */}
      {activeTab === 'code' && (
        <div className="flex-1 flex flex-col min-h-0 bg-[#0d1117]">
          {/* File selector sub-bar */}
          <div className="h-10 px-3 bg-slate-900/90 border-b border-slate-800 flex items-center justify-between shrink-0 overflow-x-auto no-scrollbar gap-2">
            <div className="flex items-center gap-1 font-mono text-xs">
              {sampleWorkspaceFiles.map((file, idx) => (
                <button
                  key={file.path}
                  onClick={() => {
                    setFileIdx(idx);
                    if (onSelectFileIndex) onSelectFileIndex(idx);
                  }}
                  className={`px-3 py-1.5 rounded-t text-xs font-mono flex items-center gap-2 border-t-2 transition-all whitespace-nowrap ${
                    fileIdx === idx
                      ? 'bg-[#0d1117] text-indigo-300 border-indigo-500 font-semibold'
                      : 'text-slate-400 hover:text-slate-200 border-transparent hover:bg-slate-800/50'
                  }`}
                >
                  <FileCode className="w-3.5 h-3.5 text-indigo-400" />
                  <span>{file.name}</span>
                </button>
              ))}
            </div>

            {/* Code toolbar */}
            <div className="flex items-center gap-1.5 shrink-0">
              <button
                onClick={() => setWordWrapEnabled(!wordWrapEnabled)}
                className={`px-2 py-1 rounded text-[11px] font-mono flex items-center gap-1 transition-all ${
                  wordWrapEnabled 
                    ? 'bg-indigo-600/30 text-indigo-300 border border-indigo-500/40' 
                    : 'bg-slate-800 text-slate-400 hover:text-slate-200'
                }`}
                title="Toggle text wrapping for long code lines"
              >
                <WrapText className="w-3 h-3" />
                <span className="hidden sm:inline">Wrap:</span>
                <span>{wordWrapEnabled ? 'ON' : 'OFF'}</span>
              </button>

              <button
                onClick={handleCopyCode}
                className="px-2 py-1 rounded text-[11px] font-mono bg-slate-800 hover:bg-slate-700 text-slate-300 flex items-center gap-1 transition-all"
                title="Copy code to clipboard"
              >
                {copiedCode ? <Check className="w-3 h-3 text-emerald-400" /> : <Copy className="w-3 h-3" />}
                <span className="hidden sm:inline">{copiedCode ? 'Copied' : 'Copy'}</span>
              </button>

              <button
                onClick={() => {
                  if (onRunCode) onRunCode(currentFile.name);
                  handleTabChange('others');
                  setOthersSubTab('terminal');
                  setTerminalLogs(prev => [
                    ...prev,
                    `[RUN] Executed ${currentFile.name} through isolated V8 sandbox`,
                    `[SUCCESS] 0 fatal warnings, memory: 14.2MB`
                  ]);
                }}
                className="px-2.5 py-1 rounded text-[11px] font-mono bg-emerald-600 hover:bg-emerald-500 text-white flex items-center gap-1 transition-all font-semibold shadow-sm"
                title="Execute code in terminal"
              >
                <Play className="w-3 h-3 fill-current" />
                <span>Run</span>
              </button>
            </div>
          </div>

          {/* File Path & Stats */}
          <div className="px-4 py-1.5 bg-slate-900/50 border-b border-slate-800/80 flex items-center justify-between text-[11px] font-mono text-slate-400">
            <span className="truncate break-all">{currentFile.path}</span>
            <span className="shrink-0">{currentFile.size} • TypeScript</span>
          </div>

          {/* Code Viewer Area with Guaranteed Word-Wrap or Scroll */}
          <div className={`flex-1 p-4 overflow-y-auto text-xs font-mono select-text ${
            wordWrapEnabled ? 'break-words overflow-wrap-anywhere whitespace-pre-wrap' : 'overflow-x-auto whitespace-pre'
          }`}>
            <pre className="text-slate-200 leading-relaxed font-mono">
              {currentFile.content.split('\n').map((line, lIdx) => (
                <div key={lIdx} className="table-row group hover:bg-slate-900/60">
                  <span className="table-cell pr-4 text-right select-none text-slate-600 font-mono text-[11px] group-hover:text-slate-400 w-8 shrink-0">
                    {lIdx + 1}
                  </span>
                  <span className={`table-cell ${
                    wordWrapEnabled ? 'break-words overflow-wrap-anywhere' : ''
                  }`}>
                    {line}
                  </span>
                </div>
              ))}
            </pre>
          </div>
        </div>
      )}

      {/* ========================================================================= */}
      {/* 2. VIDEOS PANEL (Interactive Player, Screencasts, AI Demos & Camera Feed) */}
      {/* ========================================================================= */}
      {activeTab === 'videos' && (
        <div className="flex-1 flex flex-col min-h-0 bg-slate-950 overflow-y-auto p-4 space-y-4">
          {/* Main Video Viewport */}
          <div className="rounded-xl overflow-hidden bg-slate-900 border border-slate-800 shadow-xl relative group">
            {/* Visual Screen / Video Simulator */}
            <div className="aspect-video w-full bg-gradient-to-br from-slate-950 via-[#0b1329] to-slate-950 relative flex items-center justify-center overflow-hidden">
              {/* Animated visual elements simulating real playback */}
              {selectedVideoId === 'demo' && (
                <div className="absolute inset-0 p-6 flex flex-col justify-between bg-slate-950/80">
                  <div className="flex items-center justify-between">
                    <span className="text-xs font-mono px-2 py-0.5 rounded bg-indigo-500/20 text-indigo-300 border border-indigo-500/30 flex items-center gap-1.5">
                      <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
                      AUTONOMOUS AGENT ACTIVE
                    </span>
                    <span className="text-[11px] font-mono text-slate-400">FPS: 60 • 1080p</span>
                  </div>

                  <div className="space-y-2 p-3 bg-slate-900/90 rounded-lg border border-slate-800 text-xs font-mono break-words">
                    <div className="text-emerald-400">✓ planner_agent: verified travel dates 12-20 April 2025</div>
                    <div className="text-cyan-400">⚡ coder_agent: generated dynamic map route with 4 Tokyo pins</div>
                    <div className="text-amber-400">🛡️ auditor: trust boundary verified - safe for execution</div>
                  </div>

                  <div className="text-[11px] text-slate-400 flex items-center justify-between font-mono">
                    <span>Task: Multi-Agent Parallel Planning Loop</span>
                    <span className="text-indigo-400">Sync: Local IPC</span>
                  </div>
                </div>
              )}

              {selectedVideoId === 'tokyo' && (
                <div className="absolute inset-0 bg-[radial-gradient(#ec4899_1px,transparent_1px)] [background-size:20px_20px] bg-slate-950 flex flex-col justify-between p-6">
                  <div className="flex items-center justify-between">
                    <span className="text-xs font-mono px-2 py-0.5 rounded bg-rose-500/20 text-rose-300 border border-rose-500/30">
                      4K TOKYO DRONE VLOG
                    </span>
                    <span className="text-[11px] font-mono text-slate-400">HDR 60FPS</span>
                  </div>
                  <div className="text-center space-y-1">
                    <div className="text-xl font-bold text-slate-100">Shinjuku & Shibuya Night Exploration</div>
                    <div className="text-xs text-slate-400 font-mono">Featured in your 8-Day Draft Itinerary</div>
                  </div>
                  <div className="flex justify-center gap-2">
                    <span className="px-2 py-0.5 rounded text-[10px] bg-slate-900 border border-slate-800 text-slate-300">Shinjuku Gyoen</span>
                    <span className="px-2 py-0.5 rounded text-[10px] bg-slate-900 border border-slate-800 text-slate-300">Shibuya Sky</span>
                    <span className="px-2 py-0.5 rounded text-[10px] bg-slate-900 border border-slate-800 text-slate-300">Sensō-ji</span>
                  </div>
                </div>
              )}

              {selectedVideoId === 'webcam' && (
                <div className="absolute inset-0 bg-[#09101d] flex flex-col justify-between p-6">
                  <div className="flex items-center justify-between">
                    <span className="text-xs font-mono px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-300 border border-emerald-500/30 flex items-center gap-1.5">
                      <Camera className="w-3.5 h-3.5 text-emerald-400" />
                      WEBCAM VISION STREAM
                    </span>
                    <span className="text-[11px] font-mono text-emerald-400">ONLINE</span>
                  </div>
                  <div className="flex flex-col items-center justify-center space-y-2">
                    <div className="w-20 h-20 rounded-full border-2 border-dashed border-cyan-400 animate-spin flex items-center justify-center">
                      <Camera className="w-8 h-8 text-cyan-400" />
                    </div>
                    <div className="text-xs font-mono text-slate-300">Bi-directional WebRTC Vision Ready</div>
                  </div>
                  <div className="text-[10px] text-slate-400 font-mono text-center">
                    Face mesh tracking • Gaze detection active
                  </div>
                </div>
              )}

              {selectedVideoId === 'arch' && (
                <div className="absolute inset-0 bg-slate-950 p-6 flex flex-col justify-between font-mono">
                  <div className="text-xs text-indigo-400 font-bold">SYSTEM ARCHITECTURE BREAKDOWN</div>
                  <div className="grid grid-cols-3 gap-2 text-center text-[10px]">
                    <div className="p-2 rounded bg-slate-900 border border-slate-800">
                      <div className="font-bold text-slate-200">Edge Daemon</div>
                      <div className="text-slate-400">12ms RPC</div>
                    </div>
                    <div className="p-2 rounded bg-slate-900 border border-slate-800">
                      <div className="font-bold text-indigo-300">HyperVM</div>
                      <div className="text-slate-400">V8 Isolated</div>
                    </div>
                    <div className="p-2 rounded bg-slate-900 border border-slate-800">
                      <div className="font-bold text-emerald-300">Cloud Mesh</div>
                      <div className="text-slate-400">Multi-Model</div>
                    </div>
                  </div>
                  <div className="text-[10px] text-slate-400">Zero-downtime failover between Gemini & local models</div>
                </div>
              )}

              {/* Big Play / Pause Overlay Button */}
              <button
                onClick={() => setIsVideoPlaying(!isVideoPlaying)}
                className="absolute w-12 h-12 rounded-full bg-indigo-600/90 hover:bg-indigo-500 text-white flex items-center justify-center shadow-lg transition-transform hover:scale-110 cursor-pointer"
              >
                {isVideoPlaying ? <Pause className="w-5 h-5 fill-current" /> : <Play className="w-5 h-5 fill-current ml-0.5" />}
              </button>
            </div>

            {/* Video Controls Bar */}
            <div className="p-3 bg-slate-900/95 border-t border-slate-800 space-y-2">
              {/* Scrubber timeline */}
              <div 
                className="h-1.5 w-full bg-slate-800 rounded-full cursor-pointer relative overflow-hidden group"
                onClick={(e) => {
                  const rect = e.currentTarget.getBoundingClientRect();
                  const pct = Math.round(((e.clientX - rect.left) / rect.width) * 100);
                  setVideoProgress(Math.max(0, Math.min(100, pct)));
                }}
              >
                <div 
                  className="h-full bg-gradient-to-r from-indigo-500 to-cyan-400 rounded-full transition-all"
                  style={{ width: `${videoProgress}%` }}
                />
              </div>

              {/* Controls row */}
              <div className="flex items-center justify-between text-xs">
                <div className="flex items-center gap-2">
                  <button
                    onClick={() => setIsVideoPlaying(!isVideoPlaying)}
                    className="p-1 rounded text-slate-300 hover:text-white hover:bg-slate-800"
                  >
                    {isVideoPlaying ? <Pause className="w-4 h-4 fill-current" /> : <Play className="w-4 h-4 fill-current" />}
                  </button>

                  <button
                    onClick={() => setVideoProgress(0)}
                    className="p-1 rounded text-slate-400 hover:text-slate-200 hover:bg-slate-800"
                    title="Rewind to start"
                  >
                    <RotateCcw className="w-3.5 h-3.5" />
                  </button>

                  <span className="text-[11px] font-mono text-slate-400">
                    01:34 / 04:18
                  </span>

                  <button
                    onClick={() => setIsVideoMuted(!isVideoMuted)}
                    className="p-1 rounded text-slate-400 hover:text-slate-200 hover:bg-slate-800 ml-1"
                  >
                    {isVideoMuted ? <VolumeX className="w-3.5 h-3.5 text-rose-400" /> : <Volume2 className="w-3.5 h-3.5" />}
                  </button>
                </div>

                <div className="flex items-center gap-1.5">
                  <button
                    onClick={() => {
                      const speeds: ('1.0x' | '1.5x' | '2.0x')[] = ['1.0x', '1.5x', '2.0x'];
                      const next = speeds[(speeds.indexOf(playbackSpeed) + 1) % speeds.length];
                      setPlaybackSpeed(next);
                    }}
                    className="px-2 py-0.5 rounded text-[10px] font-mono bg-slate-800 text-slate-300 hover:text-white"
                  >
                    {playbackSpeed}
                  </button>
                  <span className="text-[10px] font-mono px-1.5 py-0.5 rounded bg-indigo-500/20 text-indigo-300">
                    HD
                  </span>
                </div>
              </div>
            </div>
          </div>

          {/* Video Playlist Cards */}
          <div className="space-y-2">
            <div className="text-xs font-bold text-slate-400 uppercase tracking-wider font-mono">
              Available Video Feeds & Demos
            </div>

            <div className="space-y-2">
              {videoList.map((vid) => (
                <div
                  key={vid.id}
                  onClick={() => setSelectedVideoId(vid.id)}
                  className={`p-3 rounded-xl border transition-all cursor-pointer flex items-center justify-between ${
                    selectedVideoId === vid.id
                      ? 'bg-indigo-950/40 border-indigo-500/60 shadow-md'
                      : 'bg-slate-900/60 border-slate-800 hover:bg-slate-900 hover:border-slate-700'
                  }`}
                >
                  <div className="flex items-center gap-3 min-w-0">
                    <div className={`p-2.5 rounded-lg shrink-0 ${
                      selectedVideoId === vid.id ? 'bg-indigo-600 text-white' : 'bg-slate-800 text-slate-400'
                    }`}>
                      <Tv className="w-4 h-4" />
                    </div>
                    <div className="min-w-0">
                      <div className="text-xs font-bold text-slate-200 truncate">{vid.title}</div>
                      <div className="text-[11px] text-slate-400 break-words line-clamp-1">{vid.desc}</div>
                    </div>
                  </div>

                  <div className="text-right shrink-0 ml-2">
                    <span className="text-xs font-mono font-bold text-indigo-300">{vid.duration}</span>
                    <span className="block text-[9px] text-slate-500 uppercase">{vid.category}</span>
                  </div>
                </div>
              ))}
            </div>
          </div>
        </div>
      )}

      {/* ========================================================================= */}
      {/* 3. WEB BROWSER PANEL (Address Bar, Bookmarks, Responsive Wrapped Content)   */}
      {/* ========================================================================= */}
      {activeTab === 'browser' && (
        <div className="flex-1 flex flex-col min-h-0 bg-slate-950">
          {/* Browser Omnibar Header */}
          <div className="p-2 sm:p-3 bg-slate-900/90 border-b border-slate-800 space-y-2">
            <div className="flex items-center gap-2">
              <div className="flex items-center gap-1 text-slate-400">
                <button 
                  onClick={() => handleNavigateBrowser('tokyo', 'https://tokyo-travel-guide.internal/itinerary-2025')}
                  className="p-1 rounded hover:bg-slate-800 hover:text-slate-200"
                  title="Back"
                >
                  <ArrowLeft className="w-3.5 h-3.5" />
                </button>
                <button 
                  onClick={() => handleNavigateBrowser('docs', 'https://ai.google.dev/gemini-api/docs')}
                  className="p-1 rounded hover:bg-slate-800 hover:text-slate-200"
                  title="Forward"
                >
                  <ArrowRight className="w-3.5 h-3.5" />
                </button>
                <button
                  onClick={handleRefreshBrowser}
                  className={`p-1 rounded hover:bg-slate-800 hover:text-slate-200 ${isRefreshing ? 'animate-spin text-indigo-400' : ''}`}
                  title="Reload page"
                >
                  <RefreshCw className="w-3.5 h-3.5" />
                </button>
              </div>

              {/* URL Input Bar */}
              <div className="flex-1 flex items-center bg-slate-950 border border-slate-800 rounded-lg px-2.5 py-1 text-xs text-slate-200">
                <Lock className="w-3 h-3 text-emerald-400 mr-2 shrink-0" />
                <input
                  type="text"
                  value={browserInputUrl}
                  onChange={(e) => setBrowserInputUrl(e.target.value)}
                  onKeyDown={(e) => {
                    if (e.key === 'Enter') {
                      setBrowserUrl(browserInputUrl);
                      handleRefreshBrowser();
                    }
                  }}
                  className="w-full bg-transparent outline-none font-mono text-[11px] text-slate-300 break-all"
                  placeholder="Enter URL or search..."
                />
                <span className="text-[10px] text-slate-500 font-mono ml-1 shrink-0">SECURE</span>
              </div>
            </div>

            {/* Browser Bookmark Quick-Pills */}
            <div className="flex items-center gap-1.5 overflow-x-auto no-scrollbar text-xs">
              <button
                onClick={() => handleNavigateBrowser('tokyo', 'https://tokyo-travel-guide.internal/itinerary-2025')}
                className={`px-2.5 py-1 rounded-md text-[11px] font-medium flex items-center gap-1 transition-all whitespace-nowrap ${
                  browserPage === 'tokyo' ? 'bg-indigo-600 text-white' : 'bg-slate-800/80 text-slate-300 hover:bg-slate-800'
                }`}
              >
                <span>🗾</span>
                <span>Tokyo Guide</span>
              </button>

              <button
                onClick={() => handleNavigateBrowser('docs', 'https://ai.google.dev/gemini-api/docs')}
                className={`px-2.5 py-1 rounded-md text-[11px] font-medium flex items-center gap-1 transition-all whitespace-nowrap ${
                  browserPage === 'docs' ? 'bg-indigo-600 text-white' : 'bg-slate-800/80 text-slate-300 hover:bg-slate-800'
                }`}
              >
                <span>📖</span>
                <span>Gemini API Docs</span>
              </button>

              <button
                onClick={() => handleNavigateBrowser('github', 'https://github.com/aetherius-os/omni-runtime')}
                className={`px-2.5 py-1 rounded-md text-[11px] font-medium flex items-center gap-1 transition-all whitespace-nowrap ${
                  browserPage === 'github' ? 'bg-indigo-600 text-white' : 'bg-slate-800/80 text-slate-300 hover:bg-slate-800'
                }`}
              >
                <span>⚡</span>
                <span>GitHub Repo</span>
              </button>

              <button
                onClick={() => handleNavigateBrowser('map', 'https://maps.google.com/search/tokyo+landmarks')}
                className={`px-2.5 py-1 rounded-md text-[11px] font-medium flex items-center gap-1 transition-all whitespace-nowrap ${
                  browserPage === 'map' ? 'bg-indigo-600 text-white' : 'bg-slate-800/80 text-slate-300 hover:bg-slate-800'
                }`}
              >
                <span>🗺️</span>
                <span>Tokyo Landmarks</span>
              </button>
            </div>
          </div>

          {/* Rendered Web Browser Page Content - Guaranteed Text Wrap! */}
          <div className="flex-1 overflow-y-auto p-4 sm:p-5 space-y-4 break-words overflow-wrap-anywhere">
            {browserPage === 'tokyo' && (
              <div className="space-y-4">
                <div className="p-5 rounded-2xl bg-gradient-to-br from-indigo-950/60 via-slate-900 to-slate-950 border border-slate-800 space-y-3">
                  <div className="flex items-center justify-between">
                    <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-300 uppercase font-semibold">
                      Curated 2025 Guide
                    </span>
                    <span className="text-xs text-slate-400 font-mono">Updated 2h ago</span>
                  </div>
                  <h2 className="text-lg font-bold text-slate-100 break-words">
                    Tokyo Traveler's Dossier: Spring 2025 Cherry Blossom & Tech
                  </h2>
                  <p className="text-xs text-slate-300 leading-relaxed break-words whitespace-pre-wrap">
                    Tokyo in mid-April offers ideal weather (18°C–21°C), vibrant cherry blossoms at Shinjuku Gyoen, and seamless transit via the JR Yamanote line and Tokyo Metro PASMO cards.
                  </p>
                </div>

                <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                  <div className="p-3.5 rounded-xl bg-slate-900/80 border border-slate-800 space-y-2">
                    <div className="text-xs font-bold text-indigo-300 flex items-center gap-1.5">
                      <MapPin className="w-3.5 h-3.5 text-rose-400" />
                      <span>Shinjuku & Shibuya</span>
                    </div>
                    <p className="text-xs text-slate-400 break-words">
                      Omoide Yokocho street eats, Shibuya Crossing viewpoint from Magnet by Shibuya 109, and neon alleys.
                    </p>
                  </div>

                  <div className="p-3.5 rounded-xl bg-slate-900/80 border border-slate-800 space-y-2">
                    <div className="text-xs font-bold text-cyan-300 flex items-center gap-1.5">
                      <MapPin className="w-3.5 h-3.5 text-cyan-400" />
                      <span>Asakusa & Ueno</span>
                    </div>
                    <p className="text-xs text-slate-400 break-words">
                      Sensō-ji historic temple incense pavilion, traditional matcha stands, and Tokyo National Museum.
                    </p>
                  </div>
                </div>

                <div className="p-4 rounded-xl bg-slate-900 border border-slate-800 space-y-2">
                  <h4 className="text-xs font-bold text-slate-200">Recommended Flight Deals (Live Crawled)</h4>
                  <div className="space-y-1.5 text-xs font-mono">
                    <div className="flex justify-between p-2 rounded bg-slate-950 border border-slate-800/80">
                      <span>ANA NH212 (Non-stop)</span>
                      <span className="text-emerald-400 font-bold">£621 Return</span>
                    </div>
                    <div className="flex justify-between p-2 rounded bg-slate-950 border border-slate-800/80">
                      <span>British Airways BA007</span>
                      <span className="text-slate-300">£683 Return</span>
                    </div>
                  </div>
                </div>
              </div>
            )}

            {browserPage === 'docs' && (
              <div className="space-y-4">
                <div className="p-4 rounded-xl bg-slate-900 border border-slate-800 space-y-2">
                  <h3 className="text-sm font-bold text-slate-100 break-words">Gemini 2.5 Flash SDK Integration</h3>
                  <p className="text-xs text-slate-300 break-words whitespace-pre-wrap">
                    The modern @google/genai TypeScript SDK supports sub-second streaming inference, multi-modal vision inputs, and structured JSON output.
                  </p>
                </div>

                <div className="p-3 bg-[#0d1117] rounded-xl border border-slate-800 text-xs font-mono text-indigo-300 break-words whitespace-pre-wrap">
                  {`import { GoogleGenAI } from '@google/genai';\n\nconst ai = new GoogleGenAI();\nconst response = await ai.models.generateContent({\n  model: 'gemini-2.5-flash',\n  contents: 'Generate Tokyo itinerary json'\n});`}
                </div>
              </div>
            )}

            {browserPage === 'github' && (
              <div className="space-y-3">
                <div className="p-4 rounded-xl bg-slate-900 border border-slate-800 space-y-2">
                  <div className="flex items-center justify-between">
                    <span className="text-xs font-bold font-mono text-slate-100">aetherius-os / omni-runtime</span>
                    <span className="text-[10px] px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-300 font-mono">★ 4.8k stars</span>
                  </div>
                  <p className="text-xs text-slate-400 break-words">
                    Open-source agent runtime with tri-compute architecture, zero-code model swapping, and desktop bridge security bounds.
                  </p>
                </div>
              </div>
            )}

            {browserPage === 'map' && (
              <div className="space-y-3">
                <div className="h-64 rounded-xl bg-[#0e1626] border border-slate-800 relative overflow-hidden flex items-center justify-center p-4">
                  <div className="absolute inset-0 opacity-20 bg-[radial-gradient(#38bdf8_1px,transparent_1px)] [background-size:16px_16px]" />
                  <div className="text-center space-y-2 relative z-10">
                    <MapPin className="w-8 h-8 text-rose-500 mx-auto animate-bounce" />
                    <div className="text-xs font-bold text-slate-200">Tokyo Metropolitan Area Map</div>
                    <div className="text-[11px] text-slate-400 font-mono">4 Coordinates Pinpointed: Shinjuku, Shibuya, Asakusa, Skytree</div>
                  </div>
                </div>
              </div>
            )}
          </div>
        </div>
      )}

      {/* ========================================================================= */}
      {/* 4. OTHERS PANEL (Travel Itinerary, Autonomous Terminal, Artifacts)        */}
      {/* ========================================================================= */}
      {activeTab === 'others' && (
        <div className="flex-1 flex flex-col min-h-0 bg-slate-950">
          {/* Sub-tab pills */}
          <div className="h-10 px-3 bg-slate-900/90 border-b border-slate-800 flex items-center gap-1.5 shrink-0 select-none">
            <button
              onClick={() => setOthersSubTab('travel')}
              className={`px-2.5 py-1 rounded-md text-xs font-medium transition-all ${
                othersSubTab === 'travel' 
                  ? 'bg-indigo-600 text-white shadow-sm' 
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              Tokyo Trip Plan
            </button>
            <button
              onClick={() => setOthersSubTab('terminal')}
              className={`px-2.5 py-1 rounded-md text-xs font-medium transition-all ${
                othersSubTab === 'terminal' 
                  ? 'bg-indigo-600 text-white shadow-sm' 
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              Live Terminal
            </button>
            <button
              onClick={() => setOthersSubTab('artifacts')}
              className={`px-2.5 py-1 rounded-md text-xs font-medium transition-all ${
                othersSubTab === 'artifacts' 
                  ? 'bg-indigo-600 text-white shadow-sm' 
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              Artifacts & JSON
            </button>
          </div>

          {/* Travel Itinerary Sub-View */}
          {othersSubTab === 'travel' && (
            <div className="flex-1 overflow-y-auto p-4 space-y-4 break-words overflow-wrap-anywhere">
              {/* Flights Section */}
              <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-800 space-y-3">
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-2">
                    <Plane className="w-4 h-4 text-cyan-400" />
                    <h4 className="text-xs font-bold text-slate-100 uppercase tracking-wide">
                      Flights — London to Tokyo
                    </h4>
                  </div>
                  <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-300 font-semibold">
                    Direct
                  </span>
                </div>

                <div className="space-y-2">
                  {flights.map((f, idx) => (
                    <div
                      key={idx}
                      onClick={() => setSelectedFlight(idx)}
                      className={`p-3 rounded-lg border transition-all cursor-pointer flex items-center justify-between ${
                        selectedFlight === idx
                          ? 'bg-indigo-950/40 border-indigo-500/50 text-slate-100 shadow-md'
                          : 'bg-slate-950 border-slate-800/80 text-slate-300 hover:border-slate-700'
                      }`}
                    >
                      <div className="flex items-center gap-3">
                        <div className="p-2 rounded bg-slate-900 text-indigo-400 border border-slate-800 shrink-0">
                          <Plane className="w-3.5 h-3.5" />
                        </div>
                        <div className="min-w-0">
                          <div className="flex items-center gap-2">
                            <span className="text-xs font-bold truncate">{f.airline}</span>
                            {f.best && (
                              <span className="text-[9px] px-1.5 py-0.2 rounded bg-emerald-500/20 text-emerald-300 font-mono font-bold uppercase">
                                Best
                              </span>
                            )}
                          </div>
                          <div className="text-[11px] text-slate-400 font-mono">
                            {f.route} • {f.duration}
                          </div>
                        </div>
                      </div>

                      <div className="text-right shrink-0 ml-2">
                        <span className="text-sm font-black text-slate-100 font-mono">{f.price}</span>
                        <span className="block text-[10px] text-slate-500">Return</span>
                      </div>
                    </div>
                  ))}
                </div>
              </div>

              {/* Hotels Section */}
              <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-800 space-y-3">
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-2">
                    <Hotel className="w-4 h-4 text-purple-400" />
                    <h4 className="text-xs font-bold text-slate-100 uppercase tracking-wide">
                      Hotel Options — Tokyo
                    </h4>
                  </div>
                  <span className="text-[10px] text-slate-400 font-mono">3 top vetted</span>
                </div>

                {/* Cinematic Tokyo Banner matching Image 1 Showcase Window 2 */}
                <div className="relative h-28 rounded-xl overflow-hidden border border-slate-800 shadow-md">
                  <img src={tokyoNightImg} alt="Tokyo Night Neon" className="w-full h-full object-cover" />
                  <div className="absolute inset-0 bg-gradient-to-t from-black/80 via-black/20 to-transparent" />
                  <div className="absolute bottom-2 left-3 right-3 flex items-center justify-between text-xs">
                    <div>
                      <span className="font-bold text-white text-xs">Shinjuku & Shibuya Nightscape</span>
                      <p className="text-[10px] text-slate-300">Central transit, dining & vibrant districts</p>
                    </div>
                    <span className="text-[10px] px-2 py-0.5 rounded-full bg-purple-500/30 text-purple-200 border border-purple-400/30 font-mono">
                      Curated
                    </span>
                  </div>
                </div>

                <div className="grid grid-cols-1 sm:grid-cols-3 gap-2.5">
                  {hotels.map((h, idx) => (
                    <div key={idx} className="p-3 rounded-lg bg-slate-950 border border-slate-800/80 space-y-1.5 hover:border-purple-500/40 transition-all cursor-pointer">
                      <div className="flex justify-between items-start">
                        <span className="text-xs font-bold text-slate-200 line-clamp-1">{h.name}</span>
                        <span className="text-[10px] text-amber-400 font-bold shrink-0">{h.rating}</span>
                      </div>
                      <div className="text-[10px] text-slate-400 flex items-center gap-1">
                        <MapPin className="w-3 h-3 text-purple-400" />
                        <span>{h.area}</span>
                      </div>
                      <div className="pt-1 flex items-center justify-between text-xs font-mono">
                        <span className="text-slate-300 font-semibold">{h.price}</span>
                        <span className="text-[9px] px-1.5 py-0.2 rounded bg-slate-900 text-purple-300">
                          {h.tag}
                        </span>
                      </div>
                    </div>
                  ))}
                </div>
              </div>

              {/* Itinerary Map */}
              <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-800 space-y-3">
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-2">
                    <Calendar className="w-4 h-4 text-emerald-400" />
                    <h4 className="text-xs font-bold text-slate-100 uppercase tracking-wide">
                      Itinerary Route Map (12 - 20 Apr)
                    </h4>
                  </div>
                  <span className="text-[10px] font-mono text-emerald-400">8 Days</span>
                </div>

                <div className="h-36 rounded-lg bg-[#0e1626] border border-slate-800 relative overflow-hidden flex items-center justify-center p-3">
                  <div className="absolute inset-0 opacity-15 bg-[radial-gradient(#38bdf8_1px,transparent_1px)] [background-size:16px_16px]" />
                  <svg className="absolute inset-0 w-full h-full pointer-events-none">
                    <path d="M 60 70 Q 150 40 230 75 T 340 50" fill="none" stroke="#6366f1" strokeWidth="2" strokeDasharray="4 4" />
                  </svg>
                  <div className="absolute left-[15%] top-[45%] flex items-center gap-1 bg-slate-950/90 border border-rose-500/50 px-2 py-0.5 rounded-full text-[10px] text-slate-200 font-mono">
                    <span className="w-1.5 h-1.5 rounded-full bg-rose-500" />
                    <span>Shinjuku</span>
                  </div>
                  <div className="absolute left-[38%] top-[55%] flex items-center gap-1 bg-slate-950/90 border border-rose-500/50 px-2 py-0.5 rounded-full text-[10px] text-slate-200 font-mono">
                    <span className="w-1.5 h-1.5 rounded-full bg-rose-500" />
                    <span>Shibuya</span>
                  </div>
                  <div className="absolute right-[32%] top-[25%] flex items-center gap-1 bg-slate-950/90 border border-cyan-500/50 px-2 py-0.5 rounded-full text-[10px] text-slate-200 font-mono">
                    <span className="w-1.5 h-1.5 rounded-full bg-cyan-400" />
                    <span>Asakusa</span>
                  </div>
                  <div className="absolute right-[10%] top-[40%] flex items-center gap-1 bg-slate-950/90 border border-purple-500/50 px-2 py-0.5 rounded-full text-[10px] text-slate-200 font-mono">
                    <span className="w-1.5 h-1.5 rounded-full bg-purple-400" />
                    <span>Skytree</span>
                  </div>
                </div>
              </div>
            </div>
          )}

          {/* Terminal Sub-View */}
          {othersSubTab === 'terminal' && (
            <div className="flex-1 flex flex-col min-h-0 bg-[#070b12] font-mono text-xs p-3">
              <div className="flex items-center justify-between pb-2 border-b border-slate-800 text-[11px] text-slate-400">
                <span className="flex items-center gap-1.5">
                  <span className="w-2 h-2 rounded-full bg-emerald-400" />
                  <span>aetherius@hypervm:~</span>
                </span>
                <button
                  onClick={() => setTerminalLogs(['[CLEARED] Terminal buffer reset'])}
                  className="hover:text-slate-200 text-[10px] px-1.5 py-0.5 rounded bg-slate-900 border border-slate-800"
                >
                  Clear
                </button>
              </div>

              {/* Terminal Logs with Guaranteed Word-Wrap */}
              <div className="flex-1 overflow-y-auto py-2 space-y-1 select-text break-words overflow-wrap-anywhere whitespace-pre-wrap text-slate-300">
                {terminalLogs.map((log, idx) => (
                  <div 
                    key={idx} 
                    className={`${
                      log.startsWith('$') ? 'text-indigo-400 font-bold' : 
                      log.includes('[ERROR]') ? 'text-rose-400' :
                      log.includes('[OK]') || log.includes('[SUCCESS]') ? 'text-emerald-400' : 'text-slate-300'
                    }`}
                  >
                    {log}
                  </div>
                ))}
              </div>

              {/* Terminal Command Input */}
              <form onSubmit={handleAddTerminalCmd} className="flex items-center gap-2 pt-2 border-t border-slate-800">
                <span className="text-emerald-400 font-bold select-none">$</span>
                <input
                  type="text"
                  value={terminalInput}
                  onChange={(e) => setTerminalInput(e.target.value)}
                  placeholder="Enter command (e.g. status, ping, mesh-restart)..."
                  className="flex-1 bg-transparent outline-none text-slate-200 text-xs font-mono placeholder:text-slate-600"
                />
              </form>
            </div>
          )}

          {/* Artifacts Sub-View */}
          {othersSubTab === 'artifacts' && (
            <div className="flex-1 overflow-y-auto p-4 space-y-3 font-mono text-xs break-words overflow-wrap-anywhere">
              <div className="p-3 rounded-lg bg-slate-900 border border-slate-800 space-y-2">
                <div className="flex items-center justify-between">
                  <span className="font-bold text-slate-200">tokyo-trip-manifest.json</span>
                  <button 
                    onClick={() => {
                      navigator.clipboard.writeText(JSON.stringify({
                        destination: 'Tokyo, Japan',
                        dates: '2025-04-12 to 2025-04-20',
                        recommended_flight: 'ANA NH212 (£621)',
                        hotels: ['Hotel Gracery Shinjuku', 'Cerulean Tower Tokyu Hotel'],
                        coordinates: ['Shinjuku', 'Shibuya', 'Asakusa', 'Tokyo Skytree']
                      }, null, 2));
                    }}
                    className="p-1 rounded hover:bg-slate-800 text-slate-400 hover:text-slate-200"
                    title="Copy JSON"
                  >
                    <Copy className="w-3.5 h-3.5" />
                  </button>
                </div>
                <pre className="p-3 bg-slate-950 rounded border border-slate-800 text-slate-300 overflow-x-auto text-[11px] whitespace-pre-wrap break-words">
{`{
  "destination": "Tokyo, Japan",
  "dates": "2025-04-12 to 2025-04-20",
  "flight_recommendation": {
    "carrier": "ANA",
    "flight": "NH212",
    "route": "LHR -> HND",
    "price_gbp": 621
  },
  "neighborhoods": ["Shinjuku", "Shibuya", "Asakusa", "Sumida"]
}`}
                </pre>
              </div>
            </div>
          )}
        </div>
      )}
    </div>
  );
};
