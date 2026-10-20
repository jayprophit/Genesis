import React, { useState } from 'react';
import { 
  Search, 
  Plus, 
  RefreshCw, 
  Send, 
  Mic, 
  MicOff, 
  Video, 
  VideoOff, 
  PhoneOff, 
  Volume2, 
  VolumeX, 
  Settings, 
  ShieldCheck, 
  Lock, 
  Copy, 
  Check, 
  Share2, 
  Gift, 
  ExternalLink, 
  Sparkles, 
  MessageSquare, 
  Edit3, 
  BarChart3, 
  Calendar, 
  Wrench, 
  MoreHorizontal, 
  Home, 
  Users, 
  FolderKanban, 
  FolderGit2, 
  CheckSquare, 
  Contact, 
  Globe, 
  ShoppingBag, 
  Award, 
  Layers, 
  Briefcase, 
  GraduationCap, 
  HeartHandshake, 
  Hammer, 
  Maximize2,
  Minimize2,
  ChevronRight,
  Filter,
  SlidersHorizontal,
  Camera
} from 'lucide-react';
import oeSarahImg from '../assets/images/oe_sarah_avatar_1790063627214.jpg';
import userPipImg from '../assets/images/user_webcam_pip_1790061795130.jpg';
import avatarVideoCallImg from '../assets/images/ai_avatar_videocall_1790061776459.jpg';
import { ModelAdapterInfo } from '../types';

interface OELifeAmplifiedDashboardProps {
  activeModel: ModelAdapterInfo;
  onDispatchPrompt?: (text: string) => Promise<any> | any;
  onOpenVideoCallModal?: () => void;
}

