import React, { useState } from 'react';
import { 
  Plane, 
  Hotel, 
  MapPin, 
  Calendar, 
  ExternalLink, 
  ChevronRight, 
  Check, 
  Copy, 
  Code2, 
  Globe, 
  Terminal, 
  Maximize2, 
  Minimize2, 
  X, 
  Minus, 
  Sparkles,
  RefreshCw,
  Clock,
  ArrowRight
} from 'lucide-react';
import { sampleWorkspaceFiles } from '../data/mockInitialData';

interface LiveWorkSurfaceProps {
  activeTab?: 'travel' | 'editor' | 'browser' | 'terminal';
  onSelectTab?: (tab: 'travel' | 'editor' | 'browser' | 'terminal') => void;
  selectedFileIndex?: number;
  onSelectFileIndex?: (idx: number) => void;
}

export const LiveWorkSurface: React.FC<LiveWorkSurfaceProps> = ({
  activeTab = 'travel',
  onSelectTab,
  selectedFileIndex = 0,
  onSelectFileIndex
}) => {
  const [currentTab, setCurrentTab] = useState<'travel' | 'editor' | 'browser' | 'terminal'>(activeTab);
  const [copied, setCopied] = useState(false);
  const [selectedFlight, setSelectedFlight] = useState(0);

  const handleTabChange = (t: 'travel' | 'editor' | 'browser' | 'terminal') => {
    setCurrentTab(t);
    if (onSelectTab) onSelectTab(t);
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

  const currentFile = sampleWorkspaceFiles[selectedFileIndex] || sampleWorkspaceFiles[0];

  return (
    <div className="h-full flex flex-col bg-slate-950/90 rounded-2xl border border-slate-800/90 overflow-hidden shadow-2xl">
      {/* WINDOW HEADER (Matches Panel 1 'Live Work Surface' in Showcase) */}
      <div className="h-10 px-4 bg-slate-900/90 border-b border-slate-800 flex items-center justify-between shrink-0 select-none">
        <div className="flex items-center gap-2">
          <span className="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" />
          <span className="text-xs font-bold text-slate-100 uppercase tracking-wider font-mono">
            Live Work Surface
          </span>
        </div>

        {/* Tab Switchers */}
        <div className="flex items-center bg-slate-950 border border-slate-800 rounded-lg p-0.5 text-xs">
          <button
            onClick={() => handleTabChange('travel')}
            className={`px-2.5 py-1 rounded font-medium transition-all ${
              currentTab === 'travel' 
                ? 'bg-indigo-600 text-white shadow-sm' 
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            Tokyo Trip Plan
          </button>
          <button
            onClick={() => handleTabChange('editor')}
            className={`px-2.5 py-1 rounded font-medium transition-all ${
              currentTab === 'editor' 
                ? 'bg-indigo-600 text-white shadow-sm' 
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            Code Editor
          </button>
          <button
            onClick={() => handleTabChange('terminal')}
            className={`px-2.5 py-1 rounded font-medium transition-all ${
              currentTab === 'terminal' 
                ? 'bg-indigo-600 text-white shadow-sm' 
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            Terminal
          </button>
        </div>

        {/* Window action controls */}
        <div className="flex items-center gap-2 text-slate-500">
          <Minus className="w-3.5 h-3.5 hover:text-slate-300 cursor-pointer" />
          <Maximize2 className="w-3.5 h-3.5 hover:text-slate-300 cursor-pointer" />
          <X className="w-3.5 h-3.5 hover:text-slate-300 cursor-pointer" />
        </div>
      </div>

      {/* CONTENT: TRAVEL ITINERARY & FLIGHTS (Panel 1 from Showcase) */}
      {currentTab === 'travel' && (
        <div className="flex-1 overflow-y-auto p-4 sm:p-5 space-y-4 no-scrollbar">
          {/* Card 1: Flights - London to Tokyo */}
          <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-800 space-y-3">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2">
                <Plane className="w-4 h-4 text-cyan-400" />
                <h4 className="text-xs font-bold text-slate-100 uppercase tracking-wide">
                  Flights — London to Tokyo
                </h4>
              </div>
              <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-300 font-semibold">
                Direct Flights
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
                    <div className="p-2 rounded bg-slate-900 text-indigo-400 border border-slate-800">
                      <Plane className="w-3.5 h-3.5" />
                    </div>
                    <div>
                      <div className="flex items-center gap-2">
                        <span className="text-xs font-bold">{f.airline}</span>
                        {f.best && (
                          <span className="text-[9px] px-1.5 py-0.2 rounded bg-emerald-500/20 text-emerald-300 uppercase font-mono font-bold">
                            Best
                          </span>
                        )}
                      </div>
                      <div className="text-[11px] text-slate-400 font-mono">
                        {f.route} • {f.duration}
                      </div>
                    </div>
                  </div>

                  <div className="text-right">
                    <span className="text-sm font-black text-slate-100 font-mono">{f.price}</span>
                    <span className="block text-[10px] text-slate-500">Return</span>
                  </div>
                </div>
              ))}
            </div>
          </div>

          {/* Card 2: Hotel Options - Tokyo */}
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

          {/* Card 3: Your Itinerary (Draft) */}
          <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-800 space-y-3">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2">
                <Calendar className="w-4 h-4 text-emerald-400" />
                <h4 className="text-xs font-bold text-slate-100 uppercase tracking-wide">
                  Your Itinerary (Draft — 8 Days)
                </h4>
              </div>
              <span className="text-[10px] font-mono text-emerald-400">12 - 20 Apr 2025</span>
            </div>

            {/* Simulated Interactive Map with Pins */}
            <div className="h-36 rounded-lg bg-[#0e1626] border border-slate-800 relative overflow-hidden flex items-center justify-center p-3">
              {/* Map grid lines simulation */}
              <div className="absolute inset-0 opacity-15 bg-[radial-gradient(#38bdf8_1px,transparent_1px)] [background-size:16px_16px]" />
              
              {/* Route lines */}
              <svg className="absolute inset-0 w-full h-full pointer-events-none">
                <path d="M 60 70 Q 150 40 230 75 T 340 50" fill="none" stroke="#6366f1" strokeWidth="2" strokeDasharray="4 4" />
              </svg>

              {/* Pins matching showcase */}
              <div className="absolute left-[15%] top-[45%] flex items-center gap-1 bg-slate-950/90 border border-rose-500/50 px-2 py-0.5 rounded-full text-[10px] text-slate-200 font-mono shadow-lg">
                <span className="w-1.5 h-1.5 rounded-full bg-rose-500" />
                <span>Shinjuku</span>
              </div>

              <div className="absolute left-[38%] top-[55%] flex items-center gap-1 bg-slate-950/90 border border-rose-500/50 px-2 py-0.5 rounded-full text-[10px] text-slate-200 font-mono shadow-lg">
                <span className="w-1.5 h-1.5 rounded-full bg-rose-500" />
                <span>Shibuya</span>
              </div>

              <div className="absolute right-[32%] top-[25%] flex items-center gap-1 bg-slate-950/90 border border-cyan-500/50 px-2 py-0.5 rounded-full text-[10px] text-slate-200 font-mono shadow-lg">
                <span className="w-1.5 h-1.5 rounded-full bg-cyan-400" />
                <span>Asakusa</span>
              </div>

              <div className="absolute right-[10%] top-[40%] flex items-center gap-1 bg-slate-950/90 border border-purple-500/50 px-2 py-0.5 rounded-full text-[10px] text-slate-200 font-mono shadow-lg">
                <span className="w-1.5 h-1.5 rounded-full bg-purple-400" />
                <span>Tokyo Skytree</span>
              </div>
            </div>

            {/* Day list preview */}
            <div className="grid grid-cols-2 sm:grid-cols-4 gap-2 text-[11px] font-mono">
              <div className="p-2 rounded bg-slate-950 border border-slate-800">
                <span className="text-slate-500 block text-[9px]">Day 1-2</span>
                <span className="text-slate-200 font-bold">Arrival & Shinjuku</span>
              </div>
              <div className="p-2 rounded bg-slate-950 border border-slate-800">
                <span className="text-slate-500 block text-[9px]">Day 3-4</span>
                <span className="text-slate-200 font-bold">Shibuya & Harajuku</span>
              </div>
              <div className="p-2 rounded bg-slate-950 border border-slate-800">
                <span className="text-slate-500 block text-[9px]">Day 5-6</span>
                <span className="text-slate-200 font-bold">Asakusa & Ueno</span>
              </div>
              <div className="p-2 rounded bg-slate-950 border border-slate-800">
                <span className="text-slate-500 block text-[9px]">Day 7-8</span>
                <span className="text-slate-200 font-bold">Akihabara & Ginza</span>
              </div>
            </div>
          </div>
        </div>
      )}

      {/* CONTENT: CODE EDITOR (When toggled to IDE) */}
      {currentTab === 'editor' && (
        <div className="flex-1 flex flex-col overflow-hidden bg-[#0d1117]">
          {/* File Tab Bar */}
          <div className="h-8 bg-slate-950 border-b border-slate-800 px-2 flex items-center gap-1 overflow-x-auto">
            {sampleWorkspaceFiles.map((file, idx) => (
              <button
                key={file.path}
                onClick={() => onSelectFileIndex && onSelectFileIndex(idx)}
                className={`px-3 py-1 rounded-t text-xs font-mono flex items-center gap-2 border-t-2 transition-all ${
                  selectedFileIndex === idx
                    ? 'bg-[#0d1117] border-indigo-500 text-slate-100 font-semibold'
                    : 'border-transparent text-slate-400 hover:text-slate-200 hover:bg-slate-900/60'
                }`}
              >
                <span>{file.name}</span>
              </button>
            ))}
          </div>

          {/* Code Viewer */}
          <div className="flex-1 p-4 overflow-y-auto font-mono text-xs text-slate-200 leading-relaxed">
            <pre>
              <code>{currentFile.content}</code>
            </pre>
          </div>
        </div>
      )}

      {/* CONTENT: TERMINAL (When toggled to Terminal) */}
      {currentTab === 'terminal' && (
        <div className="flex-1 p-4 font-mono text-xs text-slate-300 bg-[#070b12] overflow-y-auto space-y-1">
          <div className="text-slate-500 mb-2">OmniAgent / Nexus Autonomous Terminal v3.4</div>
          <div className="text-indigo-400">[10:14:03] Starting task: Plan Tokyo trip next month...</div>
          <div className="text-slate-300">[10:14:05] Querying Flight API: LHR → HND direct routes...</div>
          <div className="text-emerald-400">[10:14:08] Found 3 direct flights: ANA (£621), BA (£683), JAL (£701)</div>
          <div className="text-slate-300">[10:14:12] Checking hotel availability in Shinjuku & Shibuya...</div>
          <div className="text-indigo-400">[10:14:15] Compiling 8-day itinerary draft with interactive map coordinates...</div>
          <div className="text-emerald-400 font-bold">[10:14:18] Complete. Live Work Surface updated in 15ms.</div>
        </div>
      )}
    </div>
  );
};
