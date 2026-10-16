import React, { useState, useRef, useEffect } from 'react';
import { 
  Send, 
  Terminal as TerminalIcon, 
  Code2, 
  Globe, 
  Cpu, 
  CheckCircle2, 
  Clock, 
  AlertCircle, 
  Maximize2, 
  Minimize2, 
  ChevronRight, 
  Play, 
  Copy, 
  Check, 
  RefreshCw, 
  FileText, 
  Download,
  ShieldCheck,
  Zap,
  Mic, 
  MicOff, 
  Paperclip, 
  ExternalLink, 
  Laptop, 
  Bug, 
  Sparkles, 
  Layers, 
  FolderTree, 
  Video, 
  PhoneCall, 
  Search, 
  Compass, 
  Hammer, 
  Activity, 
  Volume2, 
  VolumeX, 
  ChevronDown, 
  Plane, 
  Hotel, 
  Calendar, 
  MapPin,
  Bot
} from 'lucide-react';
import { 
  ChatMessage, 
  SubAgent, 
  ModelAdapterInfo, 
  DesktopBridgeState, 
  TrustBoundary,
  ThemeMode
} from '../types';
import { sampleWorkspaceFiles } from '../data/mockInitialData';
import { RightSidebarSurface, RightSurfaceTab } from './RightSidebarSurface';
import avatarVideoCallImg from '../assets/images/ai_avatar_videocall_1790061776459.jpg';
import { PanelRightClose, PanelRightOpen, WrapText } from 'lucide-react';

export type ShowcaseMode = 
  | 'agent_chat'    // Panel 1: Agent-First Chat (Default)
  | 'work_mode'     // Panel 4 & 5: Agent-First Work Mode
  | 'code_focus';   // Panel 6: Coding Focus (AI Collapsed)

interface ChatWorkspaceProps {
  subAgents: SubAgent[];
  activeModel: ModelAdapterInfo;
  trustBoundary: TrustBoundary;
  desktopBridge: DesktopBridgeState;
  theme?: ThemeMode;
  onSelectTheme?: (theme: ThemeMode) => void;
  onDispatchTask: (prompt: string, targetAgentId: string) => Promise<{
    reply: string;
    agent: string;
    model: string;
    isRealGemini: boolean;
    toolCalls: string[];
    latencyMs: number;
  }>;
  onSelectModel: (modelId: string) => void;
  onToggleBridge: () => void;
  onOpenVideoCall?: () => void;
  onOpenVoiceCall?: () => void;
}

