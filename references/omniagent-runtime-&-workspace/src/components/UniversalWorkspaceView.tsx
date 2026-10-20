import React, { useState } from 'react';
import { 
  Palette, 
  Box, 
  Gamepad2, 
  Music, 
  Users, 
  Terminal, 
  Cpu, 
  Sliders, 
  Sparkles, 
  Play, 
  CheckCircle2, 
  Maximize2, 
  Volume2, 
  FileCode, 
  Wrench,
  Search,
  ExternalLink,
  Layers,
  ChevronRight,
  ShieldCheck,
  Bot
} from 'lucide-react';
import novaTacticalImg from '../assets/images/nova_avatar_1790060648972.jpg';
import tokyoNightImg from '../assets/images/tokyo_neon_night_1790063676819.jpg';
import lunaImg from '../assets/images/luna_purple_avatar_1790063661430.jpg';
import { ModelAdapterInfo } from '../types';

interface UniversalWorkspaceViewProps {
  activeModel: ModelAdapterInfo;
}

export const UniversalWorkspaceView: React.FC<UniversalWorkspaceViewProps> = ({
  activeModel
}) => {
  const [selectedStudio, setSelectedStudio] = useState<
    'default' | 'photoshop' | 'cad' | 'game' | 'music' | 'specialists' | 'worker' | 'benchmarks'
  >('default');

  return (
    <div className="flex-1 flex flex-col h-full bg-[#080d1a] text-slate-100 overflow-y-auto no-scrollbar font-sans selection:bg-cyan-500/30">
      
      {/* 1. TOP HEADER & STUDIO SELECTOR */}
      <header className="h-16 px-4 sm:px-6 bg-[#091122]/95 backdrop-blur-md border-b border-slate-800/80 flex items-center justify-between shrink-0 sticky top-0 z-20">
        <div className="flex items-center gap-3">
          <div className="w-9 h-9 rounded-xl bg-gradient-to-tr from-cyan-400 via-indigo-500 to-purple-500 p-[1px] shadow-lg shadow-cyan-500/20">
            <div className="w-full h-full bg-[#080d1a] rounded-[11px] flex items-center justify-center font-black text-cyan-300">
              ⚡
            </div>
          </div>
          <div>
            <div className="text-sm font-black text-white flex items-center gap-2">
              <span>UNIVERSAL WORKSPACE</span>
              <span className="text-[10px] px-1.5 py-0.5 rounded bg-cyan-500/20 text-cyan-300 font-mono">SHOWCASE</span>
            </div>
            <p className="text-[11px] text-slate-400">Your AI. Your Space. Everything in One Place.</p>
          </div>
        </div>

        {/* Studio Tabs */}
        <div className="hidden md:flex items-center gap-1 bg-[#050811] p-1 rounded-xl border border-slate-800 text-xs">
          {[
            { id: 'default', label: '1. Default', icon: Sparkles },
            { id: 'photoshop', label: '2. Photoshop', icon: Palette },
            { id: 'cad', label: '3. CAD Design', icon: Box },
            { id: 'game', label: '4. Game Dev', icon: Gamepad2 },
            { id: 'music', label: '5. Music DAW', icon: Music },
            { id: 'specialists', label: '6. AI Team', icon: Users },
            { id: 'worker', label: '8. Worker View', icon: FileCode },
            { id: 'benchmarks', label: '10. Models & MCP', icon: Cpu }
          ].map(tab => {
            const Icon = tab.icon;
            const isActive = selectedStudio === tab.id;
            return (
              <button
                key={tab.id}
                onClick={() => setSelectedStudio(tab.id as any)}
                className={`flex items-center gap-1 px-2.5 py-1.5 rounded-lg font-semibold transition-all ${
                  isActive
                    ? 'bg-gradient-to-r from-cyan-600 to-indigo-600 text-white shadow-md shadow-cyan-600/30'
                    : 'text-slate-400 hover:text-slate-200'
                }`}
              >
                <Icon className="w-3.5 h-3.5" />
                <span>{tab.label}</span>
              </button>
            );
          })}
        </div>
      </header>

      {/* 2. STUDIO VIEWPORTS */}
      <div className="p-4 flex-1 flex flex-col justify-between">
        
        {/* ARCHETYPE 1: THE DEFAULT EXPERIENCE */}
        {selectedStudio === 'default' && (
          <div className="bg-[#070b14] rounded-2xl border border-slate-800 p-6 flex flex-col lg:flex-row gap-6 shadow-2xl items-center">
            {/* Avatar & Chat */}
            <div className="w-full lg:w-72 flex flex-col items-center text-center shrink-0">
              <div className="w-48 h-48 rounded-2xl overflow-hidden ring-4 ring-cyan-500/30 shadow-2xl mb-4">
                <img src={novaTacticalImg} alt="Nova" className="w-full h-full object-cover" />
              </div>
              <div className="p-3 rounded-2xl bg-slate-900 border border-slate-800 text-xs text-left w-full space-y-2">
                <div className="font-bold text-white">How can I help you today?</div>
                <div className="space-y-1 text-slate-300">
                  <div className="flex items-center gap-1.5"><span>✦</span> Create something</div>
                  <div className="flex items-center gap-1.5"><span>✦</span> Answer a question</div>
                  <div className="flex items-center gap-1.5"><span>✦</span> Work on a project</div>
                  <div className="flex items-center gap-1.5"><span>✦</span> Open an app or file</div>
                </div>
              </div>
            </div>

            {/* Universal Workspace Canvas */}
            <div className="flex-1 w-full bg-slate-950 rounded-2xl border border-slate-800 overflow-hidden relative min-h-[380px] flex flex-col justify-between p-4 shadow-inner">
              <div className="relative flex-1 rounded-xl overflow-hidden mb-4">
                <img src={tokyoNightImg} alt="Workspace Scene" className="w-full h-full object-cover filter brightness-90" />
                <div className="absolute inset-0 bg-gradient-to-t from-black/80 via-transparent to-transparent" />
                <div className="absolute bottom-4 left-4">
                  <h3 className="text-xl font-extrabold text-white">Your Universal Workspace</h3>
                  <p className="text-xs text-slate-300">Create. Edit. Design. Build. Play. AI in this window.</p>
                </div>
              </div>

              {/* Bottom Quick Tools */}
              <div className="flex items-center justify-between pt-2 border-t border-slate-800 text-xs text-slate-300">
                <div className="flex items-center gap-2">
                  <span className="px-2 py-1 rounded-lg bg-slate-900 border border-slate-800">💬 Chat</span>
                  <span className="px-2 py-1 rounded-lg bg-slate-900 border border-slate-800">📁 Files</span>
                  <span className="px-2 py-1 rounded-lg bg-slate-900 border border-slate-800">⚡ Apps</span>
                  <span className="px-2 py-1 rounded-lg bg-slate-900 border border-slate-800">🛠️ Tools</span>
                </div>
                <span className="text-[10px] text-cyan-400 font-mono">OmniEngine Live Canvas</span>
              </div>
            </div>
          </div>
        )}

        {/* ARCHETYPE 2: IMAGE EDITING (PHOTOSHOP) */}
        {selectedStudio === 'photoshop' && (
          <div className="bg-[#070b14] rounded-2xl border border-slate-800 p-6 flex flex-col lg:flex-row gap-6 shadow-2xl">
            <div className="w-full lg:w-72 shrink-0 space-y-4">
              <div className="w-36 h-36 rounded-2xl overflow-hidden ring-2 ring-cyan-500/40">
                <img src={novaTacticalImg} alt="Assistant" className="w-full h-full object-cover" />
              </div>
              <div className="p-3.5 rounded-2xl bg-slate-900 border border-slate-800 text-xs space-y-2">
                <div className="font-bold text-white flex items-center gap-1.5">
                  <Palette className="w-4 h-4 text-cyan-400" />
                  <span>AI Creative Assistant</span>
                </div>
                <p className="text-slate-300 text-[11px]">
                  "I can help with that edit. Want me to remove the background and enhance lighting?"
                </p>
                <div className="space-y-1 pt-1 border-t border-slate-800 text-[11px]">
                  <div className="text-emerald-400 flex items-center gap-1">✓ Background removed</div>
                  <div className="text-emerald-400 flex items-center gap-1">✓ Lighting enhanced</div>
                  <div className="text-emerald-400 flex items-center gap-1">✓ Color graded</div>
                  <div className="text-emerald-400 flex items-center gap-1">✓ Sharpened details</div>
                  <div className="text-cyan-300 font-semibold pt-1">Ready for your review!</div>
                </div>
              </div>
            </div>

            {/* Photoshop Workspace */}
            <div className="flex-1 bg-slate-950 rounded-2xl border border-slate-800 p-4 flex flex-col justify-between">
              <div className="flex items-center justify-between pb-3 border-b border-slate-800 text-xs">
                <span className="font-bold text-white flex items-center gap-2">
                  <span className="px-2 py-0.5 rounded bg-blue-600 text-white font-black text-xs">Ps</span>
                  <span>Adobe Photoshop Engine • Landscape_Hero_Master.psd</span>
                </span>
                <span className="text-[10px] text-slate-400 font-mono">4K UHD • 16-bit ProPhoto RGB</span>
              </div>

              <div className="relative my-3 rounded-xl overflow-hidden h-72 border border-slate-800">
                <img src={tokyoNightImg} alt="Canvas" className="w-full h-full object-cover" />
                <div className="absolute top-3 right-3 p-2 rounded-xl bg-black/80 backdrop-blur-md border border-white/10 text-[10px] space-y-1">
                  <div className="font-bold text-white">Layers (4)</div>
                  <div className="text-slate-300">👁️ Subject</div>
                  <div className="text-slate-300">👁️ Lighting</div>
                  <div className="text-slate-300">👁️ Color Grade</div>
                  <div className="text-slate-300">👁️ Background</div>
                </div>
              </div>

              <div className="flex items-center justify-between text-xs pt-2 border-t border-slate-800">
                <span className="text-slate-400">Full creative control. AI assists or takes the lead.</span>
                <button className="px-3 py-1.5 rounded-xl bg-cyan-600 hover:bg-cyan-500 text-white font-bold">
                  Export High-Res PSD
                </button>
              </div>
            </div>
          </div>
        )}

        {/* ARCHETYPE 3: CAD DESIGN */}
        {selectedStudio === 'cad' && (
          <div className="bg-[#070b14] rounded-2xl border border-slate-800 p-6 flex flex-col lg:flex-row gap-6 shadow-2xl">
            <div className="w-full lg:w-72 shrink-0 space-y-4">
              <div className="p-4 rounded-2xl bg-slate-900 border border-slate-800 text-xs space-y-2.5">
                <div className="font-bold text-white flex items-center gap-1.5">
                  <Box className="w-4 h-4 text-cyan-400" />
                  <span>CAD Structural Analysis</span>
                </div>
                <div className="space-y-1.5 text-[11px]">
                  <div className="text-emerald-400 flex items-center gap-1.5">✓ Geometry Valid</div>
                  <div className="text-emerald-400 flex items-center gap-1.5">✓ Load Test Passed (14.2 kN)</div>
                  <div className="text-emerald-400 flex items-center gap-1.5">✓ Material OK (Ti-6Al-4V)</div>
                  <div className="text-emerald-400 flex items-center gap-1.5">✓ Zero Clearance Conflicts</div>
                </div>
                <button className="w-full py-2 rounded-xl bg-gradient-to-r from-cyan-600 to-indigo-600 text-white font-bold text-xs mt-2">
                  Run Simulation
                </button>
              </div>
            </div>

            <div className="flex-1 bg-slate-950 rounded-2xl border border-slate-800 p-4 flex flex-col justify-between">
              <div className="flex items-center justify-between pb-3 border-b border-slate-800 text-xs">
                <span className="font-bold text-white">Parametric Turbine Flange Assembly • 3D Mesh</span>
                <span className="text-[10px] text-emerald-400 font-mono">FEA Mesh: 142,800 Polygons</span>
              </div>
              <div className="h-72 rounded-xl bg-[#03060d] border border-cyan-500/30 flex items-center justify-center my-3 relative overflow-hidden">
                <div className="absolute inset-0 bg-[radial-gradient(#1e293b_1px,transparent_1px)] [background-size:16px_16px] opacity-40" />
                <div className="text-center z-10">
                  <div className="w-24 h-24 mx-auto rounded-3xl bg-cyan-500/10 border-2 border-cyan-400 flex items-center justify-center text-4xl shadow-xl shadow-cyan-500/20">
                    ⚙️
                  </div>
                  <div className="font-bold text-white text-sm mt-3">High-Stress Turbine Flange</div>
                  <div className="text-[11px] text-cyan-400">Stress: 42.1 MPa • Yield: 880 MPa • Factor: 2.1x</div>
                </div>
              </div>
              <div className="flex items-center justify-between text-xs pt-2 border-t border-slate-800">
                <span className="text-slate-400">Design. Simulate. Validate. All in the same space.</span>
                <span className="text-cyan-300 font-mono">STEP / IGES Export Ready</span>
              </div>
            </div>
          </div>
        )}

        {/* ARCHETYPE 4: GAME DEVELOPMENT */}
        {selectedStudio === 'game' && (
          <div className="bg-[#070b14] rounded-2xl border border-slate-800 p-6 flex flex-col lg:flex-row gap-6 shadow-2xl">
            <div className="w-full lg:w-72 shrink-0 space-y-4">
              <div className="p-4 rounded-2xl bg-slate-900 border border-slate-800 text-xs space-y-2.5">
                <div className="font-bold text-white flex items-center gap-1.5">
                  <Gamepad2 className="w-4 h-4 text-purple-400" />
                  <span>AI Play Test Results</span>
                </div>
                <div className="space-y-1.5 text-[11px]">
                  <div className="text-emerald-400 flex items-center gap-1.5">✓ FPS: 120 Stable</div>
                  <div className="text-emerald-400 flex items-center gap-1.5">✓ No Critical Crashes</div>
                  <div className="text-amber-400 flex items-center gap-1.5">⚠️ 2 Minor NavMesh Issues</div>
                  <div className="text-emerald-400 flex items-center gap-1.5">✓ Boss AI Behavior: Good</div>
                  <div className="text-emerald-400 flex items-center gap-1.5">✓ VRAM Memory: Optimal</div>
                </div>
                <button className="w-full py-2 rounded-xl bg-purple-600 hover:bg-purple-500 text-white font-bold text-xs mt-2">
                  Apply Autonomous Fixes
                </button>
              </div>
            </div>

            <div className="flex-1 bg-slate-950 rounded-2xl border border-slate-800 p-4 flex flex-col justify-between">
              <div className="flex items-center justify-between pb-3 border-b border-slate-800 text-xs">
                <span className="font-bold text-white">Unreal Engine 5.4 • Eldoria_Ruins_Playtest.uproject</span>
                <span className="text-[10px] text-purple-400 font-mono">DirectX 12 • Lumen Active</span>
              </div>
              <div className="h-72 rounded-xl bg-[#03060d] border border-purple-500/30 flex items-center justify-center my-3 relative overflow-hidden">
                <div className="text-center z-10">
                  <div className="text-5xl mb-2">🎮</div>
                  <div className="font-bold text-white text-sm">Autonomous Bot Playtesting in Ruins</div>
                  <div className="text-[11px] text-slate-400">Pathfinding verified across 1,400 waypoints</div>
                </div>
              </div>
              <div className="flex items-center justify-between text-xs pt-2 border-t border-slate-800">
                <span className="text-slate-400">Build. Test. Play. Iterate with AI.</span>
                <span className="text-purple-300 font-mono">Automated CI/CD Pipeline</span>
              </div>
            </div>
          </div>
        )}

        {/* ARCHETYPE 5: MUSIC PRODUCTION (DAW) */}
        {selectedStudio === 'music' && (
          <div className="bg-[#070b14] rounded-2xl border border-slate-800 p-6 flex flex-col lg:flex-row gap-6 shadow-2xl">
            <div className="w-full lg:w-72 shrink-0 space-y-4">
              <div className="p-4 rounded-2xl bg-slate-900 border border-slate-800 text-xs space-y-2.5">
                <div className="font-bold text-white flex items-center gap-1.5">
                  <Music className="w-4 h-4 text-cyan-400" />
                  <span>AI Music Assistant</span>
                </div>
                <div className="space-y-1 text-slate-300 text-[11px]">
                  <button className="w-full p-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-left">🎼 Compose Melody</button>
                  <button className="w-full p-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-left">🎹 Suggest Sounds & Patches</button>
                  <button className="w-full p-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-left">🎚️ Auto-Mix & Master</button>
                  <button className="w-full p-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-left">🎛️ Remove Noise & Stems</button>
                  <button className="w-full p-1.5 rounded-lg bg-cyan-600 text-white font-bold text-center mt-1">Export Track (WAV 24b)</button>
                </div>
              </div>
            </div>

            <div className="flex-1 bg-slate-950 rounded-2xl border border-slate-800 p-4 flex flex-col justify-between">
              <div className="flex items-center justify-between pb-3 border-b border-slate-800 text-xs">
                <span className="font-bold text-white">Synthesizer Studio • Cyber_Twilight_128BPM.als</span>
                <span className="text-[10px] text-cyan-400 font-mono">128 BPM • Key: D Minor</span>
              </div>

              {/* DAW Stems */}
              <div className="space-y-2 my-3">
                {[
                  { name: 'Drum Kit 808', color: 'bg-rose-500' },
                  { name: 'Sub Bass Sine', color: 'bg-indigo-500' },
                  { name: 'Lead Synth Arp', color: 'bg-cyan-500' },
                  { name: 'Ambient Vocal Pad', color: 'bg-purple-500' }
                ].map(track => (
                  <div key={track.name} className="p-2 rounded-xl bg-slate-900 border border-slate-800 flex items-center gap-3 text-xs">
                    <span className="w-24 font-bold text-slate-200 truncate">{track.name}</span>
                    <div className="flex-1 h-6 bg-slate-950 rounded-lg overflow-hidden flex items-center px-2 gap-1">
                      {[...Array(24)].map((_, i) => (
                        <div key={i} className={`flex-1 rounded-sm ${track.color}`} style={{ height: `${Math.sin(i * 0.8) * 50 + 50}%` }} />
                      ))}
                    </div>
                  </div>
                ))}
              </div>

              <div className="flex items-center justify-between text-xs pt-2 border-t border-slate-800">
                <span className="text-slate-400">Compose. Produce. Mix. With AI at your side.</span>
                <span className="text-cyan-400 font-mono">Low-Latency ASIO Output</span>
              </div>
            </div>
          </div>
        )}

        {/* ARCHETYPE 6: OPTIONAL AI TEAM (4 SPECIALISTS) */}
        {selectedStudio === 'specialists' && (
          <div className="bg-[#070b14] rounded-2xl border border-slate-800 p-6 space-y-4 shadow-2xl">
            <div>
              <h3 className="text-base font-extrabold text-white">Optional AI Team (Visible)</h3>
              <p className="text-xs text-slate-400">Bring in specialists when you want them. Same interface. More minds. Your choice.</p>
            </div>

            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
              {[
                { name: 'Tech Specialist', desc: 'Coding | Systems | DevOps', icon: '👨‍💻', badge: 'Active' },
                { name: 'Creative Specialist', desc: 'Design | Media | Animation', icon: '👩‍🎨', badge: 'Standby' },
                { name: 'Analysis Specialist', desc: 'Research | Data | Finance', icon: '📊', badge: 'Standby' },
                { name: 'Strategy Specialist', desc: 'Planning | Operations | Growth', icon: '🧭', badge: 'Standby' }
              ].map(spec => (
                <div key={spec.name} className="p-4 rounded-2xl bg-slate-900 border border-slate-800 hover:border-cyan-500/50 transition-all text-center space-y-2">
                  <div className="w-16 h-16 rounded-2xl bg-slate-800 mx-auto flex items-center justify-center text-3xl shadow-md">
                    {spec.icon}
                  </div>
                  <div className="font-bold text-white text-sm">{spec.name}</div>
                  <div className="text-[11px] text-slate-400">{spec.desc}</div>
                  <span className="inline-block text-[10px] px-2 py-0.5 rounded-full bg-cyan-500/20 text-cyan-300 border border-cyan-500/30">
                    {spec.badge}
                  </span>
                </div>
              ))}
            </div>
          </div>
        )}

        {/* ARCHETYPE 8: WORKER VIEW (FULL SCREEN) */}
        {selectedStudio === 'worker' && (
          <div className="bg-[#070b14] rounded-2xl border border-slate-800 p-6 flex flex-col lg:flex-row gap-6 shadow-2xl">
            <div className="w-full lg:w-72 shrink-0 space-y-4">
              <div className="w-40 h-40 rounded-2xl bg-slate-800 mx-auto flex items-center justify-center text-6xl shadow-xl ring-2 ring-cyan-500/30">
                👨‍💻
              </div>
              <div className="text-center">
                <div className="font-bold text-white text-base">Tech Specialist</div>
                <div className="text-xs text-cyan-400">Coding | Systems | DevOps</div>
              </div>
              <div className="p-3.5 rounded-2xl bg-slate-900 border border-slate-800 text-xs space-y-2">
                <div className="font-bold text-white">Task Details</div>
                <div className="text-slate-300 text-[11px]">Task: Optimize game loop & memory</div>
                <div className="text-slate-300 text-[11px]">Status: In Progress (78%)</div>
                <div className="text-slate-300 text-[11px]">ETA: 2 minutes</div>
                <div className="w-full h-1.5 bg-slate-800 rounded-full overflow-hidden mt-1">
                  <div className="h-full bg-cyan-400 rounded-full w-[78%]" />
                </div>
              </div>
            </div>

            <div className="flex-1 bg-slate-950 rounded-2xl border border-slate-800 p-4 space-y-3">
              <div className="font-bold text-white text-sm">Autonomous Execution Log</div>
              <div className="space-y-2 text-xs font-mono">
                <div className="text-emerald-400 flex items-center gap-2">✓ [10:24:02] Reading codebase AST tree...</div>
                <div className="text-emerald-400 flex items-center gap-2">✓ [10:24:08] Applying parallel vector memory improvements...</div>
                <div className="text-emerald-400 flex items-center gap-2">✓ [10:24:14] Running test suites (14/14 passed)...</div>
                <div className="text-cyan-400 flex items-center gap-2 animate-pulse">⚡ [10:24:19] Finalizing bundle chunk distribution...</div>
              </div>
            </div>
          </div>
        )}

        {/* ARCHETYPE 10: MODELS, BENCHMARKS & MCP PLUGINS */}
        {selectedStudio === 'benchmarks' && (
          <div className="bg-[#070b14] rounded-2xl border border-slate-800 p-6 space-y-6 shadow-2xl">
            <div>
              <h3 className="text-base font-extrabold text-white">Models, Benchmarks & Plugins</h3>
              <p className="text-xs text-slate-400">Power under the hood. Choice at your fingertips. Use the best tool for the job.</p>
            </div>

            <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
              {/* Models */}
              <div className="p-4 rounded-2xl bg-slate-900 border border-slate-800 space-y-2 text-xs">
                <div className="font-bold text-cyan-300">AI Models Supported</div>
                <div className="space-y-1 text-slate-300">
                  <div className="p-1.5 rounded-lg bg-slate-950 flex items-center justify-between">
                    <span>Gemini 3.8 Flash (Active)</span>
                    <span className="text-emerald-400 font-bold">1M Context</span>
                  </div>
                  <div className="p-1.5 rounded-lg bg-slate-950 flex items-center justify-between">
                    <span>Claude 3.5 Sonnet</span>
                    <span className="text-slate-400">Available</span>
                  </div>
                  <div className="p-1.5 rounded-lg bg-slate-950 flex items-center justify-between">
                    <span>GPT-4o</span>
                    <span className="text-slate-400">Available</span>
                  </div>
                  <div className="p-1.5 rounded-lg bg-slate-950 flex items-center justify-between">
                    <span>Llama 3 70B (Local)</span>
                    <span className="text-slate-400">Ollama Ready</span>
                  </div>
                </div>
              </div>

              {/* Benchmarks */}
              <div className="p-4 rounded-2xl bg-slate-900 border border-slate-800 space-y-2 text-xs">
                <div className="font-bold text-indigo-300">Performance Benchmarks</div>
                <div className="space-y-2 text-[11px]">
                  <div>
                    <div className="flex justify-between text-slate-300"><span>Reasoning</span><span>94%</span></div>
                    <div className="w-full h-1 bg-slate-800 rounded mt-0.5"><div className="h-full bg-indigo-500 rounded w-[94%]" /></div>
                  </div>
                  <div>
                    <div className="flex justify-between text-slate-300"><span>Coding & Refactoring</span><span>98%</span></div>
                    <div className="w-full h-1 bg-slate-800 rounded mt-0.5"><div className="h-full bg-cyan-400 rounded w-[98%]" /></div>
                  </div>
                  <div>
                    <div className="flex justify-between text-slate-300"><span>Multimodal Vision</span><span>96%</span></div>
                    <div className="w-full h-1 bg-slate-800 rounded mt-0.5"><div className="h-full bg-purple-500 rounded w-[96%]" /></div>
                  </div>
                </div>
              </div>

              {/* Plugins & MCP */}
              <div className="p-4 rounded-2xl bg-slate-900 border border-slate-800 space-y-2 text-xs">
                <div className="font-bold text-purple-300">Plugins & MCP Tools</div>
                <div className="space-y-1 text-slate-300">
                  <div className="flex items-center gap-2"><span>🌐</span> Web Search & Grounding</div>
                  <div className="flex items-center gap-2"><span>🐙</span> GitHub & Git Repositories</div>
                  <div className="flex items-center gap-2"><span>🎨</span> Figma Design Tokens</div>
                  <div className="flex items-center gap-2"><span>📦</span> Blender 3D Engine</div>
                  <div className="flex items-center gap-2"><span>🔌</span> Custom MCP (Model Context Protocol)</div>
                </div>
              </div>
            </div>
          </div>
        )}

      </div>
    </div>
  );
};
