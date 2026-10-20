import React, { useState, useEffect } from 'react';
import { 
  PhoneOff, 
  Mic, 
  MicOff, 
  Volume2, 
  VolumeX, 
  Sparkles, 
  Zap, 
  Video,
  Radio
} from 'lucide-react';
import { SubAgent, ModelAdapterInfo } from '../types';
import avatarVideoCallImg from '../assets/images/ai_avatar_videocall_1790061776459.jpg';

interface VoiceCallModalProps {
  isOpen: boolean;
  onClose: () => void;
  agent: SubAgent;
  model: ModelAdapterInfo;
  onSwitchToVideo?: () => void;
}

export const VoiceCallModal: React.FC<VoiceCallModalProps> = ({
  isOpen,
  onClose,
  agent,
  model,
  onSwitchToVideo
}) => {
  const [isMuted, setIsMuted] = useState(false);
  const [isSpeakerMuted, setIsSpeakerMuted] = useState(false);
  const [callDuration, setCallDuration] = useState(24); // 00:24 like in showcase
  const [isListening, setIsListening] = useState(true);
  const [isSpeaking, setIsSpeaking] = useState(false);
  const [transcript, setTranscript] = useState<string>('Listening...');

  const speakText = (text: string) => {
    if (isSpeakerMuted) return;
    if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
      window.speechSynthesis.cancel();
      const utterance = new SpeechSynthesisUtterance(text);
      utterance.rate = 1.05;
      utterance.pitch = 1.05;
      const voices = window.speechSynthesis.getVoices();
      const voice = voices.find(v => v.name.includes('Samantha') || v.name.includes('Female') || v.name.includes('Google UK English Female'));
      if (voice) utterance.voice = voice;
      utterance.onstart = () => setIsSpeaking(true);
      utterance.onend = () => {
        setIsSpeaking(false);
        setIsListening(true);
      };
      window.speechSynthesis.speak(utterance);
    }
  };

  useEffect(() => {
    if (!isOpen) {
      setCallDuration(24);
      if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
        window.speechSynthesis.cancel();
      }
      return;
    }

    const timer = setInterval(() => {
      setCallDuration(prev => prev + 1);
    }, 1000);

    return () => clearInterval(timer);
  }, [isOpen]);

  if (!isOpen) return null;

  const formatTime = (secs: number) => {
    const mins = Math.floor(secs / 60);
    const rem = secs % 60;
    return `${mins.toString().padStart(2, '0')}:${rem.toString().padStart(2, '0')}`;
  };

  const handleSimulateUserSpeech = (prompt?: string) => {
    const questions = [
      "Plan my trip to Tokyo next month.",
      "Check the status of my workspace tools and git repository.",
      "Can we swap to Claude 3.5 Sonnet engine for the coding tasks?"
    ];
    const q = prompt || questions[Math.floor(Math.random() * questions.length)];
    setIsListening(false);
    setTranscript(`"${q}"`);

    setTimeout(() => {
      let reply = "I'll handle that for you right now. Hunting flights, accommodation, and a personalized itinerary.";
      if (q.includes("workspace") || q.includes("git")) {
        reply = "Workspace tools verified. 6 parallel sub-agents are active and your desktop bridge is synchronized.";
      }
      setTranscript(reply);
      speakText(reply);
    }, 800);
  };

  const handleClose = () => {
    if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
      window.speechSynthesis.cancel();
    }
    onClose();
  };

  return (
    <div className="fixed inset-0 z-50 bg-black flex flex-col justify-between overflow-hidden select-none animate-in fade-in duration-300">
      {/* FULL SCREEN REALISTIC AVATAR BACKGROUND (Panel 2 from Showcase) */}
      <div className="absolute inset-0 z-0">
        <img 
          src={avatarVideoCallImg} 
          alt="AI Assistant Voice Call" 
          className="w-full h-full object-cover object-top filter brightness-95"
        />
        <div className="absolute inset-0 bg-gradient-to-t from-slate-950 via-slate-950/20 to-slate-950/70" />
      </div>

      {/* TOP BAR: Title & Timer (00:24) */}
      <div className="relative z-10 p-6 flex items-center justify-between">
        <div className="flex items-center gap-3">
          <div className="flex items-center gap-2 px-3 py-1.5 rounded-full bg-slate-950/80 backdrop-blur-md border border-slate-700/60 shadow-lg">
            <span className="w-2.5 h-2.5 rounded-full bg-cyan-400 animate-pulse" />
            <span className="text-xs font-semibold text-slate-100 font-mono">
              2. Voice Call (Full Screen)
            </span>
            <span className="text-slate-500">•</span>
            <span className="text-xs text-slate-300 hidden sm:inline">
              A natural conversation. No distractions.
            </span>
          </div>
        </div>

        <div className="px-3.5 py-1.5 rounded-full bg-slate-950/80 backdrop-blur-md border border-slate-700/60 text-slate-100 font-mono text-sm font-bold shadow-lg">
          {formatTime(callDuration)}
        </div>
      </div>

      {/* CENTER / LOWER-CENTER: "Listening..." & Animated Audio Waveform */}
      <div className="relative z-10 flex-1 flex flex-col items-center justify-end pb-8 px-4 space-y-4 max-w-xl mx-auto w-full">
        {/* State Indicator */}
        <div className="text-center space-y-2">
          <div className="text-lg sm:text-xl font-medium text-slate-100 drop-shadow-md">
            {transcript}
          </div>

          {/* Waveform Visualizer (Matching panel 2) */}
          <div className="flex items-center justify-center gap-1.5 h-8 pt-2">
            {[10, 16, 26, 38, 24, 46, 32, 50, 28, 44, 20, 14].map((h, i) => (
              <span
                key={i}
                className="w-1 bg-gradient-to-t from-cyan-400 to-indigo-400 rounded-full animate-pulse shadow-sm shadow-cyan-400"
                style={{
                  height: `${h}px`,
                  animationDuration: `${0.45 + (i % 4) * 0.12}s`
                }}
              />
            ))}
          </div>
        </div>

        {/* Quick Voice Prompt Buttons */}
        <div className="flex items-center justify-center gap-2 pt-2 flex-wrap">
          <button
            onClick={() => handleSimulateUserSpeech("Plan my trip to Tokyo next month.")}
            className="px-3 py-1.5 rounded-full bg-slate-950/80 hover:bg-slate-900 border border-slate-700/60 text-xs text-slate-200 shadow-md transition-all"
          >
            "Plan my trip to Tokyo"
          </button>
          <button
            onClick={() => handleSimulateUserSpeech("Check my workspace and bridge status.")}
            className="px-3 py-1.5 rounded-full bg-slate-950/80 hover:bg-slate-900 border border-slate-700/60 text-xs text-slate-200 shadow-md transition-all"
          >
            "Check workspace status"
          </button>
        </div>
      </div>

      {/* BOTTOM FLOATING CALL DOCK: Mute, End (Red pill), Speaker, Video Switch */}
      <div className="relative z-10 pb-8 flex items-center justify-center gap-6">
        <div className="flex items-center gap-4 sm:gap-6 px-8 py-3.5 rounded-full bg-slate-950/85 backdrop-blur-xl border border-slate-700/80 shadow-2xl">
          {/* Mute Button */}
          <button
            onClick={() => setIsMuted(!isMuted)}
            className={`p-3 rounded-full transition-all ${
              isMuted 
                ? 'bg-rose-500/20 text-rose-400 border border-rose-500/40' 
                : 'bg-slate-800 hover:bg-slate-700 text-slate-200'
            }`}
            title="Mute"
          >
            {isMuted ? <MicOff className="w-5 h-5" /> : <Mic className="w-5 h-5" />}
          </button>

          {/* End Button (Red pill from showcase) */}
          <button
            onClick={handleClose}
            className="px-8 py-3 rounded-full bg-rose-600 hover:bg-rose-500 text-white font-bold text-xs flex items-center gap-2 shadow-lg shadow-rose-600/40 transition-all active:scale-95"
            title="End Call"
          >
            <PhoneOff className="w-5 h-5" />
            <span>End</span>
          </button>

          {/* Speaker Button */}
          <button
            onClick={() => {
              if (!isSpeakerMuted && typeof window !== 'undefined' && 'speechSynthesis' in window) {
                window.speechSynthesis.cancel();
              }
              setIsSpeakerMuted(!isSpeakerMuted);
            }}
            className={`p-3 rounded-full transition-all ${
              isSpeakerMuted 
                ? 'bg-rose-500/20 text-rose-400 border border-rose-500/40' 
                : 'bg-slate-800 hover:bg-slate-700 text-slate-200'
            }`}
            title="Speaker"
          >
            {isSpeakerMuted ? <VolumeX className="w-5 h-5" /> : <Volume2 className="w-5 h-5" />}
          </button>

          {/* Switch to Video Call */}
          {onSwitchToVideo && (
            <button
              onClick={() => {
                handleClose();
                onSwitchToVideo();
              }}
              className="p-3 rounded-full bg-indigo-600 hover:bg-indigo-500 text-white transition-all shadow-md"
              title="Switch to Face-to-Face Video Call"
            >
              <Video className="w-5 h-5" />
            </button>
          )}
        </div>
      </div>
    </div>
  );
};