export const OELifeAmplifiedDashboard: React.FC<OELifeAmplifiedDashboardProps> = ({
  activeModel,
  onDispatchPrompt,
  onOpenVideoCallModal
}) => {
  // Top nav modes matching Image 2
  const [topMode, setTopMode] = useState<'chat' | 'create' | 'work' | 'learn' | 'live' | 'build' | 'together'>('chat');
  
  // Left sidebar active navigation
  const [leftNav, setLeftNav] = useState('chat');
  
  // Chat thread selection
  const [activeChatThread, setActiveChatThread] = useState('marketing-q1');
  const [chatFilter, setChatFilter] = useState<'all' | 'pinned' | 'projects' | 'agents'>('all');
  
  // Calls & Messages tab
  const [callsTab, setCallsTab] = useState<'messages' | 'calls' | 'contacts'>('messages');
  
  // Active call state in the right overlay
  const [isInCall, setIsInCall] = useState(true);
  const [isMicMuted, setIsMicMuted] = useState(false);
  const [isVideoStopped, setIsVideoStopped] = useState(false);
  const [isSpeakerMuted, setIsSpeakerMuted] = useState(false);
  const [callTimer, setCallTimer] = useState('12:24');

  // Input & Chat conversation
  const [inputMessage, setInputMessage] = useState('');
  const [chatMessages, setChatMessages] = useState<Array<{ sender: 'sarah' | 'alex'; text: string; time: string }>>([
    {
      sender: 'sarah',
      text: "Hey Alex 👋\nLet's make Q1 amazing.\nWhat would you like to work on today?",
      time: '10:24 AM'
    }
  ]);
  const [isAiResponding, setIsAiResponding] = useState(false);
  const [copiedLink, setCopiedLink] = useState(false);

  // Send message
  const handleSendMessage = async (customPrompt?: string) => {
    const textToSend = customPrompt || inputMessage;
    if (!textToSend.trim() || isAiResponding) return;

    const newMsg = { sender: 'alex' as const, text: textToSend, time: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }) };
    setChatMessages(prev => [...prev, newMsg]);
    setInputMessage('');
    setIsAiResponding(true);

    try {
      if (onDispatchPrompt) {
        const reply = await onDispatchPrompt(textToSend);
        setChatMessages(prev => [
          ...prev,
          { sender: 'sarah' as const, text: reply, time: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }) }
        ]);
      } else {
        // Fallback response matching Sarah's supportive personality
        setTimeout(() => {
          setChatMessages(prev => [
            ...prev,
            { 
              sender: 'sarah' as const, 
              text: `Got it, Alex! I've linked that into our Q1 milestones and synced with the Marketing & Product agents. Here is the draft breakdown ready for your review.`, 
              time: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }) 
            }
          ]);
          setIsAiResponding(false);
        }, 800);
      }
    } catch {
      setIsAiResponding(false);
    } finally {
      setIsAiResponding(false);
    }
  };

  const handleCopyInvite = () => {
    navigator.clipboard.writeText('https://oe.com/invite/ALEX');
    setCopiedLink(true);
    setTimeout(() => setCopiedLink(false), 2000);
  };

  return (
    <div className="flex-1 flex flex-col h-full bg-[#080d19] text-slate-100 overflow-y-auto no-scrollbar font-sans selection:bg-cyan-500/30">
      
      {/* 1. TOP HEADER: "Your Life. Amplified. Humans. AI. Together." */}
      <header className="h-16 px-4 sm:px-6 bg-[#0a1122]/90 backdrop-blur-md border-b border-slate-800/80 flex items-center justify-between shrink-0 sticky top-0 z-20">
        {/* Left: OE Brand Logo & Tagline */}
        <div className="flex items-center gap-4">
          <div className="flex items-center gap-3">
            <div className="w-10 h-10 rounded-2xl bg-gradient-to-tr from-cyan-400 via-indigo-500 to-purple-500 p-[1.5px] shadow-lg shadow-cyan-500/20">
              <div className="w-full h-full bg-[#070b14] rounded-[14px] flex items-center justify-center font-black text-transparent bg-clip-text bg-gradient-to-r from-cyan-300 to-indigo-200 text-lg tracking-tighter">
                OE
              </div>
            </div>
            <div>
              <div className="text-sm font-extrabold tracking-tight text-white flex items-center gap-2">
                Your Life. Amplified.
              </div>
              <div className="text-[11px] text-cyan-400 font-medium flex items-center gap-1">
                <span>✦</span> Humans. AI. Together.
              </div>
            </div>
          </div>
        </div>

        {/* Center: Top Navigation Modes (Chat, Create, Work, Learn, Live, Build, Together) */}
        <div className="hidden lg:flex items-center gap-1 bg-[#050811]/80 p-1 rounded-2xl border border-slate-800/90 shadow-inner">
          {[
            { id: 'chat', label: 'Chat', icon: MessageSquare },
            { id: 'create', label: 'Create', icon: Edit3 },
            { id: 'work', label: 'Work', icon: Briefcase },
            { id: 'learn', label: 'Learn', icon: GraduationCap },
            { id: 'live', label: 'Live', icon: HeartHandshake },
            { id: 'build', label: 'Build', icon: Hammer },
            { id: 'together', label: 'Together', icon: Users }
          ].map(item => {
            const Icon = item.icon;
            const isActive = topMode === item.id;
            return (
              <button
                key={item.id}
                onClick={() => setTopMode(item.id as any)}
                className={`flex items-center gap-1.5 px-3 py-1.5 rounded-xl text-xs font-semibold transition-all ${
                  isActive
                    ? 'bg-gradient-to-r from-cyan-600 to-indigo-600 text-white shadow-md shadow-cyan-600/30'
                    : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900/60'
                }`}
              >
                <Icon className={`w-3.5 h-3.5 ${isActive ? 'text-white' : 'text-slate-400'}`} />
                <span>{item.label}</span>
              </button>
            );
          })}
        </div>

        {/* Right: Taglines */}
        <div className="flex items-center gap-4">
          <div className="hidden md:flex flex-col text-right">
            <span className="text-xs font-bold text-slate-200">One Platform.</span>
            <span className="text-[11px] text-slate-400">Every Possibility.</span>
          </div>
          <div className="hidden xl:flex items-center gap-2 pl-3 border-l border-slate-800 text-[11px] text-slate-400">
            <span className="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" />
            <span>More Freedom. A Kinder Future.</span>
          </div>
        </div>
      </header>

      {/* 2. MAIN 4-COLUMN WORKSPACE CANVAS */}
      <div className="flex flex-col xl:flex-row flex-1 overflow-hidden p-3 gap-3 min-w-0">
        
        {/* COLUMN 1: LEFT SIDEBAR (Profile Alex, Search, Nav, Quick Access) */}
        <aside className="w-full xl:w-56 shrink-0 bg-[#070b14]/90 rounded-2xl border border-slate-800/80 p-3 flex flex-col justify-between">
          <div className="space-y-3">
            {/* User Profile: Alex Online */}
            <div className="flex items-center gap-2.5 p-2 rounded-xl bg-slate-900/60 border border-slate-800">
              <div className="relative">
                <img
                  src={userPipImg}
                  alt="Alex"
                  className="w-9 h-9 rounded-full object-cover ring-2 ring-indigo-500/40"
                />
                <span className="absolute bottom-0 right-0 w-2.5 h-2.5 rounded-full bg-emerald-400 ring-2 ring-[#070b14]" />
              </div>
              <div className="min-w-0">
                <div className="text-xs font-bold text-slate-100 truncate">Alex</div>
                <div className="text-[10px] text-emerald-400 flex items-center gap-1 font-mono">
                  Online
                </div>
              </div>
            </div>

            {/* Search anything... */}
            <div className="relative">
              <Search className="w-3.5 h-3.5 absolute left-2.5 top-2.5 text-slate-500" />
              <input
                type="text"
                placeholder="Search anything..."
                className="w-full pl-8 pr-3 py-1.5 rounded-xl bg-[#050811] border border-slate-800/90 text-xs text-slate-200 placeholder:text-slate-600 focus:outline-none focus:border-cyan-500 transition-colors"
              />
            </div>

            {/* Menu Items */}
            <nav className="space-y-0.5 text-xs">
              {[
                { id: 'home', label: 'Home', icon: Home },
                { id: 'chat', label: 'Chat', icon: MessageSquare, active: true },
                { id: 'agents', label: 'Agents', icon: Users },
                { id: 'projects', label: 'Projects', icon: FolderKanban },
                { id: 'calendar', label: 'Calendar', icon: Calendar },
                { id: 'files', label: 'Files', icon: FolderGit2 },
                { id: 'tasks', label: 'Tasks', icon: CheckSquare },
                { id: 'contacts', label: 'Contacts', icon: Contact },
                { id: 'communities', label: 'Communities', icon: Globe },
                { id: 'marketplace', label: 'Marketplace', icon: ShoppingBag },
                { id: 'rewards', label: 'Rewards', icon: Award, badge: 'New' }
              ].map(item => {
                const Icon = item.icon;
                const isActive = leftNav === item.id;
                return (
                  <button
                    key={item.id}
                    onClick={() => setLeftNav(item.id)}
                    className={`w-full flex items-center justify-between px-2.5 py-1.5 rounded-xl text-left transition-all ${
                      isActive 
                        ? 'bg-gradient-to-r from-indigo-950/80 to-slate-900 text-indigo-300 font-semibold border border-indigo-700/40 shadow-sm' 
                        : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900/40'
                    }`}
                  >
                    <div className="flex items-center gap-2">
                      <Icon className={`w-3.5 h-3.5 ${isActive ? 'text-indigo-400' : 'text-slate-500'}`} />
                      <span>{item.label}</span>
                    </div>
                    {item.badge && (
                      <span className="text-[9px] px-1.5 py-0.2 rounded-full bg-emerald-500/20 text-emerald-300 font-bold border border-emerald-500/30">
                        {item.badge}
                      </span>
                    )}
                  </button>
                );
              })}
            </nav>

            {/* Quick Access */}
            <div className="pt-2 border-t border-slate-800/80 space-y-1">
              <div className="text-[10px] uppercase font-mono tracking-wider text-slate-500 px-1">
                Quick Access
              </div>
              <button className="w-full flex items-center gap-1.5 px-2 py-1 rounded-lg text-[11px] text-slate-300 hover:text-white hover:bg-slate-900/60 transition-colors">
                <Plus className="w-3 h-3 text-cyan-400" />
                <span>New Project</span>
              </button>
              <button className="w-full flex items-center gap-1.5 px-2 py-1 rounded-lg text-[11px] text-slate-300 hover:text-white hover:bg-slate-900/60 transition-colors">
                <Plus className="w-3 h-3 text-indigo-400" />
                <span>Create Agent</span>
              </button>
              <button className="w-full flex items-center gap-1.5 px-2 py-1 rounded-lg text-[11px] text-slate-300 hover:text-white hover:bg-slate-900/60 transition-colors">
                <Users className="w-3 h-3 text-purple-400" />
                <span>Join Community</span>
              </button>
              <button className="w-full flex items-center gap-1.5 px-2 py-1 rounded-lg text-[11px] text-slate-300 hover:text-white hover:bg-slate-900/60 transition-colors">
                <Award className="w-3 h-3 text-amber-400" />
                <span>View Plans</span>
              </button>
            </div>
          </div>

          {/* Bottom Motto */}
          <div className="pt-3 border-t border-slate-800/80 text-[10px] text-slate-500 leading-tight">
            AI that works for you. People who grow with you. A future we build together.
          </div>
        </aside>

        {/* COLUMN 2: CHAT THREADS LIST */}
        <section className="w-full xl:w-64 shrink-0 bg-[#070b14]/90 rounded-2xl border border-slate-800/80 p-3 flex flex-col justify-between">
          <div className="space-y-3">
            {/* Header: Chat + New Chat button */}
            <div className="flex items-center justify-between">
              <h2 className="text-sm font-bold text-slate-100 flex items-center gap-1.5">
                <span>Chat</span>
              </h2>
              <div className="flex items-center gap-1">
                <button 
                  onClick={() => handleSendMessage("Let's begin a fresh initiative for Q1.")}
                  className="p-1 rounded-lg bg-indigo-600/30 hover:bg-indigo-600/50 text-indigo-300 border border-indigo-500/30 transition-colors"
                  title="New Chat"
                >
                  <Plus className="w-3.5 h-3.5" />
                </button>
                <button 
                  onClick={() => setChatMessages([{ sender: 'sarah', text: "Hey Alex 👋\nLet's make Q1 amazing.\nWhat would you like to work on today?", time: '10:24 AM' }])}
                  className="p-1 rounded-lg hover:bg-slate-800 text-slate-400 transition-colors"
                  title="Reset conversation"
                >
                  <RefreshCw className="w-3.5 h-3.5" />
                </button>
              </div>
            </div>

            {/* Filter pills: All, Pinned, Projects, Agents */}
            <div className="flex items-center gap-1 bg-[#050811] p-1 rounded-xl border border-slate-800 text-[11px]">
              {(['all', 'pinned', 'projects', 'agents'] as const).map(tab => (
                <button
                  key={tab}
                  onClick={() => setChatFilter(tab)}
                  className={`flex-1 py-1 rounded-lg font-medium capitalize transition-all ${
                    chatFilter === tab 
                      ? 'bg-slate-800 text-white shadow-sm' 
                      : 'text-slate-400 hover:text-slate-200'
                  }`}
                >
                  {tab}
                </button>
              ))}
            </div>

            {/* Conversation List */}
            <div className="space-y-3 overflow-y-auto max-h-[520px] no-scrollbar text-xs pr-1">
              {/* Group Today */}
              <div>
                <div className="text-[10px] font-mono uppercase text-slate-500 mb-1.5 px-1">Today</div>
                <div className="space-y-1">
                  <div 
                    onClick={() => setActiveChatThread('marketing-q1')}
                    className={`p-2 rounded-xl cursor-pointer transition-all flex items-start gap-2 ${
                      activeChatThread === 'marketing-q1' 
                        ? 'bg-indigo-950/60 border border-indigo-700/50 shadow-sm' 
                        : 'hover:bg-slate-900/60 border border-transparent'
                    }`}
                  >
                    <img src={oeSarahImg} alt="Sarah" className="w-7 h-7 rounded-full object-cover shrink-0 ring-1 ring-cyan-500/40" />
                    <div className="min-w-0 flex-1">
                      <div className="flex items-center justify-between">
                        <span className="font-bold text-slate-100 truncate">Marketing Plan Q1</span>
                        <span className="text-[10px] text-slate-500">10:24 AM</span>
                      </div>
                      <p className="text-[11px] text-slate-400 truncate">Let's refine the strategy...</p>
                    </div>
                  </div>

                  <div 
                    onClick={() => {
                      setActiveChatThread('travel-itinerary');
                      handleSendMessage("Show me our Tokyo trip itinerary and flight options.");
                    }}
                    className={`p-2 rounded-xl cursor-pointer transition-all flex items-start gap-2 ${
                      activeChatThread === 'travel-itinerary' 
                        ? 'bg-indigo-950/60 border border-indigo-700/50 shadow-sm' 
                        : 'hover:bg-slate-900/60 border border-transparent'
                    }`}
                  >
                    <div className="w-7 h-7 rounded-full bg-cyan-500/20 text-cyan-400 flex items-center justify-center shrink-0 font-bold text-xs">
                      ✈️
                    </div>
                    <div className="min-w-0 flex-1">
                      <div className="flex items-center justify-between">
                        <span className="font-bold text-slate-100 truncate">Travel Itinerary</span>
                        <span className="text-[10px] text-slate-500">8:41 AM</span>
                      </div>
                      <p className="text-[11px] text-slate-400 truncate">Here's your custom plan...</p>
                    </div>
                  </div>

                  <div 
                    onClick={() => setActiveChatThread('website-redesign')}
                    className="p-2 rounded-xl hover:bg-slate-900/60 border border-transparent cursor-pointer transition-all flex items-start gap-2"
                  >
                    <div className="w-7 h-7 rounded-full bg-purple-500/20 text-purple-400 flex items-center justify-center shrink-0 font-bold text-xs">
                      🎨
                    </div>
                    <div className="min-w-0 flex-1">
                      <div className="flex items-center justify-between">
                        <span className="font-bold text-slate-100 truncate">Website Redesign</span>
                        <span className="text-[10px] text-slate-500">7:12 AM</span>
                      </div>
                      <p className="text-[11px] text-slate-400 truncate">I've added the new layouts...</p>
                    </div>
                  </div>
                </div>
              </div>

              {/* Group Yesterday */}
              <div>
                <div className="text-[10px] font-mono uppercase text-slate-500 mb-1.5 px-1">Yesterday</div>
                <div className="space-y-1">
                  <div className="p-2 rounded-xl hover:bg-slate-900/60 border border-transparent cursor-pointer transition-all flex items-start gap-2">
                    <div className="w-7 h-7 rounded-full bg-emerald-500/20 text-emerald-400 flex items-center justify-center shrink-0 font-bold text-xs">
                      🏃
                    </div>
                    <div className="min-w-0 flex-1">
                      <div className="font-bold text-slate-100 truncate">Fitness & Nutrition</div>
                      <p className="text-[11px] text-slate-400 truncate">Updated workout plan...</p>
                    </div>
                  </div>

                  <div className="p-2 rounded-xl hover:bg-slate-900/60 border border-transparent cursor-pointer transition-all flex items-start gap-2">
                    <div className="w-7 h-7 rounded-full bg-amber-500/20 text-amber-400 flex items-center justify-center shrink-0 font-bold text-xs">
                      ✉️
                    </div>
                    <div className="min-w-0 flex-1">
                      <div className="font-bold text-slate-100 truncate">Email Template</div>
                      <p className="text-[11px] text-slate-400 truncate">Here's 3 options...</p>
                    </div>
                  </div>

                  <div className="p-2 rounded-xl hover:bg-slate-900/60 border border-transparent cursor-pointer transition-all flex items-start gap-2">
                    <div className="w-7 h-7 rounded-full bg-rose-500/20 text-rose-400 flex items-center justify-center shrink-0 font-bold text-xs">
                      🔬
                    </div>
                    <div className="min-w-0 flex-1">
                      <div className="font-bold text-slate-100 truncate">Research: AI Trends</div>
                      <p className="text-[11px] text-slate-400 truncate">Key takeaways inside...</p>
                    </div>
                  </div>
                </div>
              </div>

              {/* Group This Week */}
              <div>
                <div className="text-[10px] font-mono uppercase text-slate-500 mb-1.5 px-1">This Week</div>
                <div className="space-y-1">
                  <div className="p-2 rounded-xl hover:bg-slate-900/60 border border-transparent cursor-pointer transition-all flex items-start gap-2">
                    <div className="w-7 h-7 rounded-full bg-blue-500/20 text-blue-400 flex items-center justify-center shrink-0 font-bold text-xs">
                      💰
                    </div>
                    <div className="min-w-0 flex-1">
                      <div className="flex items-center justify-between">
                        <span className="font-bold text-slate-100 truncate">Personal Finance</span>
                        <span className="text-[10px] text-slate-500">Mon</span>
                      </div>
                      <p className="text-[11px] text-slate-400 truncate">Budget review and tips...</p>
                    </div>
                  </div>

                  <div className="p-2 rounded-xl hover:bg-slate-900/60 border border-transparent cursor-pointer transition-all flex items-start gap-2">
                    <div className="w-7 h-7 rounded-full bg-indigo-500/20 text-indigo-400 flex items-center justify-center shrink-0 font-bold text-xs">
                      💡
                    </div>
                    <div className="min-w-0 flex-1">
                      <div className="flex items-center justify-between">
                        <span className="font-bold text-slate-100 truncate">App Idea</span>
                        <span className="text-[10px] text-slate-500">Mon</span>
                      </div>
                      <p className="text-[11px] text-slate-400 truncate">Let's break this down...</p>
                    </div>
                  </div>

                  <div className="p-2 rounded-xl hover:bg-slate-900/60 border border-transparent cursor-pointer transition-all flex items-start gap-2">
                    <div className="w-7 h-7 rounded-full bg-teal-500/20 text-teal-400 flex items-center justify-center shrink-0 font-bold text-xs">
                      🏡
                    </div>
                    <div className="min-w-0 flex-1">
                      <div className="flex items-center justify-between">
                        <span className="font-bold text-slate-100 truncate">Family Plans</span>
                        <span className="text-[10px] text-slate-500">Mon</span>
                      </div>
                      <p className="text-[11px] text-slate-400 truncate">Here are some ideas...</p>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </section>

        {/* COLUMN 3: CENTER MAIN STAGE (Sarah Photorealistic Hoodie Portrait + Chat Bubble + Action Pills) */}
        <main className="flex-1 flex flex-col justify-between bg-gradient-to-b from-[#090f20] to-[#050811] rounded-2xl border border-slate-800/90 overflow-hidden shadow-2xl relative min-w-0">
          
          {/* Main Top Header: Marketing Plan Q1 • Project: Business • Memory */}
          <div className="p-3.5 border-b border-slate-800/80 bg-[#070b14]/80 backdrop-blur-md flex items-center justify-between shrink-0 z-10">
            <div className="flex items-center gap-3">
              <div>
                <div className="text-xs font-bold text-slate-100 uppercase tracking-wide flex items-center gap-2">
                  <span>Marketing Plan Q1</span>
                </div>
                <div className="flex items-center gap-2 text-[10px] text-slate-400 mt-0.5">
                  <span className="px-1.5 py-0.5 rounded bg-slate-800/80 text-slate-300 font-mono">
                    Project: Business
                  </span>
                  <span className="flex items-center gap-1 text-cyan-400">
                    <ShieldCheck className="w-3 h-3" />
                    <span>Memory: Global + Project + This Chat</span>
                  </span>
                </div>
              </div>
            </div>

            <div className="flex items-center gap-2">
              <button 
                onClick={onOpenVideoCallModal}
                className="px-2.5 py-1 rounded-xl bg-gradient-to-r from-cyan-600 to-indigo-600 hover:from-cyan-500 hover:to-indigo-500 text-white text-xs font-bold shadow-md shadow-cyan-600/30 transition-all flex items-center gap-1.5"
              >
                <Video className="w-3.5 h-3.5" />
                <span>Video Call</span>
              </button>
            </div>
          </div>

          {/* Central Portrait & Conversational Area */}
          <div className="flex-1 overflow-y-auto p-4 flex flex-col justify-between items-center relative z-10 no-scrollbar">
            
            {/* The Large Photorealistic Avatar: Sarah with hoodie, chin on hand */}
            <div className="w-full max-w-md my-auto flex flex-col items-center text-center relative group">
              <div className="relative w-48 h-48 sm:w-56 sm:h-56 md:w-64 md:h-64 rounded-3xl overflow-hidden ring-4 ring-indigo-500/30 shadow-2xl shadow-indigo-950/60 mb-4 transition-transform duration-300 group-hover:scale-[1.02]">
                <img
                  src={oeSarahImg}
                  alt="Sarah AI"
                  className="w-full h-full object-cover"
                />
                <div className="absolute inset-0 bg-gradient-to-t from-[#050811]/70 via-transparent to-transparent" />
                <div className="absolute bottom-2.5 left-3 flex items-center gap-2">
                  <span className="w-2.5 h-2.5 rounded-full bg-emerald-400 animate-pulse ring-2 ring-black" />
                  <span className="text-xs font-bold text-white tracking-wide drop-shadow">Sarah • OE Lead</span>
                </div>
                <div className="absolute top-2.5 right-2.5 px-2 py-0.5 rounded-full bg-black/60 backdrop-blur-md border border-white/10 text-[10px] font-mono text-cyan-300">
                  {activeModel.name}
                </div>
              </div>

              {/* Chat Speech Bubble */}
              <div className="w-full max-w-sm space-y-2">
                {chatMessages.map((msg, i) => (
                  <div
                    key={i}
                    className={`p-3.5 rounded-2xl text-xs leading-relaxed text-left transition-all ${
                      msg.sender === 'alex'
                        ? 'bg-indigo-600 text-white rounded-tr-none ml-auto max-w-[85%] shadow-md shadow-indigo-600/20'
                        : 'bg-slate-900/90 border border-slate-800 text-slate-100 rounded-tl-none mr-auto shadow-md backdrop-blur-md whitespace-pre-line'
                    }`}
                  >
                    <div className="text-[10px] text-slate-400 font-mono mb-1">
                      {msg.sender === 'alex' ? 'Alex' : 'Sarah'} • {msg.time}
                    </div>
                    {msg.text}
                  </div>
                ))}

                {isAiResponding && (
                  <div className="flex items-center gap-2 p-2 rounded-xl bg-slate-900/60 border border-slate-800 text-xs text-indigo-400 animate-pulse">
                    <Sparkles className="w-3.5 h-3.5 animate-spin" />
                    <span>Sarah is formulating the plan...</span>
                  </div>
                )}
              </div>
            </div>

            {/* Quick Action Chips: Search, Create, Analyze, Plan, Build, More */}
            <div className="w-full max-w-lg flex flex-wrap items-center justify-center gap-2 py-2">
              {[
                { label: 'Search', icon: Search, prompt: "Search latest market benchmarks and competitor strategies." },
                { label: 'Create', icon: Edit3, prompt: "Create compelling copy and creative briefs for Q1." },
                { label: 'Analyze', icon: BarChart3, prompt: "Analyze customer conversion funnels and churn metrics." },
                { label: 'Plan', icon: Calendar, prompt: "Draft an 8-week structured roadmap with milestones." },
                { label: 'Build', icon: Hammer, prompt: "Build interactive prototypes and frontend components." },
                { label: 'More', icon: MoreHorizontal, prompt: "Show available tools, automations, and agent teammates." }
              ].map(chip => {
                const Icon = chip.icon;
                return (
                  <button
                    key={chip.label}
                    onClick={() => handleSendMessage(chip.prompt)}
                    className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-[#080d1a] hover:bg-indigo-950/80 border border-slate-800 hover:border-indigo-600/50 text-slate-300 hover:text-white text-xs font-medium transition-all shadow-sm active:scale-95"
                  >
                    <Icon className="w-3.5 h-3.5 text-cyan-400" />
                    <span>{chip.label}</span>
                  </button>
                );
              })}
            </div>
          </div>

          {/* Bottom Message Input Bar: "+ Message OE... [mic] [send]" */}
          <div className="p-3 border-t border-slate-800/80 bg-[#070b14]/90 backdrop-blur-md shrink-0">
            <form
              onSubmit={(e) => {
                e.preventDefault();
                handleSendMessage();
              }}
              className="flex items-center gap-2 bg-[#050811] p-1.5 rounded-2xl border border-slate-800/90 focus-within:border-cyan-500/70 transition-colors shadow-inner"
            >
              <button
                type="button"
                className="p-2 rounded-xl text-slate-400 hover:text-slate-200 hover:bg-slate-800/60 transition-colors"
                title="Add attachment or context"
              >
                <Plus className="w-4 h-4" />
              </button>

              <input
                type="text"
                value={inputMessage}
                onChange={(e) => setInputMessage(e.target.value)}
                placeholder="Message OE..."
                className="flex-1 bg-transparent px-2 text-xs text-slate-100 placeholder:text-slate-500 focus:outline-none"
              />

              <button
                type="button"
                className="p-2 rounded-xl text-slate-400 hover:text-slate-200 hover:bg-slate-800/60 transition-colors"
                title="Voice input"
              >
                <Mic className="w-4 h-4" />
              </button>

              <button
                type="submit"
                disabled={!inputMessage.trim() || isAiResponding}
                className="p-2 rounded-xl bg-gradient-to-r from-cyan-600 to-indigo-600 hover:from-cyan-500 hover:to-indigo-500 disabled:opacity-40 text-white shadow-md shadow-cyan-600/30 transition-all active:scale-95"
              >
                <Send className="w-4 h-4" />
              </button>
            </form>
          </div>
        </main>

        {/* COLUMN 4: CALLS & MESSAGES LIST */}
        <section className="w-full xl:w-64 shrink-0 bg-[#070b14]/90 rounded-2xl border border-slate-800/80 p-3 flex flex-col justify-between">
          <div className="space-y-3">
            {/* Header: Calls & Messages */}
            <div className="flex items-center justify-between">
              <h2 className="text-sm font-bold text-slate-100">Calls & Messages</h2>
              <div className="flex items-center gap-1 text-slate-400">
                <Search className="w-3.5 h-3.5 hover:text-white cursor-pointer" />
                <Filter className="w-3.5 h-3.5 hover:text-white cursor-pointer" />
                <MoreHorizontal className="w-3.5 h-3.5 hover:text-white cursor-pointer" />
              </div>
            </div>

            {/* Tabs: Messages, Calls, Contacts */}
            <div className="flex items-center gap-1 bg-[#050811] p-1 rounded-xl border border-slate-800 text-[11px]">
              {(['messages', 'calls', 'contacts'] as const).map(tab => (
                <button
                  key={tab}
                  onClick={() => setCallsTab(tab)}
                  className={`flex-1 py-1 rounded-lg font-medium capitalize transition-all ${
                    callsTab === tab 
                      ? 'bg-slate-800 text-white shadow-sm' 
                      : 'text-slate-400 hover:text-slate-200'
                  }`}
                >
                  {tab}
                </button>
              ))}
            </div>

            {/* Contact list matching Image 2 */}
            <div className="space-y-1 overflow-y-auto max-h-[520px] no-scrollbar text-xs pr-1">
              {[
                { name: 'Sarah', sub: 'Sounds good! See you then.', time: '10:42 AM', unread: 2, avatar: oeSarahImg, active: true },
                { name: 'Marketing Team', sub: "Lisa: Here's the latest update...", time: '9:18 AM', unread: 8, avatar: '💼' },
                { name: 'OE Assistant', sub: 'Call ended • 12 min', time: '8:56 AM', avatar: '🤖' },
                { name: 'Dad', sub: 'Thanks, love you too!', time: 'Yesterday', avatar: '👨' },
                { name: 'Project Alpha', sub: 'You: File shared', time: 'Yesterday', avatar: '📁' },
                { name: 'Emma', sub: 'Video call • 28 min', time: 'Mon', avatar: '👩' },
                { name: 'Design Team', sub: 'New assets are ready.', time: 'Mon', avatar: '👥' },
                { name: 'James', sub: "Let's catch up this week.", time: 'Mon', avatar: '🧔' },
                { name: 'AI Dev Agent', sub: 'Task completed ✅', time: 'Sun', avatar: '⚡' },
                { name: 'Family Group', sub: 'Dinner at 7? 🍕', time: 'Sun', avatar: '🏠' }
              ].map((contact, idx) => (
                <div
                  key={idx}
                  onClick={() => {
                    if (contact.name === 'Sarah') {
                      setIsInCall(true);
                    }
                  }}
                  className={`p-2 rounded-xl cursor-pointer transition-all flex items-center justify-between ${
                    contact.active 
                      ? 'bg-indigo-950/50 border border-indigo-700/40 shadow-sm' 
                      : 'hover:bg-slate-900/60 border border-transparent'
                  }`}
                >
                  <div className="flex items-center gap-2 min-w-0">
                    {typeof contact.avatar === 'string' && contact.avatar.length <= 2 ? (
                      <div className="w-8 h-8 rounded-full bg-slate-800 text-sm flex items-center justify-center shrink-0">
                        {contact.avatar}
                      </div>
                    ) : (
                      <img src={contact.avatar} alt={contact.name} className="w-8 h-8 rounded-full object-cover shrink-0 ring-1 ring-cyan-500/30" />
                    )}
                    <div className="min-w-0">
                      <div className="font-bold text-slate-100 truncate">{contact.name}</div>
                      <div className="text-[10px] text-slate-400 truncate">{contact.sub}</div>
                    </div>
                  </div>

                  <div className="flex flex-col items-end shrink-0 pl-1">
                    <span className="text-[10px] text-slate-500">{contact.time}</span>
                    {contact.unread && (
                      <span className="text-[9px] w-4 h-4 rounded-full bg-emerald-500 text-white font-bold flex items-center justify-center mt-0.5">
                        {contact.unread}
                      </span>
                    )}
                  </div>
                </div>
              ))}
            </div>
          </div>
        </section>

        {/* COLUMN 5 / RIGHT SIDEBAR: ACTIVE VIDEO CALL WITH SARAH */}
        {isInCall && (
          <aside className="w-full xl:w-72 shrink-0 bg-[#060a14] rounded-2xl border border-slate-800/90 overflow-hidden flex flex-col justify-between shadow-2xl relative">
            
            {/* Top Bar: OE Brand + End-to-end encrypted */}
            <div className="p-3 bg-gradient-to-b from-black/80 to-transparent flex items-center justify-between text-xs absolute top-0 left-0 right-0 z-20">
              <div className="flex items-center gap-1.5 font-black text-cyan-400 tracking-tight">
                <span>OE</span>
              </div>
              <div className="flex items-center gap-1 text-[10px] text-emerald-400 bg-black/60 px-2 py-0.5 rounded-full border border-emerald-500/30">
                <Lock className="w-2.5 h-2.5" />
                <span>End-to-end encrypted</span>
              </div>
            </div>

            {/* Video Call Full Body Stage */}
            <div className="relative flex-1 min-h-[360px] bg-slate-950 flex items-center justify-center overflow-hidden">
              <img
                src={avatarVideoCallImg}
                alt="Sarah Video Call"
                className="w-full h-full object-cover object-top filter brightness-105"
              />
              <div className="absolute inset-0 bg-gradient-to-t from-black/90 via-transparent to-black/40" />

              {/* Sarah Call Info Overlay */}
              <div className="absolute bottom-16 left-3 text-left">
                <div className="text-sm font-extrabold text-white tracking-wide drop-shadow">
                  Sarah
                </div>
                <div className="text-[11px] font-mono text-cyan-300 flex items-center gap-1.5">
                  <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
                  <span>{callTimer}</span>
                </div>
              </div>

              {/* Floating User PiP: Alex Smiling */}
              <div className="absolute top-12 right-3 w-20 h-24 rounded-2xl overflow-hidden ring-2 ring-indigo-500 shadow-2xl z-20 group">
                <img
                  src={userPipImg}
                  alt="Alex PiP"
                  className="w-full h-full object-cover"
                />
                <div className="absolute bottom-1 right-1 px-1 rounded bg-black/60 text-[8px] font-mono text-white">
                  You
                </div>
              </div>
            </div>

            {/* Call Action Bar: Video, Mic, End red button, Volume, Camera Flip */}
            <div className="p-3 bg-slate-950/95 border-t border-slate-800/80 flex items-center justify-around z-20">
              <button
                onClick={() => setIsVideoStopped(!isVideoStopped)}
                className={`p-2 rounded-xl transition-all ${
                  isVideoStopped ? 'bg-rose-500/20 text-rose-300' : 'bg-slate-900 text-slate-300 hover:text-white'
                }`}
                title="Toggle Video"
              >
                {isVideoStopped ? <VideoOff className="w-4 h-4" /> : <Video className="w-4 h-4" />}
              </button>

              <button
                onClick={() => setIsMicMuted(!isMicMuted)}
                className={`p-2 rounded-xl transition-all ${
                  isMicMuted ? 'bg-rose-500/20 text-rose-300' : 'bg-slate-900 text-slate-300 hover:text-white'
                }`}
                title="Toggle Mic"
              >
                {isMicMuted ? <MicOff className="w-4 h-4" /> : <Mic className="w-4 h-4" />}
              </button>

              <button
                onClick={() => setIsInCall(false)}
                className="p-3 rounded-2xl bg-rose-600 hover:bg-rose-500 text-white shadow-lg shadow-rose-600/40 transition-all active:scale-95"
                title="End Call"
              >
                <PhoneOff className="w-4 h-4" />
              </button>

              <button
                onClick={() => setIsSpeakerMuted(!isSpeakerMuted)}
                className="p-2 rounded-xl bg-slate-900 text-slate-300 hover:text-white transition-colors"
                title="Toggle Speaker"
              >
                {isSpeakerMuted ? <VolumeX className="w-4 h-4" /> : <Volume2 className="w-4 h-4" />}
              </button>

              <button
                className="p-2 rounded-xl bg-slate-900 text-slate-300 hover:text-white transition-colors"
                title="Camera switch"
              >
                <Camera className="w-4 h-4" />
              </button>
            </div>
          </aside>
        )}
      </div>

      {/* 3. BOTTOM BENTO SUITE: MEMORY • FREEMIUM PLANS • REFERRAL • PRIVACY */}
      <div className="p-3 pt-0">
        <div className="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-4 gap-3">
          
          {/* Card 1: Multiple Chat Threads One Shared Memory */}
          <div className="p-4 rounded-2xl bg-[#070b14]/90 border border-slate-800/80 space-y-3">
            <div className="flex items-center gap-2">
              <div className="p-1.5 rounded-lg bg-indigo-500/20 text-indigo-400">
                <Layers className="w-4 h-4" />
              </div>
              <div>
                <h3 className="text-xs font-bold text-slate-100">Multiple Chat Threads</h3>
                <p className="text-[10px] text-cyan-400 font-medium">One Shared Memory</p>
              </div>
            </div>

            <div className="space-y-1.5 text-[11px]">
              <div className="flex items-start gap-2 p-1.5 rounded-lg bg-slate-900/60">
                <span className="text-indigo-400 font-bold">🌐</span>
                <div>
                  <div className="font-semibold text-slate-200">Global Memory</div>
                  <div className="text-[10px] text-slate-400">Your preferences, identity, long-term knowledge.</div>
                </div>
              </div>

              <div className="flex items-start gap-2 p-1.5 rounded-lg bg-slate-900/60">
                <span className="text-cyan-400 font-bold">📁</span>
                <div>
                  <div className="font-semibold text-slate-200">Project Memory</div>
                  <div className="text-[10px] text-slate-400">Everything for each project or workspace.</div>
                </div>
              </div>

              <div className="flex items-start gap-2 p-1.5 rounded-lg bg-slate-900/60">
                <span className="text-purple-400 font-bold">💬</span>
                <div>
                  <div className="font-semibold text-slate-200">Thread Memory</div>
                  <div className="text-[10px] text-slate-400">Specific to this conversation.</div>
                </div>
              </div>

              <div className="flex items-start gap-2 p-1.5 rounded-lg bg-slate-900/60">
                <span className="text-amber-400 font-bold">🤖</span>
                <div>
                  <div className="font-semibold text-slate-200">Agent Memory</div>
                  <div className="text-[10px] text-slate-400">What your AI workers know (and can share).</div>
                </div>
              </div>
            </div>
          </div>

          {/* Card 2: Free & Freemium Plans */}
          <div className="p-4 rounded-2xl bg-[#070b14]/90 border border-slate-800/80 space-y-3">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2">
                <div className="p-1.5 rounded-lg bg-cyan-500/20 text-cyan-400">
                  <Award className="w-4 h-4" />
                </div>
                <div>
                  <h3 className="text-xs font-bold text-slate-100">Free & Freemium Plans</h3>
                  <p className="text-[10px] text-slate-400">Start free. Grow anytime.</p>
                </div>
              </div>
            </div>

            <div className="grid grid-cols-3 gap-1.5 text-center text-xs">
              <div className="p-2 rounded-xl bg-slate-900/80 border border-slate-800 flex flex-col justify-between">
                <div>
                  <div className="font-bold text-slate-300 text-[11px]">Free</div>
                  <div className="font-extrabold text-white text-base my-0.5">$0</div>
                  <div className="text-[9px] text-slate-400">forever</div>
                </div>
                <button className="mt-2 py-1 px-1 rounded-lg bg-slate-800 hover:bg-slate-700 text-white text-[10px] font-semibold">
                  Get Started
                </button>
              </div>

              <div className="p-2 rounded-xl bg-indigo-950/60 border border-indigo-500/50 flex flex-col justify-between relative shadow-sm">
                <div className="absolute -top-2 left-1/2 -translate-x-1/2 px-1.5 py-0.2 rounded-full bg-cyan-500 text-black font-extrabold text-[8px]">
                  POPULAR
                </div>
                <div>
                  <div className="font-bold text-cyan-300 text-[11px]">Plus</div>
                  <div className="font-extrabold text-white text-base my-0.5">$19</div>
                  <div className="text-[9px] text-slate-400">/ month</div>
                </div>
                <button className="mt-2 py-1 px-1 rounded-lg bg-gradient-to-r from-cyan-600 to-indigo-600 text-white text-[10px] font-bold">
                  Upgrade
                </button>
              </div>

              <div className="p-2 rounded-xl bg-slate-900/80 border border-slate-800 flex flex-col justify-between">
                <div>
                  <div className="font-bold text-amber-300 text-[11px]">Pro</div>
                  <div className="font-extrabold text-white text-base my-0.5">$49</div>
                  <div className="text-[9px] text-slate-400">/ month</div>
                </div>
                <button className="mt-2 py-1 px-1 rounded-lg bg-amber-600 hover:bg-amber-500 text-white text-[10px] font-semibold">
                  Go Pro
                </button>
              </div>
            </div>

            <div className="text-[10px] text-slate-400 flex items-center justify-between px-1">
              <span>✓ Advanced models</span>
              <span>✓ Unlimited agents</span>
              <span>✓ Priority tools</span>
            </div>
          </div>

          {/* Card 3: Referral & Rewards */}
          <div className="p-4 rounded-2xl bg-[#070b14]/90 border border-slate-800/80 space-y-3">
            <div className="flex items-center gap-2">
              <div className="p-1.5 rounded-lg bg-purple-500/20 text-purple-400">
                <Gift className="w-4 h-4" />
              </div>
              <div>
                <h3 className="text-xs font-bold text-slate-100">Referral & Rewards</h3>
                <p className="text-[10px] text-slate-400">Invite. Earn. Grow Together.</p>
              </div>
            </div>

            <div className="p-2 rounded-xl bg-purple-950/30 border border-purple-800/30 text-xs">
              <div className="font-extrabold text-purple-300">Give $10, Get $10</div>
              <div className="text-[10px] text-slate-400">Invite friends and you both get rewards when they join.</div>
            </div>

            {/* Referral Link Box */}
            <div className="flex items-center gap-1.5 bg-[#050811] p-1.5 rounded-xl border border-slate-800">
              <span className="text-[10px] font-mono text-cyan-300 flex-1 truncate px-1">
                https://oe.com/invite/ALEX
              </span>
              <button
                onClick={handleCopyInvite}
                className="p-1 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 transition-colors shrink-0"
                title="Copy link"
              >
                {copiedLink ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
              </button>
            </div>

            {/* Stats */}
            <div className="grid grid-cols-3 gap-1 text-center text-xs">
              <div className="p-1.5 rounded-lg bg-slate-900/60">
                <div className="font-bold text-white">12</div>
                <div className="text-[9px] text-slate-400">Invited</div>
              </div>
              <div className="p-1.5 rounded-lg bg-slate-900/60">
                <div className="font-bold text-emerald-400">8</div>
                <div className="text-[9px] text-slate-400">Joined</div>
              </div>
              <div className="p-1.5 rounded-lg bg-slate-900/60">
                <div className="font-bold text-amber-400">$80</div>
                <div className="text-[9px] text-slate-400">Earned</div>
              </div>
            </div>
          </div>

          {/* Card 4: Secure. Private. Yours. */}
          <div className="p-4 rounded-2xl bg-[#070b14]/90 border border-slate-800/80 space-y-3 flex flex-col justify-between">
            <div>
              <div className="flex items-center gap-2 mb-2">
                <div className="p-1.5 rounded-lg bg-emerald-500/20 text-emerald-400">
                  <ShieldCheck className="w-4 h-4" />
                </div>
                <div>
                  <h3 className="text-xs font-bold text-slate-100">Secure. Private. Yours.</h3>
                  <p className="text-[10px] text-emerald-400">Built with privacy by design</p>
                </div>
              </div>

              <div className="space-y-1 text-[11px] text-slate-300">
                <div className="flex items-center gap-1.5">
                  <span className="text-emerald-400 font-bold">✓</span>
                  <span>End-to-end encrypted chats & calls</span>
                </div>
                <div className="flex items-center gap-1.5">
                  <span className="text-emerald-400 font-bold">✓</span>
                  <span>You control your data & memory retention</span>
                </div>
                <div className="flex items-center gap-1.5">
                  <span className="text-emerald-400 font-bold">✓</span>
                  <span>Private / No Memory mode toggle</span>
                </div>
                <div className="flex items-center gap-1.5">
                  <span className="text-emerald-400 font-bold">✓</span>
                  <span>Multi-device real-time secure sync</span>
                </div>
              </div>
            </div>

            <button className="w-full py-1.5 rounded-xl bg-slate-900 hover:bg-slate-800 border border-slate-800 text-xs font-semibold text-slate-200 transition-colors">
              Learn More
            </button>
          </div>

        </div>

        {/* 4. FOOTER: Available Everywhere • OE Logo & Avatars Strip */}
        <footer className="mt-3 p-3 rounded-2xl bg-[#060a14] border border-slate-800/80 flex flex-col sm:flex-row items-center justify-between gap-3 text-xs text-slate-400">
          <div className="flex items-center gap-3">
            <span className="text-[11px] font-mono text-slate-500">Available Everywhere:</span>
            <div className="flex items-center gap-2 text-slate-300 font-semibold">
              <span>🍏 iOS</span>
              <span>•</span>
              <span>🤖 Android</span>
              <span>•</span>
              <span>🪟 Windows</span>
              <span>•</span>
              <span>🍎 macOS</span>
              <span>•</span>
              <span>🌐 Web</span>
            </div>
          </div>

          <div className="text-[11px] text-slate-400 font-medium">
            More Than an App. <span className="text-cyan-400">A Kinder, Smarter, More Human Future.</span>
          </div>

          <div className="flex items-center gap-2">
            <span className="text-[10px] text-slate-500 hidden md:inline">Different Skills. A Shared Purpose.</span>
            <div className="flex -space-x-1.5 overflow-hidden">
              <img src={oeSarahImg} alt="1" className="w-6 h-6 rounded-full object-cover ring-2 ring-[#070b14]" />
              <img src={userPipImg} alt="2" className="w-6 h-6 rounded-full object-cover ring-2 ring-[#070b14]" />
              <img src={avatarVideoCallImg} alt="3" className="w-6 h-6 rounded-full object-cover ring-2 ring-[#070b14]" />
            </div>
          </div>
        </footer>
      </div>

    </div>
  );
};
