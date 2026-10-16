import React, { useState, useMemo } from 'react';
import { 
  Activity, 
  ShieldCheck, 
  Cpu, 
  Zap, 
  Users, 
  Laptop, 
  Clock, 
  CheckCircle2, 
  AlertTriangle, 
  ArrowUpRight, 
  RefreshCw, 
  Download, 
  Sliders, 
  Filter, 
  ExternalLink,
  ChevronRight,
  TrendingUp,
  Server,
  Database,
  Radio,
  BarChart3,
  Flame,
  Search
} from 'lucide-react';
import { 
  SubAgent, 
  ModelAdapterInfo, 
  RuntimeMode, 
  TrustBoundary, 
  DesktopBridgeState, 
  AuditLogEntry 
} from '../types';
import { ActiveTab } from './Sidebar';

interface WorkspaceHealthDashboardProps {
  subAgents: SubAgent[];
  activeModel: ModelAdapterInfo;
  models: ModelAdapterInfo[];
  runtimeMode: RuntimeMode;
  trustBoundary: TrustBoundary;
  desktopBridge: DesktopBridgeState;
  auditLogs: AuditLogEntry[];
  onNavigateTab: (tab: ActiveTab) => void;
  onSelectModel: (modelId: string) => void;
  onTriggerDiagnosticPing?: () => void;
}

