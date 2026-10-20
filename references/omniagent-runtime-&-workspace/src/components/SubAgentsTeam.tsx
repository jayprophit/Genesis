import React, { useState } from 'react';
import { 
  Users, 
  Play, 
  Pause, 
  CheckCircle, 
  AlertTriangle, 
  MessageSquare, 
  Sparkles, 
  Mic, 
  Volume2, 
  VolumeX, 
  Clock, 
  Send,
  Zap,
  Activity,
  Layers,
  ArrowRight
} from 'lucide-react';
import { SubAgent, InterAgentMessage } from '../types';

interface SubAgentsTeamProps {
  subAgents: SubAgent[];
  interAgentMessages: InterAgentMessage[];
  onAgentAction: (agentId: string, action: 'pause' | 'resume' | 'approve') => void;
  onSendInterAgentMessage: (from: string, to: string, content: string) => void;
}

export const SubAgentsTeam: React.FC<SubAgentsTeamProps> = ({
  subAgents,
  interAgentMessages,
  onAgentAction,
  onSendInterAgentMessage
}) => {
  const [silentMode, setSilentMode] = useState(false);
  const [pushToTalk, setPushToTalk] = useState(false);
  const [selectedAgent, setSelectedAgent] = useState<SubAgent>(subAgents[0]);
  const [newMessage, setNewMessage] = useState('');
  const [sendToAgentId, setSendToAgentId] = useState('dev');

  const handlePostMessage = (e: React.FormEvent) => {
    e.preventDefault();
    if (!newMessage.trim()) return;
    onSendInterAgentMessage('You (Admin)', subAgents.find(a => a.id === sendToAgentId)?.name || 'Dev Engineering', newMessage);
    setNewMessage('');
  };

  return (
    <div className="flex-1 p-6 overflow-y-auto space-y-6">
      {/* Top Banner & Swarm Controls */}
      <div className="flex flex-col md:flex-row md:items-center justify-between gap-4 p-5 rounded-2xl bg-gradient-to-r from-slate-900 via-slate-900/90 to-indigo-950/40 border border-slate-800">
        <div>
          <div className="flex items-center gap-2">
            <span className="w-2.5 h-2.5 rounded-full bg-emerald-400 animate-pulse" />
            <h2 className="text-base font-extrabold text-slate-100 uppercase tracking-tight">
              Parallel Sub-Agent Swarm
            </h2>
            <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-indigo-500/20 text-indigo-300 border border-indigo-500/30">
              6 Agents Online
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-1">
            Autonomous multi-agent orchestration inspired by NOVA AI Team & Claude Cowork architecture.
          </p>
        </div>

        <div className="flex items-center gap-2.5">
          {/* Silent Mode toggle (from slide 13) */}
          <button
            onClick={() => setSilentMode(!silentMode)}
            className={`flex items-center gap-1.5 px-3 py-1.5 rounded-xl text-xs font-semibold border transition-all ${
              silentMode
                ? 'bg-slate-800 text-slate-400 border-slate-700'
                : 'bg-indigo-600/20 text-indigo-200 border-indigo-500/40'
            }`}
          >
            {silentMode ? <VolumeX className="w-3.5 h-3.5 text-slate-400" /> : <Volume2 className="w-3.5 h-3.5 text-indigo-400" />}
            <span>{silentMode ? 'Silent Mode (Text Only)' : 'Audio Feed On'}</span>
          </button>

          {/* Push to talk toggle */}
          <button
            onClick={() => setPushToTalk(!pushToTalk)}
            className={`flex items-center gap-1.5 px-3 py-1.5 rounded-xl text-xs font-semibold border transition-all ${
              pushToTalk
                ? 'bg-emerald-500/20 text-emerald-300 border-emerald-500/40'
                : 'bg-slate-900 text-slate-400 border-slate-800'
            }`}
          >
            <Mic className="w-3.5 h-3.5" />
            <span>Push-To-Talk</span>
          </button>
        </div>
      </div>

      {/* Main Grid: 6 Sub-Agents Cards (NOVA Team Layout) */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        {subAgents.map((agent) => {
          const isSelected = selectedAgent.id === agent.id;
          const statusColors: Record<SubAgent['status'], { badge: string; text: string; dot: string }> = {
            running: { badge: 'bg-emerald-500/10 border-emerald-500/30 text-emerald-300', text: 'Running Task', dot: 'bg-emerald-400 animate-ping' },
            idle: { badge: 'bg-slate-800 border-slate-700 text-slate-400', text: 'Idle / Ready', dot: 'bg-slate-500' },
            waiting_approval: { badge: 'bg-amber-500/15 border-amber-500/30 text-amber-300', text: 'Waiting Approval', dot: 'bg-amber-400 animate-bounce' },
            done: { badge: 'bg-indigo-500/10 border-indigo-500/30 text-indigo-300', text: 'Complete', dot: 'bg-indigo-400' }
          };

          const currentStatus = statusColors[agent.status] || statusColors.idle;

          return (
            <div
              key={agent.id}
              onClick={() => setSelectedAgent(agent)}
              className={`p-4 rounded-2xl border transition-all cursor-pointer flex flex-col justify-between ${
                isSelected
                  ? 'bg-slate-900/90 border-indigo-500/60 shadow-lg shadow-indigo-500/10 ring-1 ring-indigo-500/30'
                  : 'bg-slate-900/40 border-slate-800/80 hover:bg-slate-900/70 hover:border-slate-700'
              }`}
            >
              <div>
                {/* Agent Header */}
                <div className="flex items-start justify-between gap-3 mb-3">
                  <div className="flex items-center gap-3">
                    <div className="relative">
                      <img
                        src={agent.avatar}
                        alt={agent.name}
                        className="w-12 h-12 rounded-xl object-cover ring-1 ring-indigo-500/40"
                      />
                      <span className={`absolute -bottom-0.5 -right-0.5 w-3 h-3 rounded-full ring-2 ring-slate-900 ${currentStatus.dot}`} />
                    </div>
                    <div>
                      <h3 className="text-xs font-bold text-slate-100">{agent.name}</h3>
                      <p className="text-[11px] text-indigo-400 font-medium">{agent.role}</p>
                      <p className="text-[10px] text-slate-500">{agent.messagesCount} actions dispatched</p>
                    </div>
                  </div>

                  <span className={`text-[10px] font-mono px-2 py-0.5 rounded-full border ${currentStatus.badge}`}>
                    {currentStatus.text}
                  </span>
                </div>

                {/* Current Task Description */}
                <div className="p-2.5 rounded-xl bg-slate-950/70 border border-slate-800/80 mb-3 text-xs">
                  <div className="text-[10px] text-slate-400 font-mono flex items-center justify-between mb-1">
                    <span>Active Goal:</span>
                    <span>{agent.progress}%</span>
                  </div>
                  <p className="text-slate-200 text-[11px] line-clamp-2 leading-relaxed">
                    {agent.currentTask}
                  </p>

                  {/* Progress Bar */}
                  <div className="w-full h-1.5 bg-slate-800 rounded-full mt-2 overflow-hidden">
                    <div
                      className="h-full bg-gradient-to-r from-indigo-500 to-cyan-400 rounded-full transition-all duration-500"
                      style={{ width: `${agent.progress}%` }}
                    />
                  </div>
                </div>

                {/* Assigned Skills */}
                <div className="flex flex-wrap gap-1 mb-3">
                  {agent.skills.map((skill, idx) => (
                    <span key={idx} className="text-[9px] font-mono px-1.5 py-0.5 rounded bg-slate-800 text-slate-300">
                      {skill}
                    </span>
                  ))}
                </div>
              </div>

              {/* Action Buttons */}
              <div className="flex items-center justify-between pt-2 border-t border-slate-800/80">
                <span className="text-[10px] text-slate-500 truncate max-w-[140px]">
                  {agent.lastAction}
                </span>

                <div className="flex items-center gap-1.5">
                  {agent.status === 'waiting_approval' && (
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        onAgentAction(agent.id, 'approve');
                      }}
                      className="px-2.5 py-1 rounded-lg bg-amber-500 hover:bg-amber-400 text-slate-950 font-bold text-[11px] transition-colors"
                    >
                      Approve
                    </button>
                  )}

                  {agent.status === 'running' ? (
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        onAgentAction(agent.id, 'pause');
                      }}
                      className="p-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300"
                      title="Pause agent"
                    >
                      <Pause className="w-3.5 h-3.5" />
                    </button>
                  ) : (
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        onAgentAction(agent.id, 'resume');
                      }}
                      className="p-1.5 rounded-lg bg-indigo-600/30 hover:bg-indigo-600/50 text-indigo-300"
                      title="Start agent"
                    >
                      <Play className="w-3.5 h-3.5" />
                    </button>
                  )}
                </div>
              </div>
            </div>
          );
        })}
      </div>

      {/* Bottom Section: Inter-Agent Live Chat Stream & Dispatch Box */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Left 2 Cols: Live Inter-Agent Messages */}
        <div className="lg:col-span-2 p-5 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-2">
              <MessageSquare className="w-4 h-4 text-indigo-400" />
              <h3 className="text-xs font-bold text-slate-100 uppercase tracking-wider">
                Inter-Agent Communication Stream (Live Mesh)
              </h3>
            </div>
            <span className="text-[10px] font-mono text-emerald-400">P2P Socket Active</span>
          </div>

          <div className="space-y-2.5 max-h-72 overflow-y-auto pr-1">
            {interAgentMessages.map((msg) => (
              <div key={msg.id} className="p-3 rounded-xl bg-slate-950 border border-slate-800/80 text-xs space-y-1">
                <div className="flex items-center justify-between text-[11px]">
                  <div className="flex items-center gap-1.5">
                    <span className="font-semibold text-indigo-300">{msg.from}</span>
                    <ArrowRight className="w-3 h-3 text-slate-500" />
                    <span className="font-semibold text-cyan-300">{msg.to}</span>
                    {msg.taskRef && (
                      <span className="text-[10px] font-mono px-1 rounded bg-slate-800 text-slate-400">
                        {msg.taskRef}
                      </span>
                    )}
                  </div>
                  <span className="text-slate-500 text-[10px] font-mono">{msg.timestamp}</span>
                </div>
                <p className="text-slate-300 text-[11px] leading-relaxed pl-1">
                  {msg.content}
                </p>
              </div>
            ))}
          </div>

          {/* Quick Inter-Agent Direct Message Box */}
          <form onSubmit={handlePostMessage} className="flex gap-2 pt-2 border-t border-slate-800">
            <select
              value={sendToAgentId}
              onChange={(e) => setSendToAgentId(e.target.value)}
              className="text-xs bg-slate-950 border border-slate-800 rounded-xl px-2.5 py-2 text-slate-300 focus:outline-none"
            >
              {subAgents.map(a => (
                <option key={a.id} value={a.id}>To: {a.name}</option>
              ))}
            </select>
            <input
              type="text"
              value={newMessage}
              onChange={(e) => setNewMessage(e.target.value)}
              placeholder="Inject direct instruction into inter-agent mesh..."
              className="flex-1 bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-xs text-slate-100 placeholder-slate-500 focus:outline-none focus:border-indigo-500"
            />
            <button
              type="submit"
              disabled={!newMessage.trim()}
              className="px-3 py-2 rounded-xl bg-indigo-600 hover:bg-indigo-500 text-white text-xs font-semibold shadow disabled:opacity-50"
            >
              Inject
            </button>
          </form>
        </div>

        {/* Right Col: Conversation Queue & Controls (from slide 13) */}
        <div className="p-5 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-4">
          <div className="flex items-center justify-between">
            <h3 className="text-xs font-bold text-slate-100 uppercase tracking-wider">
              Speaking / Execution Queue
            </h3>
            <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-slate-800 text-slate-400">
              Auto-moderation On
            </span>
          </div>

          <div className="space-y-2">
            {[
              { rank: 1, name: 'You (Admin)', status: 'Next to Speak / Override', active: true },
              { rank: 2, name: 'Atlas (Orchestrator)', status: 'Synthesizing Roadmap', active: false },
              { rank: 3, name: 'Dev Engineering', status: 'AST Code Verification', active: false },
              { rank: 4, name: 'Nova (QA)', status: 'Waiting on Socket Approval', active: false },
              { rank: 5, name: 'Orion (Data)', status: 'Ingesting Vector Batch', active: false }
            ].map((q) => (
              <div
                key={q.rank}
                className={`p-2.5 rounded-xl border text-xs flex items-center justify-between ${
                  q.active
                    ? 'bg-indigo-600/20 border-indigo-500/40 text-indigo-100 font-semibold'
                    : 'bg-slate-950 border-slate-800/80 text-slate-400'
                }`}
              >
                <div className="flex items-center gap-2">
                  <span className="font-mono text-[10px] w-4 text-slate-500">#{q.rank}</span>
                  <span>{q.name}</span>
                </div>
                <span className="text-[10px] font-mono text-slate-400">{q.status}</span>
              </div>
            ))}
          </div>

          <button
            onClick={() => alert("Speaking priority requested. Sub-agent queue paused.")}
            className="w-full py-2 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-semibold border border-slate-700 transition-colors"
          >
            Request to Speak / Pause Swarm
          </button>
        </div>
      </div>
    </div>
  );
};
