import React, { useState } from 'react';
import { 
  Users, 
  Video, 
  VideoOff, 
  Mic, 
  MicOff, 
  Share2, 
  PhoneOff, 
  MessageSquare, 
  Search, 
  Settings, 
  Sparkles, 
  Hand, 
  FileText, 
  Layers, 
  CheckCircle2, 
  Clock, 
  ChevronRight, 
  ShieldCheck, 
  Send, 
  Volume2, 
  VolumeX, 
  Maximize2,
  Radio,
  Sliders,
  Play
} from 'lucide-react';
import lunaImg from '../assets/images/luna_purple_avatar_1790063661430.jpg';
import boardroomImg from '../assets/images/nova_boardroom_1790063694078.jpg';
import userPipImg from '../assets/images/user_webcam_pip_1790061795130.jpg';
import { SubAgent } from '../types';

interface NovaTeamWorkspaceProps {
  subAgents: SubAgent[];
  onOpenVideoModal?: () => void;
}

export const NovaTeamWorkspace: React.FC<NovaTeamWorkspaceProps> = ({
  subAgents,
  onOpenVideoModal
}) => {
  const [activeTab, setActiveTab] = useState<'team' | 'home' | 'projects' | 'knowledge' | 'settings'>('team');
  const [selectedAgentId, setSelectedAgentId] = useState('luna');
  const [isMuted, setIsMuted] = useState(false);
  const [isVideoStopped, setIsVideoStopped] = useState(false);
  const [silentMode, setSilentMode] = useState(false);
  const [autoQueue, setAutoQueue] = useState(true);
  const [pushToTalk, setPushToTalk] = useState(false);
  const [allowReactions, setAllowReactions] = useState(true);
  
  // Luna chat
  const [lunaInput, setLunaInput] = useState('');
  const [lunaMessages, setLunaMessages] = useState([
    { sender: 'user', text: 'Can you adjust the color palette to be more natural and eco-friendly?' },
    { 
      sender: 'luna', 
      text: "Absolutely! I'll update the palette with more earthy tones and sustainable materials. Here's a preview...",
      hasPalette: true
    },
    { sender: 'user', text: 'That looks great. Can you also explore a version with more natural light?' },
    { sender: 'luna', text: "On it! I'll run a few visualizations and share them in a moment." }
  ]);

  const handleSendLunaMessage = (e: React.FormEvent) => {
    e.preventDefault();
    if (!lunaInput.trim()) return;

    setLunaMessages(prev => [...prev, { sender: 'user', text: lunaInput }]);
    const sentText = lunaInput;
    setLunaInput('');

    setTimeout(() => {
      setLunaMessages(prev => [
        ...prev,
        { 
          sender: 'luna', 
          text: `Great thought on "${sentText}"! I've updated the materials canvas and synced the lighting shader with Dev and Atlas.` 
        }
      ]);
    }, 900);
  };

  const team12Members = [
    { id: 'atlas', name: 'Atlas', role: 'Project Lead', avatar: '👨‍💼', color: 'from-amber-500 to-orange-600' },
    { id: 'luna', name: 'Luna', role: 'Design', avatar: '👩‍🎨', image: lunaImg, color: 'from-purple-500 to-indigo-600' },
    { id: 'dev', name: 'Dev', role: 'Development', avatar: '👨‍💻', color: 'from-blue-500 to-cyan-600' },
    { id: 'aria', name: 'Aria', role: 'Research', avatar: '👩‍🔬', color: 'from-emerald-500 to-teal-600' },
    { id: 'sage', name: 'Sage', role: 'Strategy', avatar: '👩‍💼', color: 'from-slate-400 to-slate-600' },
    { id: 'orion', name: 'Orion', role: 'Data', avatar: '👨‍🔧', color: 'from-indigo-500 to-violet-600' },
    { id: 'nova', name: 'Nova', role: 'QA', avatar: '👩‍💻', color: 'from-rose-500 to-pink-600' },
    { id: 'kai', name: 'Kai', role: 'Operations', avatar: '👨‍🚀', color: 'from-amber-600 to-yellow-600' },
    { id: 'zuri', name: 'Zuri', role: 'Marketing', avatar: '👩‍🎤', color: 'from-fuchsia-500 to-purple-600' },
    { id: 'cleo', name: 'Cleo', role: 'Customer Insights', avatar: '👩‍💼', color: 'from-cyan-500 to-blue-600' },
    { id: 'rey', name: 'Rey', role: 'Legal & Compliance', avatar: '👨‍⚖️', color: 'from-stone-500 to-zinc-600' },
    { id: 'echo', name: 'Echo', role: 'Documentation', avatar: '🤖', color: 'from-sky-400 to-blue-500' }
  ];

  return (
    <div className="flex-1 flex flex-col h-full bg-[#080c16] text-slate-100 overflow-y-auto no-scrollbar font-sans selection:bg-purple-500/30">
      
      {/* 1. TOP HEADER: NOVA AI Team Workspace */}
      <header className="h-16 px-4 sm:px-6 bg-[#091020]/95 backdrop-blur-md border-b border-slate-800/80 flex items-center justify-between shrink-0 sticky top-0 z-20">
        <div className="flex items-center gap-4">
          <div className="flex items-center gap-2.5">
            <div className="w-9 h-9 rounded-xl bg-gradient-to-tr from-cyan-400 via-indigo-500 to-purple-500 p-[1px] shadow-lg shadow-cyan-500/20">
              <div className="w-full h-full bg-[#080c16] rounded-[11px] flex items-center justify-center font-black text-transparent bg-clip-text bg-gradient-to-r from-cyan-300 to-purple-300 text-base">
                ✦
              </div>
            </div>
            <div>
              <span className="font-black tracking-tight text-white text-base">NOVA</span>
              <span className="text-[11px] text-slate-400 ml-2 hidden sm:inline">Your AI Team. In Real Time.</span>
            </div>
          </div>

          {/* Navigation */}
          <nav className="hidden md:flex items-center gap-1 bg-[#050811] p-1 rounded-xl border border-slate-800 text-xs">
            {(['home', 'team', 'projects', 'knowledge', 'settings'] as const).map(tab => (
              <button
                key={tab}
                onClick={() => setActiveTab(tab)}
                className={`px-3 py-1.5 rounded-lg capitalize font-semibold transition-all ${
                  activeTab === tab
                    ? 'bg-gradient-to-r from-cyan-600 to-indigo-600 text-white shadow-sm'
                    : 'text-slate-400 hover:text-slate-200'
                }`}
              >
                {tab}
              </button>
            ))}
          </nav>
        </div>

        {/* Right Search & Profile */}
        <div className="flex items-center gap-3">
          <div className="relative hidden lg:block">
            <Search className="w-3.5 h-3.5 absolute left-3 top-2.5 text-slate-500" />
            <input
              type="text"
              placeholder="Search or ask anything..."
              className="pl-8 pr-3 py-1.5 rounded-xl bg-[#050811] border border-slate-800 text-xs text-slate-200 placeholder:text-slate-500 focus:outline-none focus:border-cyan-500 w-60"
            />
          </div>

          <div className="flex items-center gap-2 pl-2 border-l border-slate-800">
            <img src={userPipImg} alt="You" className="w-8 h-8 rounded-full object-cover ring-2 ring-indigo-500" />
            <div className="hidden sm:block text-left text-xs">
              <div className="font-bold text-slate-100 flex items-center gap-1">
                You <span className="text-[9px] px-1 rounded bg-indigo-500/30 text-indigo-300 font-mono">Pro</span>
              </div>
            </div>
          </div>
        </div>
      </header>

      {/* 2. TOP SPLIT: BOARDROOM TABLE (Left) + 1-ON-1 LUNA TALKING (Right) */}
      <div className="p-3 grid grid-cols-1 xl:grid-cols-12 gap-3 min-w-0">
        
        {/* TOP LEFT: HOLOGRAPHIC BOARDROOM TABLE (7 Cols) */}
        <div className="xl:col-span-7 bg-[#070b14] rounded-2xl border border-slate-800/90 overflow-hidden flex flex-col justify-between shadow-2xl relative min-h-[380px]">
          {/* Background Holographic Boardroom Scene */}
          <div className="relative flex-1 overflow-hidden min-h-[300px]">
            <img
              src={boardroomImg}
              alt="Holographic Boardroom"
              className="w-full h-full object-cover filter brightness-95"
            />
            <div className="absolute inset-0 bg-gradient-to-t from-[#070b14] via-transparent to-black/60" />

            {/* Top Motto */}
            <div className="absolute top-4 left-4 max-w-sm text-left drop-shadow-md">
              <h2 className="text-lg font-black text-white leading-tight">
                Ideas<br />People<br />Progress<br />Together.
              </h2>
              <p className="text-[11px] text-slate-300 mt-1">A more capable you, with a team by your side.</p>
            </div>

            <div className="absolute top-4 right-4 text-right text-xs text-slate-300 italic hidden sm:block">
              "Different minds.<br />A brighter future."
            </div>

            {/* Project Card: EcoHome Redesign */}
            <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-72 p-3 rounded-2xl bg-[#091020]/90 backdrop-blur-md border border-cyan-500/40 text-center shadow-2xl">
              <div className="text-xs font-bold text-white flex items-center justify-center gap-1.5">
                <span className="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" />
                <span>Project: EcoHome Redesign</span>
              </div>
              <div className="text-[10px] text-slate-400 mt-0.5">Planning • Research • Design • Build</div>
              
              <div className="mt-2 flex items-center justify-between text-[10px] text-cyan-300 font-mono">
                <span>Progress</span>
                <span>62%</span>
              </div>
              <div className="w-full h-1.5 bg-slate-800 rounded-full overflow-hidden mt-0.5">
                <div className="h-full bg-gradient-to-r from-cyan-400 to-indigo-500 rounded-full w-[62%]" />
              </div>
            </div>

            {/* Avatar Pill Strip around Table */}
            <div className="absolute bottom-16 left-3 right-3 flex items-center justify-around flex-wrap gap-1">
              {[
                { name: 'Aria', role: 'Research' },
                { name: 'Dev', role: 'Development' },
                { name: 'Sage', role: 'Strategy' },
                { name: 'Atlas', role: 'Project Lead' },
                { name: 'Luna', role: 'Design' },
                { name: 'Orion', role: 'Data' },
                { name: 'Nova', role: 'QA' },
                { name: 'Kai', role: 'Operations' }
              ].map(agent => (
                <div key={agent.name} className="px-2 py-1 rounded-xl bg-black/70 backdrop-blur-md border border-white/10 text-center text-[10px]">
                  <div className="font-bold text-white">{agent.name}</div>
                  <div className="text-[9px] text-slate-400">{agent.role}</div>
                </div>
              ))}
            </div>
          </div>

          {/* Bottom Control Bar */}
          <div className="p-3 bg-[#080d19] border-t border-slate-800/80 flex items-center justify-between">
            <div className="flex items-center gap-2 text-xs text-emerald-400">
              <span className="w-2.5 h-2.5 rounded-full bg-emerald-400 animate-pulse" />
              <span>Team is collaborating... 3 agents are discussing design options.</span>
            </div>

            <div className="flex items-center gap-2">
              <button 
                onClick={() => setIsMuted(!isMuted)}
                className="px-2.5 py-1.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-xs font-semibold text-slate-300 transition-colors flex items-center gap-1.5"
              >
                {isMuted ? <MicOff className="w-3.5 h-3.5 text-rose-400" /> : <Mic className="w-3.5 h-3.5" />}
                <span>{isMuted ? 'Unmute All' : 'Mute All'}</span>
              </button>

              <button className="px-2.5 py-1.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-xs font-semibold text-slate-300 transition-colors flex items-center gap-1.5">
                <MessageSquare className="w-3.5 h-3.5" />
                <span>Team Chat</span>
              </button>

              <button className="px-2.5 py-1.5 rounded-xl bg-indigo-600/30 text-indigo-300 border border-indigo-500/40 text-xs font-semibold transition-colors flex items-center gap-1.5">
                <Share2 className="w-3.5 h-3.5" />
                <span>Present</span>
              </button>

              <button className="px-3 py-1.5 rounded-xl bg-rose-600 hover:bg-rose-500 text-xs font-bold text-white transition-colors">
                End Session
              </button>
            </div>
          </div>
        </div>

        {/* TOP RIGHT: TALKING WITH LUNA 1-ON-1 (5 Cols) */}
        <div className="xl:col-span-5 bg-[#070b14] rounded-2xl border border-slate-800/90 overflow-hidden flex flex-col justify-between shadow-2xl">
          
          {/* Header */}
          <div className="p-3 bg-[#080d19] border-b border-slate-800/80 flex items-center justify-between">
            <div className="flex items-center gap-2.5">
              <div className="relative">
                <img src={lunaImg} alt="Luna" className="w-8 h-8 rounded-full object-cover ring-2 ring-purple-500" />
                <span className="absolute bottom-0 right-0 w-2 h-2 rounded-full bg-emerald-400 ring-1 ring-black" />
              </div>
              <div>
                <div className="text-xs font-bold text-white flex items-center gap-1.5">
                  Talking with Luna
                </div>
                <div className="text-[10px] text-purple-400">Design Specialist • Creative Strategy</div>
              </div>
            </div>

            <div className="flex items-center gap-1.5">
              <span className="text-[10px] px-2 py-0.5 rounded-full bg-emerald-500/20 text-emerald-400 border border-emerald-500/30 flex items-center gap-1">
                <span className="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse" />
                Online
              </span>
            </div>
          </div>

          {/* Split Body: Sidebar Agents + Video & Chat */}
          <div className="flex-1 flex overflow-hidden">
            {/* Left Teammates Rail */}
            <div className="w-20 shrink-0 bg-[#060912] border-r border-slate-800/80 p-1.5 space-y-1 overflow-y-auto no-scrollbar">
              {[
                { id: 'luna', name: 'Luna', role: 'Design', active: true },
                { id: 'atlas', name: 'Atlas', role: 'Lead' },
                { id: 'aria', name: 'Aria', role: 'Research', badge: 'In a call' },
                { id: 'dev', name: 'Dev', role: 'Dev' },
                { id: 'sage', name: 'Sage', role: 'Strategy' },
                { id: 'orion', name: 'Orion', role: 'Data' },
                { id: 'nova', name: 'Nova', role: 'QA', badge: 'Reviewing...' },
                { id: 'kai', name: 'Kai', role: 'Ops' }
              ].map(agent => (
                <button
                  key={agent.id}
                  onClick={() => setSelectedAgentId(agent.id)}
                  className={`w-full p-1.5 rounded-xl text-left transition-all ${
                    selectedAgentId === agent.id
                      ? 'bg-purple-950/80 border border-purple-500/50 text-white'
                      : 'hover:bg-slate-900/60 text-slate-400'
                  }`}
                >
                  <div className="text-[11px] font-bold truncate">{agent.name}</div>
                  <div className="text-[9px] text-slate-500 truncate">{agent.role}</div>
                  {agent.badge && (
                    <div className="text-[8px] text-emerald-400 truncate">{agent.badge}</div>
                  )}
                </button>
              ))}
            </div>

            {/* Right: Large Luna Video & Live Chat */}
            <div className="flex-1 flex flex-col justify-between bg-slate-950/40 p-2.5 overflow-y-auto no-scrollbar">
              
              {/* Luna Video Call Frame */}
              <div className="relative w-full h-44 rounded-2xl overflow-hidden ring-2 ring-purple-500/30 shadow-xl shrink-0">
                <img src={lunaImg} alt="Luna Video" className="w-full h-full object-cover object-top" />
                <div className="absolute inset-0 bg-gradient-to-t from-black/80 via-transparent to-transparent" />
                
                {/* Overlay Controls */}
                <div className="absolute bottom-2 left-2 right-2 flex items-center justify-between text-xs">
                  <div className="flex items-center gap-1 bg-black/60 px-2 py-0.5 rounded-lg text-[10px] text-white">
                    <span className="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse" />
                    <span>Luna • Active Video</span>
                  </div>

                  <div className="flex items-center gap-1">
                    <button 
                      onClick={() => setIsMuted(!isMuted)} 
                      className="p-1 rounded-lg bg-black/60 text-white hover:bg-slate-800"
                    >
                      {isMuted ? <MicOff className="w-3 h-3 text-rose-400" /> : <Mic className="w-3 h-3" />}
                    </button>
                    <button 
                      onClick={() => setIsVideoStopped(!isVideoStopped)} 
                      className="p-1 rounded-lg bg-black/60 text-white hover:bg-slate-800"
                    >
                      {isVideoStopped ? <VideoOff className="w-3 h-3 text-rose-400" /> : <Video className="w-3 h-3" />}
                    </button>
                    <button className="px-2 py-1 rounded-lg bg-rose-600 text-white font-bold text-[10px]">
                      End Chat
                    </button>
                  </div>
                </div>
              </div>

              {/* Chat Stream with Palette Preview */}
              <div className="space-y-2 mt-2 text-xs">
                {lunaMessages.map((msg, i) => (
                  <div
                    key={i}
                    className={`p-2.5 rounded-xl ${
                      msg.sender === 'user'
                        ? 'bg-purple-950/70 text-purple-100 border border-purple-800/40 ml-auto max-w-[90%]'
                        : 'bg-slate-900/90 text-slate-100 border border-slate-800 mr-auto max-w-[95%]'
                    }`}
                  >
                    <div className="text-[9px] text-slate-400 font-mono mb-0.5">
                      {msg.sender === 'user' ? 'You' : 'Luna'}
                    </div>
                    <div>{msg.text}</div>

                    {msg.hasPalette && (
                      <div className="mt-2 p-2 rounded-lg bg-black/60 border border-slate-800 flex items-center gap-2">
                        <div className="flex items-center gap-1.5">
                          <div className="w-6 h-6 rounded-md bg-[#3e5641]" title="Sage Forest" />
                          <div className="w-6 h-6 rounded-md bg-[#a39171]" title="Sandstone" />
                          <div className="w-6 h-6 rounded-md bg-[#b87d4b]" title="Warm Terracotta" />
                          <div className="w-6 h-6 rounded-md bg-[#ded3c3]" title="Natural Linen" />
                        </div>
                        <span className="text-[10px] text-slate-300 font-mono">Earthy Sustainable Palette</span>
                      </div>
                    )}
                  </div>
                ))}
              </div>

              {/* Input */}
              <form onSubmit={handleSendLunaMessage} className="mt-2 flex items-center gap-1.5">
                <input
                  type="text"
                  value={lunaInput}
                  onChange={(e) => setLunaInput(e.target.value)}
                  placeholder="Type a message..."
                  className="flex-1 px-3 py-1.5 rounded-xl bg-[#050811] border border-slate-800 text-xs text-white placeholder:text-slate-500 focus:outline-none focus:border-purple-500"
                />
                <button
                  type="submit"
                  className="p-1.5 rounded-xl bg-purple-600 hover:bg-purple-500 text-white transition-colors"
                >
                  <Send className="w-3.5 h-3.5" />
                </button>
              </form>
            </div>
          </div>
        </div>
      </div>

      {/* 3. BOTTOM SPLIT: 12-PERSON ALL-HANDS GRID (Left) + CONVERSATION QUEUE & INTER-AGENT LOGS (Right) */}
      <div className="p-3 pt-0 grid grid-cols-1 xl:grid-cols-12 gap-3 min-w-0">
        
        {/* BOTTOM LEFT: TEAM MEETING - ALL HANDS - LIVE (8 Cols) */}
        <div className="xl:col-span-8 bg-[#070b14] rounded-2xl border border-slate-800/90 p-3 flex flex-col justify-between shadow-2xl">
          <div>
            <div className="flex items-center justify-between mb-3">
              <div className="flex items-center gap-2">
                <div className="p-1 rounded-lg bg-rose-500/20 text-rose-400">
                  <Radio className="w-4 h-4 animate-pulse" />
                </div>
                <div>
                  <h3 className="text-xs font-bold text-white flex items-center gap-1.5">
                    <span>Team Meeting - All Hands</span>
                    <span className="px-1.5 py-0.2 rounded-full bg-rose-500 text-white font-bold text-[9px] uppercase">LIVE</span>
                  </h3>
                  <p className="text-[10px] text-slate-400">12/12 team members | Coordinating next steps</p>
                </div>
              </div>

              <div className="text-[11px] text-slate-400 italic hidden sm:block">
                "Great things happen when everyone has a voice."
              </div>
            </div>

            {/* 12-Video Call Grid */}
            <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-2">
              {team12Members.map((member) => (
                <div
                  key={member.id}
                  className="relative h-28 rounded-xl bg-slate-900 overflow-hidden border border-slate-800 group hover:border-cyan-500/50 transition-all flex flex-col justify-between p-2 shadow-inner"
                >
                  {member.image ? (
                    <img src={member.image} alt={member.name} className="absolute inset-0 w-full h-full object-cover filter brightness-90 group-hover:scale-105 transition-transform" />
                  ) : (
                    <div className={`absolute inset-0 bg-gradient-to-tr ${member.color} opacity-20 flex items-center justify-center text-3xl`}>
                      {member.avatar}
                    </div>
                  )}

                  <div className="absolute inset-0 bg-gradient-to-t from-black/80 via-transparent to-black/30" />

                  {/* Top Status */}
                  <div className="relative z-10 flex items-center justify-between text-[10px]">
                    <span className="px-1.5 py-0.2 rounded bg-black/60 text-white font-mono text-[9px]">
                      HD
                    </span>
                    <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
                  </div>

                  {/* Bottom Info */}
                  <div className="relative z-10 text-left">
                    <div className="text-xs font-bold text-white drop-shadow truncate">{member.name}</div>
                    <div className="text-[9px] text-cyan-300 drop-shadow truncate">{member.role}</div>
                  </div>
                </div>
              ))}
            </div>
          </div>

          {/* Meeting Controls Bar */}
          <div className="mt-3 pt-2.5 border-t border-slate-800/80 flex items-center justify-between flex-wrap gap-2 text-xs">
            <div className="flex items-center gap-1.5">
              <button 
                onClick={() => setIsMuted(!isMuted)}
                className="px-2.5 py-1.5 rounded-xl bg-slate-900 hover:bg-slate-800 text-slate-300 hover:text-white flex items-center gap-1 font-semibold"
              >
                {isMuted ? <MicOff className="w-3.5 h-3.5 text-rose-400" /> : <Mic className="w-3.5 h-3.5" />}
                <span>{isMuted ? 'Unmute' : 'Mute'}</span>
              </button>

              <button 
                onClick={() => setIsVideoStopped(!isVideoStopped)}
                className="px-2.5 py-1.5 rounded-xl bg-slate-900 hover:bg-slate-800 text-slate-300 hover:text-white flex items-center gap-1 font-semibold"
              >
                {isVideoStopped ? <VideoOff className="w-3.5 h-3.5 text-rose-400" /> : <Video className="w-3.5 h-3.5" />}
                <span>Video</span>
              </button>

              <button className="px-2.5 py-1.5 rounded-xl bg-slate-900 hover:bg-slate-800 text-slate-300 hover:text-white flex items-center gap-1 font-semibold">
                <Hand className="w-3.5 h-3.5 text-amber-400" />
                <span>Raise Hand</span>
              </button>

              <button className="px-2.5 py-1.5 rounded-xl bg-slate-900 hover:bg-slate-800 text-slate-300 hover:text-white flex items-center gap-1 font-semibold">
                <FileText className="w-3.5 h-3.5" />
                <span>Add Note</span>
              </button>

              <button className="px-2.5 py-1.5 rounded-xl bg-slate-900 hover:bg-slate-800 text-slate-300 hover:text-white flex items-center gap-1 font-semibold">
                <MessageSquare className="w-3.5 h-3.5" />
                <span>View Chat</span>
              </button>
            </div>

            <button className="px-4 py-1.5 rounded-xl bg-rose-600 hover:bg-rose-500 text-white font-bold text-xs shadow-md shadow-rose-600/30">
              Leave
            </button>
          </div>
        </div>

        {/* BOTTOM RIGHT: CONVERSATION QUEUE & INTER-AGENT LOGS (4 Cols) */}
        <div className="xl:col-span-4 space-y-3">
          
          {/* Conversation Queue Card */}
          <div className="p-3.5 rounded-2xl bg-[#070b14] border border-slate-800/90 shadow-xl space-y-2.5">
            <div className="flex items-center justify-between">
              <h4 className="text-xs font-bold text-white">Conversation Queue</h4>
              <div className="flex items-center gap-1 text-[10px] text-slate-400">
                <span>Auto-moderation:</span>
                <span className="text-emerald-400 font-bold">ON</span>
              </div>
            </div>

            <div className="space-y-1 text-xs">
              <div className="p-1.5 rounded-xl bg-indigo-950/60 border border-indigo-500/40 flex items-center justify-between font-semibold">
                <span className="text-indigo-300">1. You (Next to speak)</span>
                <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
              </div>
              <div className="p-1 rounded-lg bg-slate-900/60 text-slate-400 flex items-center justify-between text-[11px]">
                <span>2. Atlas</span>
                <span>Ready</span>
              </div>
              <div className="p-1 rounded-lg bg-slate-900/60 text-slate-400 flex items-center justify-between text-[11px]">
                <span>3. Zuri</span>
                <span>Queued</span>
              </div>
              <div className="p-1 rounded-lg bg-slate-900/60 text-slate-400 flex items-center justify-between text-[11px]">
                <span>4. Kai</span>
                <span>Queued</span>
              </div>
            </div>

            <button className="w-full py-1.5 rounded-xl bg-gradient-to-r from-cyan-600 to-indigo-600 text-white text-xs font-bold shadow-md shadow-cyan-600/30">
              Request to Speak
            </button>
          </div>

          {/* Inter-Agent Messages (Live) Card */}
          <div className="p-3.5 rounded-2xl bg-[#070b14] border border-slate-800/90 shadow-xl space-y-2">
            <div className="flex items-center justify-between">
              <h4 className="text-xs font-bold text-white flex items-center gap-1.5">
                <span>Inter-Agent Messages</span>
                <span className="text-[9px] px-1.5 py-0.2 rounded bg-cyan-500/20 text-cyan-300 font-mono">Live</span>
              </h4>
            </div>

            <div className="space-y-1.5 text-xs">
              <div className="p-2 rounded-xl bg-slate-900/80 border border-slate-800">
                <div className="flex items-center justify-between text-[10px] text-slate-400">
                  <span className="font-bold text-emerald-400">Aria → Dev</span>
                  <span>10:14 AM</span>
                </div>
                <p className="text-[11px] text-slate-300 mt-0.5">Found a new dataset that might be useful.</p>
              </div>

              <div className="p-2 rounded-xl bg-slate-900/80 border border-slate-800">
                <div className="flex items-center justify-between text-[10px] text-slate-400">
                  <span className="font-bold text-blue-400">Dev → Aria</span>
                  <span>10:15 AM</span>
                </div>
                <p className="text-[11px] text-slate-300 mt-0.5">Great! I'll integrate it now.</p>
              </div>

              <div className="p-2 rounded-xl bg-slate-900/80 border border-slate-800">
                <div className="flex items-center justify-between text-[10px] text-slate-400">
                  <span className="font-bold text-purple-400">Luna → Nova</span>
                  <span>10:16 AM</span>
                </div>
                <p className="text-[11px] text-slate-300 mt-0.5">Here are the updated visuals for review.</p>
              </div>

              <div className="p-2 rounded-xl bg-slate-900/80 border border-slate-800">
                <div className="flex items-center justify-between text-[10px] text-slate-400">
                  <span className="font-bold text-rose-400">Nova → Luna</span>
                  <span>10:17 AM</span>
                </div>
                <p className="text-[11px] text-slate-300 mt-0.5">They look amazing! Just one small tweak...</p>
              </div>
            </div>
          </div>

          {/* Individual Agent View: Sage */}
          <div className="p-3.5 rounded-2xl bg-[#070b14] border border-slate-800/90 shadow-xl space-y-2">
            <div className="flex items-center justify-between">
              <div>
                <div className="text-xs font-bold text-white">Sage</div>
                <div className="text-[10px] text-slate-400">Strategy Specialist</div>
              </div>
              <button className="px-2.5 py-1 rounded-xl bg-slate-800 hover:bg-slate-700 text-xs font-semibold text-slate-200">
                Talk to Sage
              </button>
            </div>

            <div className="p-2 rounded-xl bg-slate-900/60 text-[11px] text-slate-300 space-y-1">
              <div><strong className="text-slate-400">Current Task:</strong> Refining go-to-market strategy</div>
              <div><strong className="text-slate-400">Recent:</strong> Analyzed 3 market reports, generated strategic recommendations</div>
            </div>
          </div>

        </div>

      </div>

    </div>
  );
};