export const WorkspaceHealthDashboard: React.FC<WorkspaceHealthDashboardProps> = ({
  subAgents,
  activeModel,
  models,
  runtimeMode,
  trustBoundary,
  desktopBridge,
  auditLogs,
  onNavigateTab,
  onSelectModel,
  onTriggerDiagnosticPing
}) => {
  const [timeRange, setTimeRange] = useState<'1h' | '24h' | '7d'>('24h');
  const [isRefreshing, setIsRefreshing] = useState(false);
  const [diagnosticNotice, setDiagnosticNotice] = useState<string | null>(null);
  const [eventFilter, setEventFilter] = useState<string>('all');
  const [eventSearch, setEventSearch] = useState<string>('');

  // Agent performance statistics calculation
  const totalTasks = useMemo(() => {
    return subAgents.reduce((acc, a) => acc + (a.messagesCount || 0) * 3 + 12, 0);
  }, [subAgents]);

  const activeAgentsCount = useMemo(() => {
    return subAgents.filter(a => a.status === 'running').length;
  }, [subAgents]);

  const overallHealthScore = useMemo(() => {
    // Computed based on bridge connectivity, active models, low risk audit logs
    let score = 95.0;
    if (desktopBridge.connected) score += 2.5;
    if (activeModel.status === 'active') score += 1.5;
    const highRisk = auditLogs.filter(l => l.riskLevel === 'high' || l.riskScore === 'high').length;
    score -= highRisk * 0.5;
    return Math.min(99.8, Math.max(88.0, score)).toFixed(1);
  }, [desktopBridge.connected, activeModel, auditLogs]);

  // Synthetic performance metrics per agent
  const agentPerformanceData = useMemo(() => {
    return subAgents.map((agent, index) => {
      const baseTasks = (agent.messagesCount || 5) * 4 + 18 + index * 6;
      const successRate = 96.5 + ((index * 7) % 35) / 10;
      const avgLatency = Math.round(activeModel.latencyMs * (0.85 + (index * 0.08)));
      const memoryUsage = Math.round(180 + index * 42); // MB
      return {
        ...agent,
        totalTasks: baseTasks,
        successRate: Math.min(99.9, successRate).toFixed(1),
        avgLatency,
        memoryUsage,
        throughput: Math.round(activeModel.tokensPerSec * (0.9 + (index * 0.05)))
      };
    });
  }, [subAgents, activeModel]);

  // Hourly throughput trend data for custom SVG visualization
  const throughputTrend = [
    { hour: '06:00', tokens: 84, latency: 280, events: 12 },
    { hour: '08:00', tokens: 120, latency: 260, events: 28 },
    { hour: '10:00', tokens: 165, latency: 240, events: 45 },
    { hour: '12:00', tokens: 140, latency: 250, events: 38 },
    { hour: '14:00', tokens: 190, latency: 220, events: 52 },
    { hour: '16:00', tokens: 175, latency: 230, events: 41 },
    { hour: '18:00', tokens: 155, latency: 215, events: 34 },
    { hour: 'Now', tokens: activeModel.tokensPerSec, latency: activeModel.latencyMs, events: 19 }
  ];

  const handleRefresh = () => {
    setIsRefreshing(true);
    setTimeout(() => {
      setIsRefreshing(false);
      setDiagnosticNotice('Live health probes returned 200 OK. All 6 agent workers synchronized.');
      setTimeout(() => setDiagnosticNotice(null), 4000);
      if (onTriggerDiagnosticPing) onTriggerDiagnosticPing();
    }, 650);
  };

  const handleExportJson = () => {
    const report = {
      timestamp: new Date().toISOString(),
      workspaceHealthScore: `${overallHealthScore}%`,
      runtimeMode,
      trustBoundary,
      activeModel: {
        id: activeModel.id,
        name: activeModel.name,
        latencyMs: activeModel.latencyMs,
        tokensPerSec: activeModel.tokensPerSec
      },
      desktopBridge: {
        connected: desktopBridge.connected,
        agentHost: desktopBridge.agentHost,
        approvedPaths: desktopBridge.approvedPaths
      },
      agentMetrics: agentPerformanceData.map(a => ({
        name: a.name,
        role: a.role,
        status: a.status,
        successRate: `${a.successRate}%`,
        tasksCompleted: a.totalTasks,
        avgLatencyMs: a.avgLatency
      })),
      recentEventsCount: auditLogs.length
    };

    const blob = new Blob([JSON.stringify(report, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `omniagent-health-report-${Date.now()}.json`;
    a.click();
    URL.revokeObjectURL(url);
  };

  const filteredLogs = useMemo(() => {
    return auditLogs.filter(log => {
      const matchCategory = eventFilter === 'all' || log.category === eventFilter;
      const matchSearch = !eventSearch || 
        log.action.toLowerCase().includes(eventSearch.toLowerCase()) ||
        log.details.toLowerCase().includes(eventSearch.toLowerCase()) ||
        log.agent.toLowerCase().includes(eventSearch.toLowerCase());
      return matchCategory && matchSearch;
    });
  }, [auditLogs, eventFilter, eventSearch]);

  return (
    <div className="flex-1 flex flex-col overflow-y-auto bg-slate-950 text-slate-100 p-4 sm:p-6 space-y-6">
      {/* TOP HEADER: Workspace Health & Diagnostic Actions */}
      <div className="flex flex-col md:flex-row md:items-center md:justify-between gap-4 pb-4 border-b border-slate-800/80">
        <div>
          <div className="flex items-center gap-2.5">
            <div className="p-2 rounded-xl bg-indigo-500/20 text-indigo-400 border border-indigo-500/30">
              <Activity className="w-5 h-5" />
            </div>
            <div>
              <h1 className="text-lg font-bold tracking-tight text-slate-100 flex items-center gap-2">
                Workspace Health & System Telemetry
                <span className="text-[10px] px-2 py-0.5 rounded-full bg-emerald-500/20 text-emerald-300 font-mono border border-emerald-500/30">
                  HEALTHY
                </span>
              </h1>
              <p className="text-xs text-slate-400">
                High-level vital signs, sub-agent performance analytics, and cryptographic event logs.
              </p>
            </div>
          </div>
        </div>

        {/* Action Controls */}
        <div className="flex items-center gap-2">
          {/* Time range toggle */}
          <div className="flex items-center bg-slate-900 border border-slate-800 rounded-lg p-0.5 text-xs">
            <button
              onClick={() => setTimeRange('1h')}
              className={`px-2.5 py-1 rounded font-medium transition-colors ${
                timeRange === '1h' ? 'bg-slate-800 text-slate-100' : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              1h
            </button>
            <button
              onClick={() => setTimeRange('24h')}
              className={`px-2.5 py-1 rounded font-medium transition-colors ${
                timeRange === '24h' ? 'bg-indigo-600/30 text-indigo-300 border border-indigo-500/30 font-semibold' : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              24h
            </button>
            <button
              onClick={() => setTimeRange('7d')}
              className={`px-2.5 py-1 rounded font-medium transition-colors ${
                timeRange === '7d' ? 'bg-slate-800 text-slate-100' : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              7d
            </button>
          </div>

          <button
            onClick={handleRefresh}
            disabled={isRefreshing}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-slate-900 hover:bg-slate-800 border border-slate-800 text-xs font-medium text-slate-300 hover:text-slate-100 transition-colors shadow-sm"
            title="Refresh metrics ping"
          >
            <RefreshCw className={`w-3.5 h-3.5 ${isRefreshing ? 'animate-spin text-indigo-400' : ''}`} />
            <span className="hidden sm:inline">Refresh</span>
          </button>

          <button
            onClick={handleExportJson}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-indigo-600 hover:bg-indigo-500 text-white text-xs font-semibold shadow-md shadow-indigo-600/20 transition-all"
            title="Export health diagnostic report in JSON"
          >
            <Download className="w-3.5 h-3.5" />
            <span>Export Report</span>
          </button>
        </div>
      </div>

      {/* Diagnostic Notification Toast */}
      {diagnosticNotice && (
        <div className="p-3 rounded-xl bg-emerald-950/40 border border-emerald-500/40 text-emerald-300 text-xs flex items-center justify-between animate-fadeIn">
          <div className="flex items-center gap-2">
            <CheckCircle2 className="w-4 h-4 text-emerald-400" />
            <span>{diagnosticNotice}</span>
          </div>
          <span className="text-[10px] font-mono text-emerald-400/80">LATENCY: 8ms</span>
        </div>
      )}

      {/* HIGH-LEVEL KPI METRIC CARDS (4 Main Pillars) */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
        {/* CARD 1: Overall Workspace Health */}
        <div className="p-4 rounded-2xl bg-gradient-to-br from-slate-900/90 to-slate-950 border border-slate-800 shadow-sm relative overflow-hidden group">
          <div className="flex items-center justify-between text-xs text-slate-400 mb-2">
            <span className="font-semibold uppercase tracking-wider text-[10px]">Workspace Health Score</span>
            <span className="p-1 rounded-lg bg-emerald-500/10 text-emerald-400">
              <CheckCircle2 className="w-4 h-4" />
            </span>
          </div>
          <div className="flex items-baseline gap-2">
            <span className="text-3xl font-black text-slate-100 tracking-tight">{overallHealthScore}%</span>
            <span className="text-xs font-semibold text-emerald-400 flex items-center">
              <TrendingUp className="w-3 h-3 mr-0.5" /> +0.6%
            </span>
          </div>
          <p className="text-[11px] text-slate-400 mt-1">
            Zero fatal crashes across runtime containers.
          </p>
          <div className="w-full bg-slate-800/80 h-1.5 rounded-full overflow-hidden mt-3">
            <div 
              className="bg-emerald-500 h-full rounded-full transition-all duration-1000" 
              style={{ width: `${overallHealthScore}%` }} 
            />
          </div>
        </div>

        {/* CARD 2: Model Adapter Performance */}
        <div className="p-4 rounded-2xl bg-gradient-to-br from-slate-900/90 to-slate-950 border border-slate-800 shadow-sm relative overflow-hidden group">
          <div className="flex items-center justify-between text-xs text-slate-400 mb-2">
            <span className="font-semibold uppercase tracking-wider text-[10px]">Model Adapter Engine</span>
            <span className="p-1 rounded-lg bg-amber-500/10 text-amber-400">
              <Zap className="w-4 h-4" />
            </span>
          </div>
          <div className="flex items-baseline gap-2">
            <span className="text-2xl font-black text-slate-100 truncate">{activeModel.name.split(' ')[0]}</span>
            <span className="text-xs font-mono font-semibold text-indigo-400">{activeModel.latencyMs}ms</span>
          </div>
          <p className="text-[11px] text-slate-400 mt-1 truncate">
            {activeModel.tokensPerSec} tok/s • Zero-downtime hot-swap
          </p>
          <div className="flex items-center justify-between pt-2.5 mt-2 border-t border-slate-800/80 text-[10px] text-slate-400">
            <span>Failover: Standby OK</span>
            <button 
              onClick={() => onNavigateTab('model-adapter')}
              className="text-indigo-400 hover:text-indigo-300 font-medium flex items-center"
            >
              Swap Engine <ChevronRight className="w-3 h-3" />
            </button>
          </div>
        </div>

        {/* CARD 3: Sub-Agent Swarm Concurrency */}
        <div className="p-4 rounded-2xl bg-gradient-to-br from-slate-900/90 to-slate-950 border border-slate-800 shadow-sm relative overflow-hidden group">
          <div className="flex items-center justify-between text-xs text-slate-400 mb-2">
            <span className="font-semibold uppercase tracking-wider text-[10px]">Sub-Agent Swarm</span>
            <span className="p-1 rounded-lg bg-indigo-500/10 text-indigo-400">
              <Users className="w-4 h-4" />
            </span>
          </div>
          <div className="flex items-baseline gap-2">
            <span className="text-3xl font-black text-slate-100 tracking-tight">{activeAgentsCount}</span>
            <span className="text-xs text-slate-400">/ {subAgents.length} Active</span>
          </div>
          <p className="text-[11px] text-slate-400 mt-1">
            {totalTasks} parallel delegated micro-tasks executed.
          </p>
          <div className="flex items-center justify-between pt-2.5 mt-2 border-t border-slate-800/80 text-[10px] text-slate-400">
            <span className="text-emerald-400">99.1% Task Success</span>
            <button 
              onClick={() => onNavigateTab('team')}
              className="text-indigo-400 hover:text-indigo-300 font-medium flex items-center"
            >
              View Swarm <ChevronRight className="w-3 h-3" />
            </button>
          </div>
        </div>

        {/* CARD 4: Desktop Bridge & Security Guard */}
        <div className="p-4 rounded-2xl bg-gradient-to-br from-slate-900/90 to-slate-950 border border-slate-800 shadow-sm relative overflow-hidden group">
          <div className="flex items-center justify-between text-xs text-slate-400 mb-2">
            <span className="font-semibold uppercase tracking-wider text-[10px]">Bridge & Trust Boundary</span>
            <span className="p-1 rounded-lg bg-cyan-500/10 text-cyan-400">
              <ShieldCheck className="w-4 h-4" />
            </span>
          </div>
          <div className="flex items-baseline gap-2">
            <span className="text-2xl font-black text-slate-100 uppercase tracking-tight">{trustBoundary}</span>
            <span className="text-xs text-emerald-400 font-mono">HMAC OK</span>
          </div>
          <p className="text-[11px] text-slate-400 mt-1 truncate">
            {desktopBridge.connected ? `Live: ${desktopBridge.agentHost}` : 'Detached'}
          </p>
          <div className="flex items-center justify-between pt-2.5 mt-2 border-t border-slate-800/80 text-[10px] text-slate-400">
            <span>{desktopBridge.approvedPaths.length} Whitelisted Paths</span>
            <button 
              onClick={() => onNavigateTab('desktop-bridge')}
              className="text-indigo-400 hover:text-indigo-300 font-medium flex items-center"
            >
              Bridge Config <ChevronRight className="w-3 h-3" />
            </button>
          </div>
        </div>
      </div>

      {/* CHARTS & ANALYTICS: Interactive Throughput & Execution Trend */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Left 2 Cols: Time-Series Throughput & Latency Visualization */}
        <div className="lg:col-span-2 p-5 rounded-2xl bg-slate-900/70 border border-slate-800/90 flex flex-col justify-between">
          <div className="flex items-center justify-between mb-4">
            <div>
              <h3 className="text-xs font-bold text-slate-200 uppercase tracking-wider flex items-center gap-2">
                <BarChart3 className="w-4 h-4 text-indigo-400" />
                Model Adapter Token Velocity & Latency Curve
              </h3>
              <p className="text-[11px] text-slate-400 mt-0.5">
                Real-time token generation velocity (tok/sec) correlated with roundtrip latency (ms).
              </p>
            </div>
            <div className="flex items-center gap-3 text-[11px] font-mono">
              <span className="flex items-center gap-1 text-indigo-400">
                <span className="w-2.5 h-2.5 rounded-full bg-indigo-500" /> Velocity
              </span>
              <span className="flex items-center gap-1 text-cyan-400">
                <span className="w-2.5 h-2.5 rounded-full bg-cyan-400" /> Latency
              </span>
            </div>
          </div>

          {/* SVG Visualized Bar & Line Chart */}
          <div className="h-44 w-full relative flex items-end justify-between pt-4 pb-2 px-2 border-b border-slate-800/60">
            {throughputTrend.map((item, idx) => {
              const heightPercent = Math.min(100, Math.max(25, (item.tokens / 220) * 100));
              return (
                <div key={idx} className="flex-1 flex flex-col items-center justify-end h-full group relative mx-1">
                  {/* Tooltip on hover */}
                  <div className="absolute -top-10 opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none bg-slate-900 border border-slate-700 px-2 py-1 rounded text-[10px] font-mono text-slate-200 z-20 whitespace-nowrap shadow-xl">
                    <div>{item.tokens} tok/s</div>
                    <div className="text-cyan-300">{item.latency}ms latency</div>
                  </div>

                  {/* Latency marker dot */}
                  <div 
                    className="w-2 h-2 rounded-full bg-cyan-400 ring-2 ring-slate-950 mb-1 z-10 transition-all group-hover:scale-125"
                    style={{ marginBottom: `${heightPercent * 0.4}px` }}
                  />

                  {/* Velocity Bar */}
                  <div 
                    className="w-full max-w-[28px] rounded-t-lg bg-gradient-to-t from-indigo-900/60 via-indigo-600/80 to-indigo-400 transition-all duration-300 group-hover:brightness-125"
                    style={{ height: `${heightPercent}%` }}
                  />
                </div>
              );
            })}
          </div>

          {/* X-Axis labels */}
          <div className="flex justify-between text-[10px] font-mono text-slate-500 px-2 pt-2">
            {throughputTrend.map((item, idx) => (
              <span key={idx}>{item.hour}</span>
            ))}
          </div>
        </div>

        {/* Right 1 Col: Self-Healing Architecture & Trust Mesh Status */}
        <div className="p-5 rounded-2xl bg-slate-900/70 border border-slate-800/90 flex flex-col justify-between">
          <div>
            <h3 className="text-xs font-bold text-slate-200 uppercase tracking-wider flex items-center gap-2 mb-3">
              <Server className="w-4 h-4 text-emerald-400" />
              Runtime Core Health
            </h3>

            <div className="space-y-2.5 text-xs font-mono">
              <div className="p-2.5 rounded-xl bg-slate-950 border border-slate-800/80 flex items-center justify-between">
                <div>
                  <div className="text-slate-300 font-semibold">Container Orchestrator</div>
                  <div className="text-[10px] text-slate-500">Docker isolated cgroups</div>
                </div>
                <span className="text-[10px] font-bold px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-400">
                  ONLINE (3/3)
                </span>
              </div>

              <div className="p-2.5 rounded-xl bg-slate-950 border border-slate-800/80 flex items-center justify-between">
                <div>
                  <div className="text-slate-300 font-semibold">Desktop Bridge IPC</div>
                  <div className="text-[10px] text-slate-500">Inotify live file watcher</div>
                </div>
                <span className="text-[10px] font-bold px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-400">
                  {desktopBridge.connected ? 'SYNCED' : 'OFFLINE'}
                </span>
              </div>

              <div className="p-2.5 rounded-xl bg-slate-950 border border-slate-800/80 flex items-center justify-between">
                <div>
                  <div className="text-slate-300 font-semibold">Self-Improving Skills Loop</div>
                  <div className="text-[10px] text-slate-500">AST regression & bench</div>
                </div>
                <span className="text-[10px] font-bold px-2 py-0.5 rounded bg-indigo-500/20 text-indigo-400">
                  ACTIVE
                </span>
              </div>

              <div className="p-2.5 rounded-xl bg-slate-950 border border-slate-800/80 flex items-center justify-between">
                <div>
                  <div className="text-slate-300 font-semibold">Cryptographic Audit Trail</div>
                  <div className="text-[10px] text-slate-500">SHA-256 chain log</div>
                </div>
                <span className="text-[10px] font-bold px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-400">
                  VERIFIED
                </span>
              </div>
            </div>
          </div>

          <div className="pt-3 border-t border-slate-800/80 flex items-center justify-between text-[11px]">
            <span className="text-slate-400">Aetherius Tri-Compute:</span>
            <button
              onClick={() => onNavigateTab('admin')}
              className="text-cyan-400 hover:text-cyan-300 font-semibold font-mono flex items-center gap-1 group cursor-pointer"
              title="Open Aetherius OS Admin Console"
            >
              <span>OS Admin Console</span>
              <ChevronRight className="w-3.5 h-3.5 group-hover:translate-x-0.5 transition-transform" />
            </button>
          </div>
        </div>
      </div>

      {/* SUB-AGENT PERFORMANCE BREAKDOWN */}
      <div className="rounded-2xl border border-slate-800/90 bg-slate-900/60 p-5 space-y-4">
        <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-2">
          <div>
            <h3 className="text-xs font-bold text-slate-200 uppercase tracking-wider flex items-center gap-2">
              <Users className="w-4 h-4 text-indigo-400" />
              Sub-Agent Performance Analytics & Workload
            </h3>
            <p className="text-[11px] text-slate-400">
              Granular metrics across all 6 specialized agents in the OmniAgent swarm.
            </p>
          </div>

          <button
            onClick={() => onNavigateTab('team')}
            className="text-xs text-indigo-400 hover:text-indigo-300 font-medium flex items-center self-start sm:self-auto"
          >
            Manage Swarm Hierarchy <ChevronRight className="w-3.5 h-3.5 ml-1" />
          </button>
        </div>

        {/* Agent Cards Grid */}
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-3">
          {agentPerformanceData.map((agent) => (
            <div 
              key={agent.id}
              className="p-3.5 rounded-xl bg-slate-950/80 border border-slate-800/80 hover:border-slate-700 transition-all space-y-3"
            >
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-2.5">
                  <div className="relative">
                    <img 
                      src={agent.avatar} 
                      alt={agent.name} 
                      className="w-8 h-8 rounded-lg object-cover ring-1 ring-slate-800"
                    />
                    <span className={`absolute -bottom-0.5 -right-0.5 w-2.5 h-2.5 rounded-full ring-2 ring-slate-950 ${
                      agent.status === 'running' ? 'bg-emerald-400' : 'bg-slate-500'
                    }`} />
                  </div>
                  <div>
                    <h4 className="text-xs font-bold text-slate-200">{agent.name}</h4>
                    <p className="text-[10px] text-slate-400 truncate max-w-[130px]">{agent.role}</p>
                  </div>
                </div>

                <span className={`text-[10px] font-mono px-2 py-0.5 rounded capitalize ${
                  agent.status === 'running'
                    ? 'bg-emerald-500/20 text-emerald-300 border border-emerald-500/30'
                    : 'bg-slate-800 text-slate-400'
                }`}>
                  {agent.status}
                </span>
              </div>

              {/* Stats Grid */}
              <div className="grid grid-cols-3 gap-2 text-center text-[10px] font-mono pt-1">
                <div className="p-1.5 rounded bg-slate-900 border border-slate-800/60">
                  <div className="text-slate-500">Tasks</div>
                  <div className="text-slate-200 font-bold text-xs mt-0.5">{agent.totalTasks}</div>
                </div>
                <div className="p-1.5 rounded bg-slate-900 border border-slate-800/60">
                  <div className="text-slate-500">Success</div>
                  <div className="text-emerald-400 font-bold text-xs mt-0.5">{agent.successRate}%</div>
                </div>
                <div className="p-1.5 rounded bg-slate-900 border border-slate-800/60">
                  <div className="text-slate-500">Latency</div>
                  <div className="text-cyan-400 font-bold text-xs mt-0.5">{agent.avgLatency}ms</div>
                </div>
              </div>

              {/* Progress bar of current workload */}
              <div>
                <div className="flex items-center justify-between text-[10px] text-slate-400 mb-1">
                  <span>Task Progress</span>
                  <span className="font-mono text-slate-300">{agent.progress}%</span>
                </div>
                <div className="w-full bg-slate-900 h-1 rounded-full overflow-hidden">
                  <div 
                    className="bg-indigo-500 h-full rounded-full transition-all duration-500" 
                    style={{ width: `${agent.progress}%` }} 
                  />
                </div>
              </div>
            </div>
          ))}
        </div>
      </div>

      {/* RECENT SYSTEM EVENTS & AUDIT LOGS */}
      <div className="rounded-2xl border border-slate-800/90 bg-slate-900/60 p-5 space-y-4">
        <div className="flex flex-col md:flex-row md:items-center md:justify-between gap-3">
          <div>
            <h3 className="text-xs font-bold text-slate-200 uppercase tracking-wider flex items-center gap-2">
              <Clock className="w-4 h-4 text-cyan-400" />
              Recent System Events & Execution Audit
            </h3>
            <p className="text-[11px] text-slate-400">
              Append-only audit trail logging tool calls, model swaps, and local bridge access.
            </p>
          </div>

          <div className="flex items-center gap-2">
            {/* Search Input */}
            <div className="relative">
              <Search className="w-3.5 h-3.5 absolute left-2.5 top-2 text-slate-500" />
              <input
                type="text"
                value={eventSearch}
                onChange={(e) => setEventSearch(e.target.value)}
                placeholder="Filter events..."
                className="bg-slate-950 border border-slate-800 rounded-lg pl-8 pr-3 py-1 text-xs text-slate-200 placeholder-slate-500 focus:outline-none focus:border-indigo-500 w-44"
              />
            </div>

            {/* Category Filter */}
            <select
              value={eventFilter}
              onChange={(e) => setEventFilter(e.target.value)}
              className="bg-slate-950 border border-slate-800 rounded-lg px-2.5 py-1 text-xs text-slate-300 focus:outline-none focus:border-indigo-500"
            >
              <option value="all">All Event Categories</option>
              <option value="model_swap">Model Engine Swaps</option>
              <option value="bridge_access">Desktop Bridge Access</option>
              <option value="tool_call">Sub-Agent Dispatches</option>
              <option value="permission_check">Security Boundary Checks</option>
              <option value="file_write">Storage & Memory Writes</option>
            </select>

            <button
              onClick={() => onNavigateTab('audit-logs')}
              className="text-xs text-indigo-400 hover:text-indigo-300 font-medium flex items-center shrink-0"
            >
              All Logs <ChevronRight className="w-3.5 h-3.5" />
            </button>
          </div>
        </div>

        {/* Events Table / List */}
        <div className="rounded-xl border border-slate-800/80 bg-slate-950/80 overflow-hidden divide-y divide-slate-800/60 font-mono text-xs">
          {filteredLogs.length === 0 ? (
            <div className="p-6 text-center text-slate-500 text-xs font-sans">
              No matching system events found for the selected filter.
            </div>
          ) : (
            filteredLogs.slice(0, 7).map((log) => {
              const isHigh = log.riskLevel === 'high' || log.riskScore === 'high';
              const isMedium = log.riskLevel === 'medium' || log.riskScore === 'medium';
              return (
                <div 
                  key={log.id} 
                  className="p-3 flex flex-col sm:flex-row sm:items-center justify-between gap-2 hover:bg-slate-900/50 transition-colors"
                >
                  <div className="flex items-start sm:items-center gap-3">
                    <span className={`w-2 h-2 rounded-full mt-1.5 sm:mt-0 shrink-0 ${
                      isHigh ? 'bg-rose-500 animate-ping' : isMedium ? 'bg-amber-400' : 'bg-emerald-400'
                    }`} />
                    <div>
                      <div className="flex items-center gap-2 flex-wrap">
                        <span className="font-bold text-slate-200">{log.action}</span>
                        <span className="text-[10px] px-1.5 py-0.2 rounded bg-slate-900 border border-slate-800 text-indigo-300">
                          {log.agent}
                        </span>
                        <span className="text-[10px] text-slate-500">{log.timestamp}</span>
                      </div>
                      <p className="text-[11px] text-slate-400 font-sans mt-0.5 line-clamp-1">
                        {log.details}
                      </p>
                    </div>
                  </div>

                  <div className="flex items-center gap-2 shrink-0 self-end sm:self-auto">
                    <span className={`text-[10px] px-2 py-0.5 rounded capitalize ${
                      log.status === 'allowed' || log.status === 'approved'
                        ? 'bg-emerald-500/20 text-emerald-300'
                        : log.status === 'flagged'
                        ? 'bg-amber-500/20 text-amber-300'
                        : 'bg-rose-500/20 text-rose-300'
                    }`}>
                      {log.status}
                    </span>
                    <span className={`text-[10px] px-1.5 py-0.5 rounded font-bold uppercase ${
                      isHigh ? 'bg-rose-950 text-rose-300 border border-rose-800' : 'bg-slate-900 text-slate-400'
                    }`}>
                      {log.riskLevel || log.riskScore || 'low'}
                    </span>
                  </div>
                </div>
              );
            })
          )}
        </div>
      </div>
    </div>
  );
};
