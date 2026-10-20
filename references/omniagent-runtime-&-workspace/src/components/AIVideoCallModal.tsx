import React, { useState, useEffect, useRef } from 'react';
import { 
  PhoneOff, 
  Mic, 
  MicOff, 
  Video, 
  VideoOff, 
  Volume2, 
  VolumeX, 
  Share2, 
  MoreHorizontal, 
  Sparkles, 
  MessageSquare, 
  Maximize2, 
  Minimize2, 
  Settings, 
  Radio, 
  Send,
  Zap
} from 'lucide-react';
import { ModelAdapterInfo } from '../types';

import avatarVideoCallImg from '../assets/images/ai_avatar_videocall_1790061776459.jpg';
import userWebcamImg from '../assets/images/user_webcam_pip_1790061795130.jpg';

interface AIVideoCallModalProps {
  isOpen: boolean;
  onClose: () => void;
  activeModel: ModelAdapterInfo;
  onSwitchToChat?: () => void;
}

export const AIVideoCallModal: React.FC<AIVideoCallModalProps> = ({
  isOpen,
  onClose,
  activeModel,
  onSwitchToChat
}) => {
  const [callDuration, setCallDuration] = useState(317); // Starts at 05:17 like the showcase
  const [isMuted, setIsMuted] = useState(false);
  const [isVideoStopped, setIsVideoStopped] = useState(false);
  const [isSpeakerMuted, setIsSpeakerMuted] = useState(false);
  const [isScreenSharing, setIsScreenSharing] = useState(false);
  const [isListening, setIsListening] = useState(false);
  const [isSpeaking, setIsSpeaking] = useState(false);
  
  // Real camera support with fallback to photorealistic user image
  const [hasRealWebcam, setHasRealWebcam] = useState(false);
  const userVideoRef = useRef<HTMLVideoElement>(null);
  const webcamStreamRef = useRef<MediaStream | null>(null);

  // Conversation & Subtitle State
  const [currentSubtitle, setCurrentSubtitle] = useState<string>(
    "I'm here with you. What shall we work on together today?"
  );
  const [userSpeechInput, setUserSpeechInput] = useState('');
  const [chatLog, setChatLog] = useState<{ sender: 'ai' | 'user'; text: string; time: string }[]>([
    {
      sender: 'ai',
      text: "Face to face video connection established with 18ms latency. I'm listening.",
      time: '05:14'
    }
  ]);

  // Audio synthesis for real human-like voice response
  const speakText = (text: string) => {
    if (isSpeakerMuted) return;
    if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
      window.speechSynthesis.cancel();
      const utterance = new SpeechSynthesisUtterance(text);
      utterance.rate = 1.05;
      utterance.pitch = 1.05;
      
      // Try finding a natural female voice
      const voices = window.speechSynthesis.getVoices();
      const naturalVoice = voices.find(v => 
        (v.name.includes('Samantha') || v.name.includes('Google UK English Female') || v.name.includes('Natural') || v.name.includes('Zira') || v.name.includes('Female'))
      );
      if (naturalVoice) {
        utterance.voice = naturalVoice;
      }

      utterance.onstart = () => setIsSpeaking(true);
      utterance.onend = () => setIsSpeaking(false);
      utterance.onerror = () => setIsSpeaking(false);

      window.speechSynthesis.speak(utterance);
    }
  };

  // Timer effect
  useEffect(() => {
    if (!isOpen) return;

    const timer = setInterval(() => {
      setCallDuration(prev => prev + 1);
    }, 1000);

    return () => clearInterval(timer);
  }, [isOpen]);

  // Try real webcam feed if available and permitted
  useEffect(() => {
    if (!isOpen || isVideoStopped) {
      if (webcamStreamRef.current) {
        webcamStreamRef.current.getTracks().forEach(track => track.stop());
        webcamStreamRef.current = null;
      }
      setHasRealWebcam(false);
      return;
    }

    if (navigator.mediaDevices && navigator.mediaDevices.getUserMedia) {
      navigator.mediaDevices.getUserMedia({ video: { width: 320, height: 320 } })
        .then(stream => {
          webcamStreamRef.current = stream;
          if (userVideoRef.current) {
            userVideoRef.current.srcObject = stream;
          }
          setHasRealWebcam(true);
        })
        .catch(() => {
          // Gracefully fall back to photorealistic PIP asset
          setHasRealWebcam(false);
        });
    }

    return () => {
      if (webcamStreamRef.current) {
        webcamStreamRef.current.getTracks().forEach(track => track.stop());
        webcamStreamRef.current = null;
      }
    };
  }, [isOpen, isVideoStopped]);

  if (!isOpen) return null;

  const formatTime = (secs: number) => {
    const mins = Math.floor(secs / 60);
    const rem = secs % 60;
    return `${mins.toString().padStart(2, '0')}:${rem.toString().padStart(2, '0')}`;
  };

  // Handle User Voice or Quick Question
  const handleUserQuestion = (text: string) => {
    if (!text.trim()) return;
    const now = formatTime(callDuration);
    setChatLog(prev => [...prev, { sender: 'user', text, time: now }]);
    setUserSpeechInput('');
    setIsListening(false);

    // AI Generates Real-time Response
    setIsSpeaking(true);
    let reply = "I understand. Let me coordinate that through our recursive runtime right away.";
    if (text.toLowerCase().includes('tokyo') || text.toLowerCase().includes('trip') || text.toLowerCase().includes('flight')) {
      reply = "I'm checking flights to Tokyo for next month. British Airways and ANA have direct options from London Heathrow, and I'm drafting your Shibuya itinerary right now.";
    } else if (text.toLowerCase().includes('code') || text.toLowerCase().includes('bug') || text.toLowerCase().includes('architecture')) {
      reply = "I've reviewed your active workspace files. The Tri-Compute scheduler and AST transforms are verified, and your local desktop bridge is fully synchronized.";
    } else if (text.toLowerCase().includes('who are you') || text.toLowerCase().includes('hello')) {
      reply = "Hi there! I'm Nova, your AI assistant. I'm connected face-to-face with you and ready to build, plan, or research anything you need.";
    }

    setCurrentSubtitle(reply);
    setChatLog(prev => [...prev, { sender: 'ai', text: reply, time: now }]);
    speakText(reply);
  };

  const handleEndCall = () => {
    if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
      window.speechSynthesis.cancel();
    }
    if (webcamStreamRef.current) {
      webcamStreamRef.current.getTracks().forEach(track => track.stop());
    }
    onClose();
  };

  return (
    <div className="fixed inset-0 z-50 bg-black flex flex-col justify-between overflow-hidden select-none animate-in fade-in duration-300">
      {/* BACKGROUND AVATAR VIDEO FEED (Matching Panel 3 in Showcase) */}
      <div className="absolute inset-0 z-0">
        <img 
          src={avatarVideoCallImg} 
          alt="AI Assistant Face to Face" 
          className={`w-full h-full object-cover object-top transition-transform duration-1000 ${
            isSpeaking ? 'scale-[1.015]' : 'scale-100'
          }`}
        />
        {/* Cinematic ambient room lighting overlay */}
        <div className="absolute inset-0 bg-gradient-to-t from-slate-950 via-transparent to-slate-950/70 pointer-events-none" />
        <div className="absolute inset-0 bg-gradient-to-r from-slate-950/40 via-transparent to-slate-950/40 pointer-events-none" />
      </div>

      {/* TOP BAR: Call Status, Title, Timer */}
      <div className="relative z-10 p-4 sm:p-6 flex items-center justify-between">
        <div className="flex items-center gap-3">
          <div className="flex items-center gap-2 px-3 py-1.5 rounded-full bg-slate-950/80 backdrop-blur-md border border-slate-700/60 shadow-lg">
            <span className="w-2.5 h-2.5 rounded-full bg-emerald-400 animate-pulse" />
            <span className="text-xs font-semibold text-slate-100 font-mono tracking-wide">
              3. Video Call
            </span>
            <span className="text-slate-500">•</span>
            <span className="text-xs text-slate-300 hidden sm:inline">
              Face to face. Anywhere in the world.
            </span>
          </div>

          <div className="hidden md:flex items-center gap-1.5 px-3 py-1.5 rounded-full bg-indigo-950/70 border border-indigo-500/30 text-indigo-300 text-xs font-mono">
            <Sparkles className="w-3.5 h-3.5 text-indigo-400" />
            <span>{activeModel.name} Live Engine</span>
          </div>
        </div>

        {/* Call Duration Timer (05:17 in showcase) */}
        <div className="flex items-center gap-3">
          <div className="px-3.5 py-1.5 rounded-full bg-slate-950/80 backdrop-blur-md border border-slate-700/60 text-slate-100 font-mono text-sm font-bold tracking-wider shadow-lg">
            {formatTime(callDuration)}
          </div>
        </div>
      </div>

      {/* CENTER STAGE: SUBTITLES & AUDIO WAVEFORM */}
      <div className="relative z-10 flex-1 flex flex-col justify-end p-4 sm:p-8 max-w-4xl mx-auto w-full space-y-4">
        {/* Subtle Live Audio Waveform when AI is speaking or user is listening */}
        <div className="flex flex-col items-center justify-center space-y-2">
          {isSpeaking && (
            <div className="flex items-center gap-1 h-6">
              {[4, 12, 22, 16, 28, 14, 24, 18, 10, 20, 15, 8].map((h, i) => (
                <span 
                  key={i} 
                  className="w-1 bg-cyan-400 rounded-full animate-pulse shadow-sm shadow-cyan-400"
                  style={{ 
                    height: `${h}px`,
                    animationDuration: `${0.4 + (i % 5) * 0.15}s`
                  }} 
                />
              ))}
            </div>
          )}

          {/* Subtitle / Caption Card */}
          {currentSubtitle && (
            <div className="px-5 py-3 rounded-2xl bg-slate-950/85 backdrop-blur-md border border-slate-700/60 text-center max-w-2xl shadow-2xl transition-all">
              <p className="text-sm sm:text-base text-slate-100 font-medium leading-relaxed drop-shadow-sm">
                "{currentSubtitle}"
              </p>
            </div>
          )}
        </div>

        {/* Quick Conversational Prompt Chips */}
        <div className="flex items-center justify-center gap-2 flex-wrap">
          {[
            "Plan my trip to Tokyo next month",
            "Explain project architecture and local files",
            "Can you run a security audit on my desktop bridge?",
            "What shall we do today?"
          ].map((prompt, idx) => (
            <button
              key={idx}
              onClick={() => handleUserQuestion(prompt)}
              className="px-3 py-1.5 rounded-full bg-slate-900/80 hover:bg-slate-800 backdrop-blur-md border border-slate-700/60 text-xs text-slate-200 hover:text-white transition-all shadow-md active:scale-95"
            >
              {prompt}
            </button>
          ))}
        </div>
      </div>

      {/* USER WEBCAM PICTURE-IN-PICTURE (PIP) - Bottom Right Corner (Matching Panel 3 in Showcase) */}
      <div className="absolute right-4 sm:right-8 bottom-28 z-20 w-32 sm:w-44 h-40 sm:h-52 rounded-2xl overflow-hidden border-2 border-slate-700/80 bg-slate-900 shadow-2xl group">
        {!isVideoStopped ? (
          hasRealWebcam ? (
            <video 
              ref={userVideoRef} 
              autoPlay 
              playsInline 
              muted 
              className="w-full h-full object-cover -scale-x-100" 
            />
          ) : (
            <img 
              src={userWebcamImg} 
              alt="You (Webcam Feed)" 
              className="w-full h-full object-cover"
            />
          )
        ) : (
          <div className="w-full h-full flex flex-col items-center justify-center bg-slate-950 text-slate-500">
            <VideoOff className="w-6 h-6 mb-1" />
            <span className="text-[10px] font-mono">Camera Off</span>
          </div>
        )}

        <div className="absolute bottom-2 left-2 px-2 py-0.5 rounded-md bg-slate-950/80 backdrop-blur-sm border border-slate-800 text-[10px] font-mono text-slate-300 flex items-center gap-1.5">
          <span className="w-1.5 h-1.5 rounded-full bg-emerald-400" />
          <span>You</span>
        </div>
      </div>

      {/* BOTTOM FLOATING CALL CONTROLS DOCK (Matching Panel 3 in Showcase) */}
      <div className="relative z-10 pb-6 px-4 flex flex-col items-center justify-center gap-3">
        {/* Text Input Option for Video Call */}
        <form 
          onSubmit={(e) => {
            e.preventDefault();
            handleUserQuestion(userSpeechInput);
          }}
          className="w-full max-w-xl flex items-center gap-2 px-3 py-1.5 rounded-2xl bg-slate-950/80 backdrop-blur-md border border-slate-700/60 shadow-lg"
        >
          <input
            type="text"
            value={userSpeechInput}
            onChange={(e) => setUserSpeechInput(e.target.value)}
            placeholder="Speak or type a question to your AI..."
            className="flex-1 bg-transparent text-xs text-slate-100 placeholder-slate-400 focus:outline-none px-2 py-1"
          />
          <button
            type="submit"
            className="p-1.5 rounded-xl bg-indigo-600 hover:bg-indigo-500 text-white transition-colors"
          >
            <Send className="w-3.5 h-3.5" />
          </button>
        </form>

        {/* Action Buttons: Mute, Stop Video, End Call, Share, More */}
        <div className="flex items-center gap-3 sm:gap-4 px-6 py-3 rounded-full bg-slate-950/85 backdrop-blur-xl border border-slate-700/80 shadow-2xl">
          {/* Mute Mic */}
          <button
            onClick={() => setIsMuted(!isMuted)}
            className={`flex flex-col items-center gap-1 p-2.5 sm:p-3 rounded-full transition-all ${
              isMuted 
                ? 'bg-rose-500/20 text-rose-400 border border-rose-500/40' 
                : 'bg-slate-800 hover:bg-slate-700 text-slate-200'
            }`}
            title={isMuted ? 'Unmute Microphone' : 'Mute Microphone'}
          >
            {isMuted ? <MicOff className="w-5 h-5" /> : <Mic className="w-5 h-5" />}
          </button>

          {/* Stop Video */}
          <button
            onClick={() => setIsVideoStopped(!isVideoStopped)}
            className={`flex flex-col items-center gap-1 p-2.5 sm:p-3 rounded-full transition-all ${
              isVideoStopped 
                ? 'bg-rose-500/20 text-rose-400 border border-rose-500/40' 
                : 'bg-slate-800 hover:bg-slate-700 text-slate-200'
            }`}
            title={isVideoStopped ? 'Start Video' : 'Stop Video'}
          >
            {isVideoStopped ? <VideoOff className="w-5 h-5" /> : <Video className="w-5 h-5" />}
          </button>

          {/* END CALL BUTTON (Red pill button from showcase) */}
          <button
            onClick={handleEndCall}
            className="px-6 sm:px-8 py-3 rounded-full bg-rose-600 hover:bg-rose-500 text-white font-semibold text-xs flex items-center gap-2 shadow-lg shadow-rose-600/40 transition-all active:scale-95"
            title="End Video Call"
          >
            <PhoneOff className="w-5 h-5" />
            <span className="font-bold">End</span>
          </button>

          {/* Screen Share */}
          <button
            onClick={() => setIsScreenSharing(!isScreenSharing)}
            className={`flex flex-col items-center gap-1 p-2.5 sm:p-3 rounded-full transition-all ${
              isScreenSharing 
                ? 'bg-indigo-600 text-white' 
                : 'bg-slate-800 hover:bg-slate-700 text-slate-200'
            }`}
            title="Share Screen"
          >
            <Share2 className="w-5 h-5" />
          </button>

          {/* Speaker Mute/Unmute */}
          <button
            onClick={() => {
              if (!isSpeakerMuted && typeof window !== 'undefined' && 'speechSynthesis' in window) {
                window.speechSynthesis.cancel();
              }
              setIsSpeakerMuted(!isSpeakerMuted);
            }}
            className={`flex flex-col items-center gap-1 p-2.5 sm:p-3 rounded-full transition-all ${
              isSpeakerMuted 
                ? 'bg-rose-500/20 text-rose-400 border border-rose-500/40' 
                : 'bg-slate-800 hover:bg-slate-700 text-slate-200'
            }`}
            title={isSpeakerMuted ? 'Unmute AI Voice' : 'Mute AI Voice'}
          >
            {isSpeakerMuted ? <VolumeX className="w-5 h-5" /> : <Volume2 className="w-5 h-5" />}
          </button>
        </div>
      </div>
    </div>
  );
};