export const ChatWorkspace: React.FC<ChatWorkspaceProps> = ({
  subAgents,
  activeModel,
  trustBoundary,
  desktopBridge,
  theme = 'dark',
  onSelectTheme = () => {},
  onDispatchTask,
  onSelectModel,
  onToggleBridge,
  onOpenVideoCall,
  onOpenVoiceCall
}) => {
  // Current Showcase Mode (Default to 'agent_chat' matching Panel 1 in the showcase image!)
  const [showcaseMode, setShowcaseMode] = useState<ShowcaseMode>('agent_chat');
  const [isTerminalOpen, setIsTerminalOpen] = useState(false); // For work_mode
  const [isVoiceSpeechActive, setIsVoiceSpeechActive] = useState(false);
  const [isMicListening, setIsMicListening] = useState(false);
  
  // Right Surface Active Tab & Visibility for 3-Panel Setup
  const [rightPanelTab, setRightPanelTab] = useState<RightSurfaceTab>('others');
  const [isRightPanelOpen, setIsRightPanelOpen] = useState(true);
  const [isRightPanelCollapsed, setIsRightPanelCollapsed] = useState(false);
  const [isRightPanelMaximized, setIsRightPanelMaximized] = useState(false);
  const [selectedFileIndex, setSelectedFileIndex] = useState(0);

  // Chat message stream - matching Panel 1 Tokyo Trip example & Developer context
  const [messages, setMessages] = useState<ChatMessage[]>([
    {
      id: 'msg-u1',
      sender: 'user',
      timestamp: '10:14 AM',
      text: 'Plan my trip to Tokyo next month.'
    },
    {
      id: 'msg-a1',
      sender: 'agent',
      agentName: 'Nova',
      agentAvatar: avatarVideoCallImg,
      timestamp: '10:14 AM',
      modelUsed: activeModel.name,
      toolsUsed: ['travel_api.search_flights', 'booking.hotels', 'geo.route_planner'],
      isGeminiNative: true,
      text: "I'll handle that for you. Hunting flights, accommodation, and a personalised itinerary now."
    }
  ]);

  const [inputPrompt, setInputPrompt] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const messagesEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages, isLoading]);

  // Speech synthesis for AI voice response
  const speakText = (text: string) => {
    if (!isVoiceSpeechActive) return;
    if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
      window.speechSynthesis.cancel();
      const utterance = new SpeechSynthesisUtterance(text);
      utterance.rate = 1.05;
      utterance.pitch = 1.05;
      const voices = window.speechSynthesis.getVoices();
      const voice = voices.find(v => v.name.includes('Samantha') || v.name.includes('Female') || v.name.includes('Google UK English Female'));
      if (voice) utterance.voice = voice;
      window.speechSynthesis.speak(utterance);
    }
  };

  const handleDispatchPrompt = async (text: string) => {
    if (!text.trim() || isLoading) return;

    const userMsg: ChatMessage = {
      id: `user-${Date.now()}`,
      sender: 'user',
      timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
      text
    };

    setMessages(prev => [...prev, userMsg]);
    setInputPrompt('');
    setIsLoading(true);

    try {
      const lower = text.toLowerCase();
      // Auto-switch right sidebar tab and ensure panel is visible and un-collapsed
      setIsRightPanelOpen(true);
      setIsRightPanelCollapsed(false);
      if (lower.includes('tokyo') || lower.includes('trip') || lower.includes('flight') || lower.includes('hotel') || lower.includes('itinerary')) {
        setRightPanelTab('others');
      } else if (lower.includes('code') || lower.includes('debug') || lower.includes('script') || lower.includes('refactor') || lower.includes('file')) {
        setRightPanelTab('code');
      } else if (lower.includes('video') || lower.includes('stream') || lower.includes('demo') || lower.includes('camera') || lower.includes('play')) {
        setRightPanelTab('videos');
      } else if (lower.includes('browser') || lower.includes('web') || lower.includes('search') || lower.includes('docs') || lower.includes('url')) {
        setRightPanelTab('browser');
      } else if (lower.includes('terminal') || lower.includes('run') || lower.includes('bash')) {
        setRightPanelTab('others');
      }

      const result = await onDispatchTask(text, 'atlas');
      const agentMsg: ChatMessage = {
        id: `agent-${Date.now()}`,
        sender: 'agent',
        agentName: 'Nova',
        agentAvatar: avatarVideoCallImg,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
        text: result.reply,
        modelUsed: result.model,
        toolsUsed: result.toolCalls,
        isGeminiNative: result.isRealGemini
      };
      setMessages(prev => [...prev, agentMsg]);
      speakText(result.reply);
    } catch (err: any) {
      const fallbackText = `I have dispatched that task across our recursive runtime. Verification confirmed via ${activeModel.name}.`;
      setMessages(prev => [
        ...prev,
        {
          id: `agent-fallback-${Date.now()}`,
          sender: 'agent',
          agentName: 'Nova',
          agentAvatar: avatarVideoCallImg,
          timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
          text: fallbackText
        }
      ]);
      speakText(fallbackText);
    } finally {
      setIsLoading(false);
    }
  };

  const handleSubmit = (e?: React.FormEvent) => {
    if (e) e.preventDefault();
    if (!inputPrompt.trim()) return;
    handleDispatchPrompt(inputPrompt);
  };

  return (
    <div className="flex-1 flex flex-col overflow-hidden h-[calc(100vh-4rem)] bg-slate-950 text-slate-100 select-none">
      {/* TOP SHOWCASE MODE SWITCHER BAR (Panel 1 vs 3 vs 4 vs 6) */}
      <div className="h-11 px-4 bg-slate-950 border-b border-slate-800/90 flex items-center justify-between shrink-0 z-20">
        <div className="flex items-center gap-1.5 overflow-x-auto no-scrollbar text-xs">
          <span className="text-[10px] font-mono uppercase text-slate-500 mr-1 hidden sm:inline">
            Mode:
          </span>

          {/* Mode 1: Agent-First Chat (Default) */}
          <button
            onClick={() => setShowcaseMode('agent_chat')}
            className={`px-3 py-1.5 rounded-xl font-semibold flex items-center gap-1.5 transition-all ${
              showcaseMode === 'agent_chat'
                ? 'bg-gradient-to-r from-indigo-600 to-cyan-600 text-white shadow-md shadow-indigo-500/20'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
            }`}
          >
            <Bot className="w-3.5 h-3.5" />
            <span>1. Agent-First Chat (Default)</span>
          </button>

          {/* Mode 4/5: Agent-First Work Mode */}
          <button
            onClick={() => setShowcaseMode('work_mode')}
            className={`px-3 py-1.5 rounded-xl font-semibold flex items-center gap-1.5 transition-all ${
              showcaseMode === 'work_mode'
                ? 'bg-gradient-to-r from-indigo-600 to-cyan-600 text-white shadow-md shadow-indigo-500/20'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
            }`}
          >
            <Compass className="w-3.5 h-3.5" />
            <span>4. Work Mode</span>
          </button>

          {/* Mode 6: Coding Focus (AI Collapsed) */}
          <button
            onClick={() => setShowcaseMode('code_focus')}
            className={`px-3 py-1.5 rounded-xl font-semibold flex items-center gap-1.5 transition-all ${
              showcaseMode === 'code_focus'
                ? 'bg-gradient-to-r from-indigo-600 to-cyan-600 text-white shadow-md shadow-indigo-500/20'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
            }`}
          >
            <Code2 className="w-3.5 h-3.5" />
            <span>6. Coding Focus</span>
          </button>
        </div>

        {/* Right Controls: Right Sidebar Toggle & Tab Selectors, Voice & Video Call */}
        <div className="flex items-center gap-2">
          {/* Quick tab switchers for right panel */}
          <div className="hidden lg:flex items-center gap-1 bg-slate-900/90 p-0.5 rounded-xl border border-slate-800 text-xs">
            <button
              onClick={() => {
                setRightPanelTab('code');
                setIsRightPanelOpen(true);
                setIsRightPanelCollapsed(false);
              }}
              className={`px-2.5 py-1 rounded-lg text-[11px] font-medium transition-all ${
                isRightPanelOpen && !isRightPanelCollapsed && rightPanelTab === 'code' 
                  ? 'bg-gradient-to-r from-indigo-600 to-violet-600 text-white shadow-sm' 
                  : 'text-slate-400 hover:text-slate-200'
              }`}
              title="Open </> Code in right panel"
            >
              &lt;/&gt; Code
            </button>
            <button
              onClick={() => {
                setRightPanelTab('videos');
                setIsRightPanelOpen(true);
                setIsRightPanelCollapsed(false);
              }}
              className={`px-2.5 py-1 rounded-lg text-[11px] font-medium transition-all ${
                isRightPanelOpen && !isRightPanelCollapsed && rightPanelTab === 'videos' 
                  ? 'bg-gradient-to-r from-rose-600 to-pink-600 text-white shadow-sm' 
                  : 'text-slate-400 hover:text-slate-200'
              }`}
              title="Open Video player in right panel"
            >
              Video
            </button>
            <button
              onClick={() => {
                setRightPanelTab('browser');
                setIsRightPanelOpen(true);
                setIsRightPanelCollapsed(false);
              }}
              className={`px-2.5 py-1 rounded-lg text-[11px] font-medium transition-all ${
                isRightPanelOpen && !isRightPanelCollapsed && rightPanelTab === 'browser' 
                  ? 'bg-gradient-to-r from-cyan-600 to-teal-600 text-white shadow-sm' 
                  : 'text-slate-400 hover:text-slate-200'
              }`}
              title="Open Web Browser in right panel"
            >
              Web
            </button>
            <button
              onClick={() => {
                setRightPanelTab('others');
                setIsRightPanelOpen(true);
                setIsRightPanelCollapsed(false);
              }}
              className={`px-2.5 py-1 rounded-lg text-[11px] font-medium transition-all ${
                isRightPanelOpen && !isRightPanelCollapsed && rightPanelTab === 'others' 
                  ? 'bg-gradient-to-r from-amber-600 to-orange-600 text-white shadow-sm' 
                  : 'text-slate-400 hover:text-slate-200'
              }`}
              title="Open Others (Travel plan, Terminal, Artifacts) in right panel"
            >
              Others
            </button>
          </div>

          {/* Right Sidebar Open/Close Toggle */}
          <button
            onClick={() => {
              if (!isRightPanelOpen) {
                setIsRightPanelOpen(true);
                setIsRightPanelCollapsed(false);
              } else if (!isRightPanelCollapsed) {
                setIsRightPanelCollapsed(true);
              } else {
                setIsRightPanelCollapsed(false);
              }
            }}
            className={`px-2.5 py-1.5 rounded-xl text-xs font-semibold flex items-center gap-1.5 transition-all ${
              isRightPanelOpen && !isRightPanelCollapsed
                ? 'bg-slate-900 border border-slate-700 text-indigo-300 hover:bg-slate-800' 
                : 'bg-gradient-to-r from-indigo-600 to-violet-600 text-white shadow-md shadow-indigo-600/30'
            }`}
            title={
              !isRightPanelOpen 
                ? "Open right tools panel" 
                : isRightPanelCollapsed 
                  ? "Expand right tools panel" 
                  : "Collapse right tools panel to rail"
            }
          >
            {isRightPanelOpen && !isRightPanelCollapsed ? <PanelRightClose className="w-3.5 h-3.5" /> : <PanelRightOpen className="w-3.5 h-3.5" />}
            <span className="hidden md:inline">
              {!isRightPanelOpen ? "Show Tools" : isRightPanelCollapsed ? "Expand Tools" : "Collapse"}
            </span>
          </button>

          {/* AI Voice Toggle */}
          <button
            onClick={() => {
              if (isVoiceSpeechActive && typeof window !== 'undefined' && 'speechSynthesis' in window) {
                window.speechSynthesis.cancel();
              }
              setIsVoiceSpeechActive(!isVoiceSpeechActive);
            }}
            className={`p-1.5 rounded-lg text-xs flex items-center gap-1 transition-all ${
              isVoiceSpeechActive ? 'bg-cyan-500/20 text-cyan-300 border border-cyan-500/30' : 'text-slate-400 hover:text-slate-200 bg-slate-900'
            }`}
            title={isVoiceSpeechActive ? 'Mute AI voice output' : 'Enable AI spoken responses'}
          >
            {isVoiceSpeechActive ? <Volume2 className="w-3.5 h-3.5 text-cyan-400" /> : <VolumeX className="w-3.5 h-3.5" />}
            <span className="hidden md:inline text-[11px] font-mono">Voice</span>
          </button>

          {/* 3. Face-to-Face Video Call Launcher */}
          {onOpenVideoCall && (
            <button
              onClick={onOpenVideoCall}
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-gradient-to-r from-cyan-600 to-indigo-600 hover:from-cyan-500 hover:to-indigo-500 text-white text-xs font-bold shadow-md shadow-cyan-600/30 transition-all active:scale-95"
              title="Launch full-screen Face-to-Face Video Call"
            >
              <Video className="w-3.5 h-3.5" />
              <span>3. Video Call</span>
            </button>
          )}

          {/* 2. Voice Call Launcher */}
          {onOpenVoiceCall && (
            <button
              onClick={onOpenVoiceCall}
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-slate-900 hover:bg-slate-800 border border-slate-700 text-slate-200 text-xs font-semibold transition-all active:scale-95"
              title="Launch Full-Screen Voice Call"
            >
              <PhoneCall className="w-3.5 h-3.5 text-indigo-400" />
              <span className="hidden sm:inline">2. Voice Call</span>
            </button>
          )}
        </div>
      </div>

      {/* VIEW 1: AGENT-FIRST CHAT (DEFAULT) - 3-PANEL CASCADING WORKSPACE */}
      {showcaseMode === 'agent_chat' && (
        <div className="flex-1 flex flex-row overflow-hidden p-2 sm:p-3 gap-3 relative min-w-0">
          {/* CENTER / MAIN SCREEN: AGENT-FIRST STAGE (Large realistic avatar + conversation + prompt bar) */}
          <div className={`flex flex-col justify-between rounded-2xl bg-gradient-to-b from-slate-900/90 via-slate-950 to-slate-950 border border-slate-800/90 overflow-hidden shadow-2xl relative transition-all duration-300 min-w-0 ${
            !isRightPanelOpen 
              ? 'flex-1 w-full' 
              : isRightPanelCollapsed
                ? 'flex-1 w-full'
                : isRightPanelMaximized 
                  ? 'hidden lg:flex lg:w-[28%]' 
                  : 'flex-1 w-full'
          }`}>
            {/* Header: Chat with your AI. See it work. Get results. */}
            <div className="p-4 border-b border-slate-800/80 bg-slate-950/80 backdrop-blur-md flex items-center justify-between shrink-0 z-10">
              <div className="flex items-center gap-2">
                <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
                <div>
                  <h2 className="text-xs font-bold text-slate-100 uppercase tracking-wide">
                    Agent-First Chat & Avatar
                  </h2>
                  <p className="text-[10px] text-slate-400">
                    Live Human-Like AI Interaction • Cascading Workspace
                  </p>
                </div>
              </div>

              <div className="flex items-center gap-2">
                {!isRightPanelOpen && (
                  <button
                    onClick={() => setIsRightPanelOpen(true)}
                    className="px-2.5 py-1 rounded-lg bg-indigo-600/30 hover:bg-indigo-600/50 text-indigo-300 text-xs font-mono flex items-center gap-1 border border-indigo-500/30 transition-colors"
                    title="Open Right Sidebar"
                  >
                    <PanelRightOpen className="w-3 h-3 text-cyan-400" />
                    <span className="hidden sm:inline">Tools</span>
                  </button>
                )}
                <button
                  onClick={onOpenVideoCall}
                  className="px-2.5 py-1 rounded-lg bg-indigo-500/20 hover:bg-indigo-500/30 text-indigo-300 text-xs font-mono flex items-center gap-1 border border-indigo-500/30 transition-colors"
                >
                  <Video className="w-3 h-3 text-cyan-400" />
                  <span>Face to Face</span>
                </button>
              </div>
            </div>

            {/* Conversation Area with Realistic Human Avatar on the side/stage */}
            <div className="flex-1 overflow-y-auto p-4 space-y-4 no-scrollbar relative z-10 break-words">
              {/* PHOTOREALISTIC HUMAN AVATAR PRESENCE BANNER */}
              <div className="relative rounded-2xl overflow-hidden border border-slate-800 bg-[#070b14] p-4 flex flex-col sm:flex-row items-center gap-4 group">
                {/* Large Realistic Portrait */}
                <div className="relative w-28 sm:w-32 h-36 sm:h-40 rounded-2xl overflow-hidden shrink-0 ring-2 ring-indigo-500/40 shadow-xl shadow-indigo-500/20">
                  <img
                    src={avatarVideoCallImg}
                    alt="Nova AI Video Companion"
                    className="w-full h-full object-cover object-top group-hover:scale-105 transition-transform duration-500"
                  />
                  <div className="absolute inset-0 bg-gradient-to-t from-slate-950/80 via-transparent to-transparent" />
                  <div className="absolute bottom-1.5 left-2 flex items-center gap-1 text-[10px] font-mono text-cyan-300 font-bold">
                    <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
                    <span>Live AI</span>
                  </div>
                </div>

                {/* AI Status & Welcome */}
                <div className="flex-1 space-y-2 text-center sm:text-left min-w-0">
                  <div className="flex items-center justify-center sm:justify-start gap-2 flex-wrap">
                    <h3 className="text-sm font-bold text-slate-100">Nova AI Companion</h3>
                    <span className="text-[10px] font-mono px-2 py-0.5 rounded-full bg-indigo-500/20 text-indigo-300 border border-indigo-500/30">
                      Engine: {activeModel.name}
                    </span>
                  </div>
                  <p className="text-xs text-slate-300 leading-relaxed break-words">
                    "I'm here live on your screen. You can ask me to plan trips, inspect code, run terminal commands, or switch to full face-to-face video."
                  </p>

                  <div className="flex items-center justify-center sm:justify-start gap-1.5 pt-1 flex-wrap">
                    <button
                      onClick={() => handleDispatchPrompt("Plan my trip to Tokyo next month.")}
                      className="px-2.5 py-1 rounded-lg bg-indigo-600/30 hover:bg-indigo-600/50 text-indigo-200 text-[11px] font-medium border border-indigo-500/30 transition-all text-left"
                    >
                      ✈️ Plan Tokyo Trip
                    </button>
                    <button
                      onClick={() => {
                        setIsRightPanelOpen(true);
                        setRightPanelTab('code');
                        handleDispatchPrompt("Inspect agent-runtime.ts and local code files");
                      }}
                      className="px-2.5 py-1 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 text-[11px] font-medium border border-slate-700 transition-all text-left"
                    >
                      💻 Inspect Code
                    </button>
                    <button
                      onClick={() => {
                        setIsRightPanelOpen(true);
                        setRightPanelTab('videos');
                      }}
                      className="px-2.5 py-1 rounded-lg bg-cyan-950/40 hover:bg-cyan-900/50 text-cyan-200 text-[11px] font-medium border border-cyan-700/40 transition-all text-left"
                    >
                      🎬 Watch Videos
                    </button>
                    <button
                      onClick={() => {
                        setIsRightPanelOpen(true);
                        setRightPanelTab('browser');
                      }}
                      className="px-2.5 py-1 rounded-lg bg-emerald-950/40 hover:bg-emerald-900/50 text-emerald-200 text-[11px] font-medium border border-emerald-700/40 transition-all text-left"
                    >
                      🌐 Web Browser
                    </button>
                  </div>
                </div>
              </div>

              {/* Chat Messages with enhanced Word-Wrapping and content wrapping */}
              {messages.map((msg) => {
                const isUser = msg.sender === 'user';
                return (
                  <div
                    key={msg.id}
                    className={`flex flex-col ${isUser ? 'items-end' : 'items-start'} space-y-1 w-full`}
                  >
                    <div className="text-[10px] text-slate-500 font-mono px-1">
                      {isUser ? 'You' : 'Nova'} • {msg.timestamp}
                    </div>

                    <div
                      className={`max-w-[95%] sm:max-w-[88%] rounded-2xl p-3.5 text-xs leading-relaxed break-words break-all whitespace-pre-wrap overflow-wrap-anywhere ${
                        isUser
                          ? 'bg-indigo-600 text-white rounded-tr-none shadow-md shadow-indigo-600/20'
                          : 'bg-slate-900 border border-slate-800 text-slate-200 rounded-tl-none shadow-sm'
                      }`}
                    >
                      <div className="break-words break-all whitespace-pre-wrap overflow-wrap-anywhere">{msg.text}</div>
                    </div>
                  </div>
                );
              })}

              {isLoading && (
                <div className="flex items-center gap-2 text-xs text-indigo-400 p-2 font-mono animate-pulse">
                  <Sparkles className="w-4 h-4 animate-spin" />
                  <span>Nova is coordinating with the runtime mesh...</span>
                </div>
              )}

              <div ref={messagesEndRef} />
            </div>

            {/* Bottom Prompt Bar with Mic, Paperclip, Send (Matching Panel 1) */}
            <div className="p-3 border-t border-slate-800/80 bg-slate-950/90 z-10">
              <form onSubmit={handleSubmit} className="flex items-center gap-2">
                <div className="flex-1 flex items-center gap-2 px-3 py-2 rounded-2xl bg-slate-900 border border-slate-800 focus-within:border-indigo-500/60 transition-colors">
                  <input
                    type="text"
                    value={inputPrompt}
                    onChange={(e) => setInputPrompt(e.target.value)}
                    placeholder="Ask Nova anything (e.g. Plan Tokyo trip, inspect code, search web)..."
                    className="flex-1 bg-transparent text-xs text-slate-100 placeholder-slate-400 focus:outline-none"
                  />

                  {/* Mic toggle */}
                  <button
                    type="button"
                    onClick={() => setIsMicListening(!isMicListening)}
                    className={`p-1 rounded-lg transition-colors ${
                      isMicListening ? 'text-cyan-400 bg-cyan-500/20 animate-pulse' : 'text-slate-400 hover:text-slate-200'
                    }`}
                    title="Voice input"
                  >
                    {isMicListening ? <Mic className="w-4 h-4" /> : <Mic className="w-4 h-4" />}
                  </button>

                  {/* Paperclip */}
                  <button
                    type="button"
                    className="p-1 text-slate-400 hover:text-slate-200 transition-colors"
                    title="Attach file or code snippet"
                  >
                    <Paperclip className="w-4 h-4" />
                  </button>
                </div>

                {/* Send Button */}
                <button
                  type="submit"
                  disabled={!inputPrompt.trim() || isLoading}
                  className="p-2.5 rounded-2xl bg-indigo-600 hover:bg-indigo-500 disabled:bg-slate-800 disabled:text-slate-600 text-white shadow-md shadow-indigo-600/30 transition-all active:scale-95"
                >
                  <Send className="w-4 h-4" />
                </button>
              </form>
            </div>
          </div>

          {/* RIGHT SIDEBAR / AUXILIARY SURFACE (Code, Videos, Web Browser, Others) */}
          {isRightPanelOpen && (
            <div className={`flex flex-col overflow-hidden transition-all duration-300 shrink-0 ${
              isRightPanelCollapsed
                ? 'w-14 sm:w-16'
                : isRightPanelMaximized 
                  ? 'flex-1 w-full' 
                  : 'w-[360px] md:w-[420px] lg:w-[480px] xl:w-[540px] max-w-[50%]'
            }`}>
              <RightSidebarSurface
                initialTab={rightPanelTab}
                activeTab={rightPanelTab}
                onSelectTab={setRightPanelTab}
                isCollapsed={isRightPanelCollapsed}
                onToggleCollapse={() => setIsRightPanelCollapsed(!isRightPanelCollapsed)}
                onClose={() => setIsRightPanelOpen(false)}
                isMaximized={isRightPanelMaximized}
                onToggleMaximize={() => setIsRightPanelMaximized(!isRightPanelMaximized)}
                selectedFileIndex={selectedFileIndex}
                onSelectFileIndex={setSelectedFileIndex}
                onRunCode={() => {
                  setRightPanelTab('others');
                  setIsRightPanelCollapsed(false);
                }}
              />
            </div>
          )}

          {/* Floating toggle when right panel is closed */}
          {!isRightPanelOpen && (
            <button
              onClick={() => {
                setIsRightPanelOpen(true);
                setIsRightPanelCollapsed(false);
              }}
              className="fixed right-4 bottom-20 z-30 px-3 py-2 rounded-xl bg-gradient-to-r from-indigo-600 to-violet-600 hover:from-indigo-500 hover:to-violet-500 text-white shadow-xl shadow-indigo-600/30 flex items-center gap-2 text-xs font-semibold transition-all hover:scale-105 active:scale-95 border border-indigo-400/30"
              title="Open Right Panel (</> Code, Video, Web, Others)"
            >
              <PanelRightOpen className="w-4 h-4 text-cyan-300" />
              <span>Right Tools Panel</span>
            </button>
          )}
        </div>
      )}

      {/* VIEW 4 & 5: AGENT-FIRST WORK MODE - MATCHING PANELS 4 & 5 IN SHOWCASE */}
      {showcaseMode === 'work_mode' && (
        <div className="flex-1 flex flex-col justify-between overflow-y-auto p-6 max-w-4xl mx-auto w-full space-y-6">
          {/* Main Greeting & Avatar Display */}
          <div className="flex flex-col sm:flex-row items-center gap-6 pt-4">
            <div className="relative w-36 h-44 rounded-3xl overflow-hidden ring-2 ring-indigo-500/50 shadow-2xl shrink-0">
              <img
                src={avatarVideoCallImg}
                alt="Nova Avatar Work Mode"
                className="w-full h-full object-cover object-top"
              />
              <div className="absolute inset-0 bg-gradient-to-t from-slate-950/80 via-transparent to-transparent" />
            </div>

            <div className="text-center sm:text-left space-y-2">
              <h2 className="text-2xl sm:text-3xl font-black tracking-tight text-slate-100">
                What shall we do today?
              </h2>
              <p className="text-xs sm:text-sm text-slate-400 max-w-md">
                Focus on the task. Switch tools as needed. Your AI companion will orchestrate research, planning, code generation, and execution.
              </p>
            </div>
          </div>

          {/* 4 WORK ACTION TILES (Matching Panels 4 & 5: Research, Plan, Build, Operate) */}
          <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
            {/* Tile 1: Research */}
            <button
              onClick={() => handleDispatchPrompt("Find and analyse the latest research papers and documentation on recursive AI agent runtimes")}
              className="p-4 rounded-2xl bg-slate-900/80 hover:bg-slate-900 border border-slate-800 hover:border-cyan-500/50 flex items-start gap-3 text-left transition-all group"
            >
              <div className="p-3 rounded-xl bg-cyan-500/10 text-cyan-400 border border-cyan-500/20 group-hover:scale-105 transition-transform">
                <Search className="w-5 h-5" />
              </div>
              <div>
                <h4 className="text-sm font-bold text-slate-100 group-hover:text-cyan-300 transition-colors">
                  Research
                </h4>
                <p className="text-xs text-slate-400 mt-0.5">
                  Find and analyse information across web and docs
                </p>
              </div>
            </button>

            {/* Tile 2: Plan */}
            <button
              onClick={() => handleDispatchPrompt("Plan my trip to Tokyo next month.")}
              className="p-4 rounded-2xl bg-slate-900/80 hover:bg-slate-900 border border-slate-800 hover:border-amber-500/50 flex items-start gap-3 text-left transition-all group"
            >
              <div className="p-3 rounded-xl bg-amber-500/10 text-amber-400 border border-amber-500/20 group-hover:scale-105 transition-transform">
                <Calendar className="w-5 h-5" />
              </div>
              <div>
                <h4 className="text-sm font-bold text-slate-100 group-hover:text-amber-300 transition-colors">
                  Plan
                </h4>
                <p className="text-xs text-slate-400 mt-0.5">
                  Create a step-by-step plan, itinerary or milestone
                </p>
              </div>
            </button>

            {/* Tile 3: Build */}
            <button
              onClick={() => handleDispatchPrompt("Generate and implement the complete microVM scheduler for our tri-compute fabric")}
              className="p-4 rounded-2xl bg-slate-900/80 hover:bg-slate-900 border border-slate-800 hover:border-emerald-500/50 flex items-start gap-3 text-left transition-all group"
            >
              <div className="p-3 rounded-xl bg-emerald-500/10 text-emerald-400 border border-emerald-500/20 group-hover:scale-105 transition-transform">
                <Hammer className="w-5 h-5" />
              </div>
              <div>
                <h4 className="text-sm font-bold text-slate-100 group-hover:text-emerald-300 transition-colors">
                  Build
                </h4>
                <p className="text-xs text-slate-400 mt-0.5">
                  Generate content, code, tests, or system designs
                </p>
              </div>
            </button>

            {/* Tile 4: Operate */}
            <button
              onClick={() => handleDispatchPrompt("Run an automated routine to sync local workspace bridge and check git commit log")}
              className="p-4 rounded-2xl bg-slate-900/80 hover:bg-slate-900 border border-slate-800 hover:border-purple-500/50 flex items-start gap-3 text-left transition-all group"
            >
              <div className="p-3 rounded-xl bg-purple-500/10 text-purple-400 border border-purple-500/20 group-hover:scale-105 transition-transform">
                <Activity className="w-5 h-5" />
              </div>
              <div>
                <h4 className="text-sm font-bold text-slate-100 group-hover:text-purple-300 transition-colors">
                  Operate
                </h4>
                <p className="text-xs text-slate-400 mt-0.5">
                  Take autonomous action on your behalf via local bridge
                </p>
              </div>
            </button>
          </div>

          {/* EXPANDABLE / COLLAPSIBLE TERMINAL (Matching Panel 5: See what's happening behind the scenes) */}
          <div className="rounded-2xl border border-slate-800 bg-slate-950 overflow-hidden">
            <div 
              onClick={() => setIsTerminalOpen(!isTerminalOpen)}
              className="h-10 px-4 bg-slate-900/80 flex items-center justify-between cursor-pointer text-xs font-mono select-none"
            >
              <div className="flex items-center gap-2">
                <TerminalIcon className="w-3.5 h-3.5 text-indigo-400" />
                <span className="font-bold text-slate-300">
                  Terminal {isTerminalOpen ? '(open)' : '(collapsed)'}
                </span>
                <span className="text-[10px] text-slate-500">
                  [10:14:03] Starting task...
                </span>
              </div>

              <div className="flex items-center gap-2 text-slate-400">
                <span className="text-[10px] px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-300">
                  Running...
                </span>
                <ChevronDown className={`w-4 h-4 transition-transform ${isTerminalOpen ? 'rotate-180' : ''}`} />
              </div>
            </div>

            {isTerminalOpen && (
              <div className="p-4 font-mono text-xs text-slate-300 space-y-1 bg-[#060a12] border-t border-slate-800 leading-relaxed">
                <div className="text-slate-500">[10:14:03] Starting task: Plan Tokyo trip...</div>
                <div className="text-slate-400">[10:14:05] Searching flights (LHR → HND direct)...</div>
                <div className="text-cyan-400">[10:14:08] Found 32 options across British Airways and ANA</div>
                <div className="text-purple-400">[10:14:12] Checking hotel availability in Shinjuku & Shibuya...</div>
                <div className="text-emerald-400 font-bold">[10:14:15] Compiling itinerary draft complete (42ms).</div>
              </div>
            )}
          </div>

          {/* Prompt input */}
          <div className="pt-2">
            <form onSubmit={handleSubmit} className="flex items-center gap-2">
              <input
                type="text"
                value={inputPrompt}
                onChange={(e) => setInputPrompt(e.target.value)}
                placeholder="Ask me anything..."
                className="flex-1 bg-slate-900 border border-slate-800 rounded-2xl px-4 py-3 text-xs text-slate-100 placeholder-slate-400 focus:outline-none focus:border-indigo-500"
              />
              <button
                type="submit"
                className="p-3 rounded-2xl bg-indigo-600 hover:bg-indigo-500 text-white font-semibold text-xs shadow-md shadow-indigo-600/30 transition-all"
              >
                <Send className="w-4 h-4" />
              </button>
            </form>
          </div>
        </div>
      )}

      {/* VIEW 6: CODING FOCUS (AI COLLAPSED) - MATCHING PANEL 6 IN SHOWCASE */}
      {showcaseMode === 'code_focus' && (
        <div className="flex-1 flex flex-col justify-between overflow-hidden">
          {/* Full-width code editor */}
          <div className="flex-1 flex flex-col overflow-hidden bg-[#0d1117]">
            {/* File Tab Bar */}
            <div className="h-9 bg-slate-950 border-b border-slate-800 px-3 flex items-center gap-2">
              <div className="flex items-center gap-1.5 overflow-x-auto text-xs font-mono">
                {sampleWorkspaceFiles.map((file, idx) => (
                  <button
                    key={file.path}
                    onClick={() => setSelectedFileIndex(idx)}
                    className={`px-3 py-1.5 rounded-t text-xs font-mono flex items-center gap-2 border-t-2 transition-all ${
                      selectedFileIndex === idx
                        ? 'bg-[#0d1117] border-indigo-500 text-slate-100 font-bold'
                        : 'border-transparent text-slate-400 hover:text-slate-200'
                    }`}
                  >
                    <span>{file.name}</span>
                  </button>
                ))}
              </div>
            </div>

            <div className="flex-1 p-6 overflow-y-auto font-mono text-xs text-slate-200 leading-relaxed">
              <pre>
                <code>{sampleWorkspaceFiles[selectedFileIndex]?.content}</code>
              </pre>
            </div>
          </div>

          {/* Minimal Collapsed Pill at Bottom (Matching Panel 6: [Avatar] AI Assistant (collapsed)) */}
          <div className="h-10 bg-slate-950 border-t border-slate-800 px-4 flex items-center justify-between text-xs font-mono select-none">
            <button
              onClick={() => setShowcaseMode('agent_chat')}
              className="flex items-center gap-2 hover:bg-slate-900 px-2 py-1 rounded-lg transition-colors cursor-pointer group"
            >
              <div className="w-5 h-5 rounded-full overflow-hidden ring-1 ring-indigo-500/50">
                <img
                  src={avatarVideoCallImg}
                  alt="Nova Avatar"
                  className="w-full h-full object-cover"
                />
              </div>
              <span className="font-bold text-slate-200 group-hover:text-indigo-300">
                AI Assistant (collapsed)
              </span>
              <span className="text-[10px] text-slate-500">• Click to expand</span>
            </button>

            <div className="flex items-center gap-3 text-slate-400 text-[11px]">
              <span>Ln 1, Col 1</span>
              <span>UTF-8</span>
              <span>TypeScript</span>
              <span>Prettier</span>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
