import React, { useState } from 'react';
import { 
  Sparkles, 
  Mic, 
  MicOff, 
  Volume2, 
  VolumeX, 
  Play, 
  Code2, 
  Bug, 
  FileCheck2, 
  Wrench, 
  Terminal, 
  Zap, 
  Send,
  Maximize2,
  ChevronRight
} from 'lucide-react';
import avatarVideoCallImg from '../assets/images/ai_avatar_videocall_1790061776459.jpg';
const novaAvatarImg = avatarVideoCallImg;

export type AvatarState = 'idle' | 'listening' | 'thinking' | 'speaking' | 'coding';

interface NovaAvatarViewProps {
  variant?: 'full' | 'compact' | 'card';
  activeModelName?: string;
  onDispatchPrompt?: (prompt: string) => void;
  onSwitchToWorkLayout?: () => void;
}

export const NovaAvatarView: React.FC<NovaAvatarViewProps> = ({
  variant = 'full',
  activeModelName = 'Gemini 3.8 Flash',
  onDispatchPrompt,
  onSwitchToWorkLayout
}) => {
  const [avatarState, setAvatarState] = useState<AvatarState>('idle');
  const [isMicActive, setIsMicActive] = useState(false);
  const [isMuted, setIsMuted] = useState(false);
  const [quickInput, setQuickInput] = useState('');

  const quickActionPrompts = [
    { label: 'Explain this code', icon: Code2, prompt: 'Please explain the architecture of this project and file dependencies in detail.' },
    { label: 'Help me debug', icon: Bug, prompt: 'Scan the active codebase for potential runtime errors, type mismatches, and edge cases.' },
    { label: 'Add a new feature', icon: Zap, prompt: 'Propose a new modular feature specification and write the implementation.' },
    { label: 'Create a commit message', icon: FileCheck2, prompt: 'Analyze recent file diffs and generate a Conventional Commits formatted commit message.' },
    { label: 'Refactor it', icon: Wrench, prompt: 'Refactor the active code for better readability, performance, and clean modularity.' },
    { label: 'Create tests', icon: Terminal, prompt: 'Write comprehensive unit and integration tests with mock fixtures for this module.' }
  ];

  const handleActionClick = (prompt: string) => {
    setAvatarState('thinking');
    setTimeout(() => setAvatarState('speaking'), 800);
    setTimeout(() => setAvatarState('idle'), 3500);
    if (onDispatchPrompt) {
      onDispatchPrompt(prompt);
    }
  };

  const handleFormSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (!quickInput.trim()) return;
    const text = quickInput;
    setQuickInput('');
    setAvatarState('thinking');
    setTimeout(() => setAvatarState('speaking'), 800);
    setTimeout(() => setAvatarState('idle'), 3500);
    if (onDispatchPrompt) {
      onDispatchPrompt(text);
    }
  };

  const toggleMic = () => {
    const next = !isMicActive;
    setIsMicActive(next);
    setAvatarState(next ? 'listening' : 'idle');
  };

  // Status badge styling
  const stateLabels: Record<AvatarState, { text: string; color: string }> = {
    idle: { text: 'Online & Ready', color: 'bg-emerald-500/20 text-emerald-300 border-emerald-500/30' },
    listening: { text: 'Listening...', color: 'bg-cyan-500/20 text-cyan-300 border-cyan-500/30 animate-pulse' },
    thinking: { text: 'Synthesizing Plan...', color: 'bg-purple-500/20 text-purple-300 border-purple-500/30 animate-pulse' },
    speaking: { text: 'Streaming Response', color: 'bg-indigo-500/20 text-indigo-300 border-indigo-500/30' },
    coding: { text: 'Writing AST Transforms', color: 'bg-amber-500/20 text-amber-300 border-amber-500/30' }
  };

  if (variant === 'compact') {
    return (
      <div className="flex items-center gap-3 p-3 rounded-2xl bg-slate-900/60 border border-slate-800 backdrop-blur-sm">
        <div className="relative group shrink-0">
          <div className="w-12 h-12 rounded-xl overflow-hidden ring-2 ring-indigo-500/50 shadow-lg shadow-indigo-500/20">
            <img 
              src={novaAvatarImg} 
              alt="Nova AI" 
              className="w-full h-full object-cover object-center group-hover:scale-105 transition-transform duration-300"
            />
          </div>
          <span className="absolute -bottom-1 -right-1 w-3.5 h-3.5 rounded-full bg-emerald-400 ring-2 ring-slate-950" />
        </div>

        <div className="flex-1 min-w-0">
          <div className="flex items-center justify-between gap-2">
            <div className="flex items-center gap-2">
              <h3 className="text-xs font-bold text-slate-100">Nova AI</h3>
              <span className={`text-[10px] font-mono px-1.5 py-0.2 rounded border ${stateLabels[avatarState].color}`}>
                {stateLabels[avatarState].text}
              </span>
            </div>
            <span className="text-[10px] text-slate-500 font-mono hidden sm:inline">
              Engine: {activeModelName}
            </span>
          </div>
          <p className="text-[11px] text-slate-400 truncate mt-0.5">
            "Good to see you. How can I help you build today?"
          </p>
        </div>

        <div className="flex items-center gap-1">
          <button
            onClick={toggleMic}
            className={`p-2 rounded-lg transition-colors ${
              isMicActive ? 'bg-cyan-500/20 text-cyan-300 border border-cyan-500/30' : 'bg-slate-800 text-slate-400 hover:text-slate-200'
            }`}
            title="Toggle Mic"
          >
            {isMicActive ? <Mic className="w-3.5 h-3.5" /> : <MicOff className="w-3.5 h-3.5" />}
          </button>
        </div>
      </div>
    );
  }

  if (variant === 'card') {
    return (
      <div className="flex flex-col items-center p-5 rounded-2xl bg-gradient-to-b from-slate-900/90 to-slate-950 border border-slate-800 text-center">
        <div className="relative mb-3">
          <div className="w-20 h-20 rounded-2xl overflow-hidden ring-2 ring-indigo-500/40 shadow-xl shadow-indigo-500/10">
            <img 
              src={novaAvatarImg} 
              alt="Nova AI" 
              className="w-full h-full object-cover object-top"
            />
          </div>
          <span className="absolute -bottom-1 -right-1 w-4 h-4 rounded-full bg-emerald-400 ring-2 ring-slate-950" />
        </div>

        <h3 className="text-sm font-bold text-slate-100">Good to see you.</h3>
        <p className="text-xs text-slate-400 mt-1 max-w-xs leading-relaxed">
          How can I help you build today?
        </p>

        {/* Quick prompt cards as seen in image 2 & 3 */}
        <div className="w-full grid grid-cols-1 sm:grid-cols-2 gap-2 mt-4">
          {quickActionPrompts.slice(0, 4).map((action, idx) => {
            const Icon = action.icon;
            return (
              <button
                key={idx}
                onClick={() => handleActionClick(action.prompt)}
                className="flex items-center gap-2 p-2.5 rounded-xl bg-slate-900/80 hover:bg-slate-800 border border-slate-800 hover:border-slate-700 text-left text-xs text-slate-200 transition-all hover:scale-[1.01]"
              >
                <div className="p-1.5 rounded-lg bg-slate-950 border border-slate-800 text-indigo-400 shrink-0">
                  <Icon className="w-3.5 h-3.5" />
                </div>
                <span className="truncate">{action.label}</span>
              </button>
            );
          })}
        </div>
      </div>
    );
  }

  // Full Avatar Layout (Mode 1 & 4 in collages: Pure focus, large avatar front and center)
  return (
    <div className="flex-1 flex flex-col items-center justify-between p-6 overflow-y-auto relative bg-radial-gradient">
      {/* Top Bar inside focus view */}
      <div className="w-full max-w-4xl flex items-center justify-between z-10">
        <div className="flex items-center gap-2.5">
          <span className="w-2.5 h-2.5 rounded-full bg-cyan-400 animate-pulse" />
          <span className="text-xs font-mono font-bold tracking-wider text-slate-300 uppercase">
            NEXUS / NOVA AI COGNITIVE CORE
          </span>
          <span className={`text-[10px] font-mono px-2 py-0.5 rounded-full border ${stateLabels[avatarState].color}`}>
            {stateLabels[avatarState].text}
          </span>
        </div>

        {onSwitchToWorkLayout && (
          <button
            onClick={onSwitchToWorkLayout}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-slate-900 hover:bg-slate-800 border border-slate-800 text-xs font-medium text-slate-300 hover:text-slate-100 transition-colors"
          >
            <span>Open Work Layout</span>
            <ChevronRight className="w-3.5 h-3.5" />
          </button>
        )}
      </div>

      {/* Main Center Avatar Stage */}
      <div className="my-auto flex flex-col items-center justify-center text-center z-10 max-w-xl">
        <div className="relative mb-6">
          {/* Ambient Glowing Aura */}
          <div className="absolute -inset-4 rounded-3xl bg-gradient-to-r from-indigo-500/20 via-cyan-500/20 to-purple-500/20 blur-xl opacity-70 animate-pulse" />

          {/* Avatar Container */}
          <div className="relative w-56 h-56 sm:w-64 sm:h-64 rounded-3xl overflow-hidden ring-2 ring-indigo-500/50 shadow-2xl shadow-indigo-500/30">
            <img 
              src={novaAvatarImg} 
              alt="Nova AI Autonomous Companion" 
              className="w-full h-full object-cover object-top filter contrast-[1.05]"
            />

            {/* Glowing Tech HUD overlay */}
            <div className="absolute inset-0 bg-gradient-to-t from-slate-950/80 via-transparent to-transparent flex flex-col justify-end p-4">
              <div className="flex items-center justify-between text-[11px] font-mono text-cyan-300">
                <span>CORE: ONLINE</span>
                <span>{activeModelName}</span>
              </div>
            </div>
          </div>

          {/* Interactive Audio Waveform pill under avatar if speaking/listening */}
          {(avatarState === 'listening' || avatarState === 'speaking') && (
            <div className="absolute -bottom-3 left-1/2 -translate-x-1/2 px-4 py-1 rounded-full bg-slate-900 border border-cyan-500/50 shadow-lg flex items-center gap-1">
              <span className="w-1 h-3 bg-cyan-400 rounded-full animate-bounce" />
              <span className="w-1 h-5 bg-cyan-400 rounded-full animate-bounce [animation-delay:0.1s]" />
              <span className="w-1 h-4 bg-indigo-400 rounded-full animate-bounce [animation-delay:0.2s]" />
              <span className="w-1 h-6 bg-cyan-400 rounded-full animate-bounce [animation-delay:0.3s]" />
              <span className="w-1 h-3 bg-cyan-400 rounded-full animate-bounce [animation-delay:0.15s]" />
            </div>
          )}
        </div>

        <h1 className="text-xl sm:text-2xl font-bold tracking-tight text-slate-100 mb-1">
          Good to see you.
        </h1>
        <p className="text-xs sm:text-sm text-slate-400 max-w-md leading-relaxed">
          How can I help you build today? I have direct control over your workspace, local desktop bridge, terminal containers, and model pipelines.
        </p>

        {/* 6 Quick Action Cards (from Layout 2 & 3 in gallery) */}
        <div className="w-full grid grid-cols-2 sm:grid-cols-3 gap-2 mt-6">
          {quickActionPrompts.map((action, idx) => {
            const Icon = action.icon;
            return (
              <button
                key={idx}
                onClick={() => handleActionClick(action.prompt)}
                className="flex items-center gap-2 p-3 rounded-xl bg-slate-900/80 hover:bg-slate-800 border border-slate-800 hover:border-slate-700 text-left text-xs text-slate-200 transition-all hover:scale-[1.02] shadow-sm"
              >
                <div className="p-1.5 rounded-lg bg-slate-950 border border-slate-800 text-indigo-400 shrink-0">
                  <Icon className="w-3.5 h-3.5" />
                </div>
                <span className="font-medium truncate">{action.label}</span>
              </button>
            );
          })}
        </div>
      </div>

      {/* Floating Prompt Bar at bottom */}
      <div className="w-full max-w-2xl z-10 mt-6">
        <form onSubmit={handleFormSubmit} className="relative flex items-center">
          <input
            type="text"
            value={quickInput}
            onChange={(e) => setQuickInput(e.target.value)}
            placeholder="Ask Nova anything... [Enter to send]"
            className="w-full bg-slate-900/90 border border-slate-800 rounded-2xl px-4 py-3 pr-24 text-xs text-slate-100 placeholder-slate-500 backdrop-blur-md focus:outline-none focus:border-indigo-500 shadow-xl"
          />

          <div className="absolute right-2 flex items-center gap-1.5">
            <button
              type="button"
              onClick={toggleMic}
              className={`p-2 rounded-xl transition-colors ${
                isMicActive ? 'bg-cyan-500 text-slate-950 font-bold' : 'text-slate-400 hover:text-slate-200 hover:bg-slate-800'
              }`}
              title={isMicActive ? "Mute mic" : "Speak with Nova"}
            >
              {isMicActive ? <Mic className="w-4 h-4 animate-pulse" /> : <MicOff className="w-4 h-4" />}
            </button>

            <button
              type="submit"
              disabled={!quickInput.trim()}
              className={`p-2 rounded-xl transition-all ${
                quickInput.trim()
                  ? 'bg-indigo-600 hover:bg-indigo-500 text-white shadow-md'
                  : 'bg-slate-800 text-slate-600 cursor-not-allowed'
              }`}
            >
              <Send className="w-4 h-4" />
            </button>
          </div>
        </form>

        <div className="flex items-center justify-between text-[11px] text-slate-500 px-2 mt-2">
          <span>Engine: {activeModelName}</span>
          <span>Zero vendor lock-in • Cryptographically guarded</span>
        </div>
      </div>
    </div>
  );
};
