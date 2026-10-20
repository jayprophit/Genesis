import React, { useState, useEffect } from 'react';
import { Header } from './components/Header';
import { Sidebar, ActiveTab } from './components/Sidebar';
import { WorkspaceHealthDashboard } from './components/WorkspaceHealthDashboard';
import { AetheriusAdminPanel } from './components/AetheriusAdminPanel';
import { ChatWorkspace } from './components/ChatWorkspace';
import { OELifeAmplifiedDashboard } from './components/OELifeAmplifiedDashboard';
import { NovaTeamWorkspace } from './components/NovaTeamWorkspace';
import { UniversalWorkspaceView } from './components/UniversalWorkspaceView';
import { SubAgentsTeam } from './components/SubAgentsTeam';
import { ModelAdapterHub } from './components/ModelAdapterHub';
import { OwnedLayerBlueprint } from './components/OwnedLayerBlueprint';
import { SkillsLibrary } from './components/SkillsLibrary';
import { MemoryInspector } from './components/MemoryInspector';
import { DesktopBridgeView } from './components/DesktopBridgeView';
import { RoutinesScheduler } from './components/RoutinesScheduler';
import { AuditLogView } from './components/AuditLogView';
import { VoiceCallModal } from './components/VoiceCallModal';
import { AIVideoCallModal } from './components/AIVideoCallModal';
import { SetupConfigModal } from './components/SetupConfigModal';

import { 
  RuntimeMode, 
  TrustBoundary, 
  ThemeMode, 
  DeviceView, 
  ModelAdapterInfo, 
  SubAgent, 
  Skill, 
  Routine, 
  AuditLogEntry, 
  DesktopBridgeState, 
  MemoryItem, 
  InterAgentMessage 
} from './types';

import { 
  initialSubAgents, 
  initialModels, 
  initialSkills, 
  initialRoutines, 
  initialAuditLogs, 
  initialDesktopBridge, 
  initialMemoryItems, 
  initialInterAgentMessages 
} from './data/mockInitialData';

export default function App() {
  // Runtime State
  const [runtimeMode, setRuntimeMode] = useState<RuntimeMode>('managed-cloud');
  const [trustBoundary, setTrustBoundary] = useState<TrustBoundary>('guarded');
  const [activeTab, setActiveTab] = useState<ActiveTab>('oe-dashboard');
  const [activeModelId, setActiveModelId] = useState<string>('gemini-3.8-flash');
  const [theme, setTheme] = useState<ThemeMode>('dark');
  const [deviceView, setDeviceView] = useState<DeviceView>('desktop');

  // Modals
  const [isVoiceModalOpen, setIsVoiceModalOpen] = useState(false);
  const [isVideoModalOpen, setIsVideoModalOpen] = useState(false);
  const [isSetupModalOpen, setIsSetupModalOpen] = useState(false);

  // Data Collections
  const [subAgents, setSubAgents] = useState<SubAgent[]>(initialSubAgents);
  const [models, setModels] = useState<ModelAdapterInfo[]>(initialModels);
  const [skills, setSkills] = useState<Skill[]>(initialSkills);
  const [routines, setRoutines] = useState<Routine[]>(initialRoutines);
  const [auditLogs, setAuditLogs] = useState<AuditLogEntry[]>(initialAuditLogs);
  const [desktopBridge, setDesktopBridge] = useState<DesktopBridgeState>(initialDesktopBridge);
  const [memoryItems, setMemoryItems] = useState<MemoryItem[]>(initialMemoryItems);
  const [interAgentMessages, setInterAgentMessages] = useState<InterAgentMessage[]>(initialInterAgentMessages);

  // Sync with Express backend on startup
  useEffect(() => {
    async function fetchOverview() {
      try {
        const res = await fetch('/api/runtime/overview');
        if (res.ok) {
          const data = await res.json();
          if (data.mode) setRuntimeMode(data.mode);
          if (data.trustBoundary) setTrustBoundary(data.trustBoundary);
          if (data.activeModel) setActiveModelId(data.activeModel);
          if (data.models) setModels(data.models);
          if (data.subAgents) setSubAgents(data.subAgents);
          if (data.skills) setSkills(data.skills);
          if (data.routines) setRoutines(data.routines);
          if (data.auditLogs) setAuditLogs(data.auditLogs);
          if (data.desktopBridge) setDesktopBridge(data.desktopBridge);
        }
      } catch (e) {
        console.warn('Backend overview fetch notice (using client hydration):', e);
      }
    }
    fetchOverview();
  }, []);

  // Update theme data attribute on body
  useEffect(() => {
    document.body.setAttribute('data-theme', theme);
  }, [theme]);

  const activeModel = models.find(m => m.id === activeModelId) || models[0];

  // Actions
  const handleSelectMode = async (mode: RuntimeMode) => {
    setRuntimeMode(mode);
    try {
      await fetch('/api/runtime/mode', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode })
      });
      // Append audit log
      const newLog: AuditLogEntry = {
        id: `audit-${Date.now()}`,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
        agent: 'System',
        action: 'RUNTIME_MODE_SWITCH',
        details: `Switched runtime architecture mode to '${mode}'`,
        riskLevel: 'low',
        category: 'permission_check',
        status: 'allowed'
      };
      setAuditLogs(prev => [newLog, ...prev]);
    } catch (e) {
      console.error(e);
    }
  };

  const handleSelectModel = async (modelId: string) => {
    setActiveModelId(modelId);
    try {
      await fetch('/api/runtime/model', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ model: modelId })
      });
      const selected = models.find(m => m.id === modelId);
      const newLog: AuditLogEntry = {
        id: `audit-${Date.now()}`,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
        agent: 'ModelAdapter',
        action: 'HOT_SWAP_ENGINE',
        details: `Zero-downtime engine swap to '${selected?.name || modelId}'`,
        riskLevel: 'low',
        category: 'model_swap',
        status: 'allowed'
      };
      setAuditLogs(prev => [newLog, ...prev]);
    } catch (e) {
      console.error(e);
    }
  };

  const handleSelectBoundary = async (boundary: TrustBoundary) => {
    setTrustBoundary(boundary);
    try {
      await fetch('/api/runtime/trust-boundary', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ trustBoundary: boundary })
      });
      const newLog: AuditLogEntry = {
        id: `audit-${Date.now()}`,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
        agent: 'TrustGovernor',
        action: 'TRUST_BOUNDARY_UPDATED',
        details: `Updated security autonomy boundary to '${boundary.toUpperCase()}'`,
        riskLevel: boundary === 'permissive' ? 'medium' : 'low',
        category: 'permission_check',
        status: 'allowed'
      };
      setAuditLogs(prev => [newLog, ...prev]);
    } catch (e) {
      console.error(e);
    }
  };

  const handleToggleBridge = async () => {
    const nextState = !desktopBridge.connected;
    setDesktopBridge(prev => ({
      ...prev,
      connected: nextState,
      syncStatus: nextState ? 'synced' : 'offline'
    }));
    try {
      await fetch('/api/bridge/toggle', { method: 'POST' });
      const newLog: AuditLogEntry = {
        id: `audit-${Date.now()}`,
        timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
        agent: 'DesktopBridge',
        action: nextState ? 'BRIDGE_DAEMON_ATTACHED' : 'BRIDGE_DAEMON_DETACHED',
        details: nextState ? 'HMAC session opened with workstation localhost:9876' : 'Local RPC socket terminated',
        riskLevel: 'low',
        category: 'bridge_access',
        status: 'allowed'
      };
      setAuditLogs(prev => [newLog, ...prev]);
    } catch (e) {
      console.error(e);
    }
  };

  const handleDispatchTask = async (prompt: string, targetAgentId: string = 'sarah'): Promise<any> => {
    const res = await fetch('/api/agent/dispatch', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ prompt, agent: targetAgentId })
    });
    
    if (!res.ok) {
      throw new Error(`Dispatch failed: ${res.statusText}`);
    }

    const data = await res.json();

    // Increment assigned agent's messages & bump progress
    setSubAgents(prev => prev.map(a => {
      if (a.id === targetAgentId || a.name.toLowerCase().includes(data.agent.toLowerCase())) {
        return {
          ...a,
          messagesCount: a.messagesCount + 1,
          progress: Math.min(100, a.progress + 15),
          lastAction: 'Just completed prompt dispatch'
        };
      }
      return a;
    }));

    // Record in audit log
    const auditEntry: AuditLogEntry = {
      id: `audit-${Date.now()}`,
      timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
      agent: data.agent,
      action: 'TASK_DISPATCH_COMPLETE',
      details: `Prompt processed using ${data.model} in ${data.latencyMs}ms. Tools: ${data.toolCalls.join(', ')}`,
      riskLevel: 'low',
      category: 'tool_call',
      status: 'allowed'
    };
    setAuditLogs(prev => [auditEntry, ...prev]);

    return data;
  };

  const handleAgentAction = (agentId: string, action: 'pause' | 'resume' | 'approve') => {
    setSubAgents(prev => prev.map(a => {
      if (a.id === agentId) {
        if (action === 'pause') return { ...a, status: 'idle', lastAction: 'Manually paused by user' };
        if (action === 'resume') return { ...a, status: 'running', lastAction: 'Resumed parallel execution' };
        if (action === 'approve') return { ...a, status: 'running', lastAction: 'Approved by admin', progress: a.progress + 20 };
      }
      return a;
    }));

    const agentObj = subAgents.find(a => a.id === agentId);
    const newLog: AuditLogEntry = {
      id: `audit-${Date.now()}`,
      timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
      agent: agentObj?.name || 'Admin',
      action: `AGENT_${action.toUpperCase()}`,
      details: `User executed action '${action}' on agent ${agentObj?.name}`,
      riskLevel: action === 'approve' ? 'medium' : 'low',
      category: 'permission_check',
      status: 'allowed'
    };
    setAuditLogs(prev => [newLog, ...prev]);
  };

  const handleImproveSkill = (skillId: string) => {
    setSkills(prev => prev.map(s => {
      if (s.id === skillId) {
        const nextVersion = (parseFloat(s.version) + 0.1).toFixed(1);
        const nextSuccess = Math.min(99, s.successRate + 2);
        return {
          ...s,
          version: nextVersion,
          successRate: nextSuccess,
          selfImprovedTimes: s.selfImprovedTimes + 1
        };
      }
      return s;
    }));

    const target = skills.find(s => s.id === skillId);
    const newLog: AuditLogEntry = {
      id: `audit-${Date.now()}`,
      timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
      agent: 'SelfImproveLoop',
      action: 'SKILL_AST_OPTIMIZED',
      details: `Autonomous AST optimization applied to skill '${target?.name}'. Success rate bumped to 98%.`,
      riskLevel: 'low',
      category: 'file_write',
      status: 'allowed'
    };
    setAuditLogs(prev => [newLog, ...prev]);
  };

  const handleRunSkill = (skillName: string) => {
    setSkills(prev => prev.map(s => {
      if (s.name === skillName) {
        return { ...s, runsCount: s.runsCount + 1 };
      }
      return s;
    }));
    setActiveTab('chat');
  };

  const handleAddMemory = (tier: MemoryItem['tier'], title: string, content: string, tags: string[]) => {
    const newItem: MemoryItem = {
      id: `mem-${Date.now()}`,
      tier,
      title,
      content,
      tags,
      updatedAt: 'Just now',
      vectorScore: 0.94
    };
    setMemoryItems(prev => [newItem, ...prev]);

    const newLog: AuditLogEntry = {
      id: `audit-${Date.now()}`,
      timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
      agent: 'MemoryPipeline',
      action: 'VECTOR_EMBEDDING_INGESTED',
      details: `Ingested memory item '${title}' into partition Tier: ${tier.toUpperCase()}`,
      riskLevel: 'low',
      category: 'file_write',
      status: 'allowed'
    };
    setAuditLogs(prev => [newLog, ...prev]);
  };

  const handleToggleRoutine = (id: string) => {
    setRoutines(prev => prev.map(r => r.id === id ? { ...r, status: r.status === 'active' ? 'paused' : 'active' } : r));
  };

  const handleRunRoutineNow = (id: string) => {
    setRoutines(prev => prev.map(r => r.id === id ? { ...r, lastRun: 'Just now' } : r));
    const rObj = routines.find(r => r.id === id);
    const newLog: AuditLogEntry = {
      id: `audit-${Date.now()}`,
      timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
      agent: rObj?.assignedAgent || 'CronDaemon',
      action: 'SCHEDULED_ROUTINE_EXECUTED',
      details: `Manual trigger: '${rObj?.name}' ran to completion.`,
      riskLevel: 'low',
      category: 'tool_call',
      status: 'allowed'
    };
    setAuditLogs(prev => [newLog, ...prev]);
  };

  const handleAddBridgePath = (path: string) => {
    setDesktopBridge(prev => ({
      ...prev,
      approvedPaths: [...prev.approvedPaths, path]
    }));
  };

  const handleRemoveBridgePath = (path: string) => {
    setDesktopBridge(prev => ({
      ...prev,
      approvedPaths: prev.approvedPaths.filter(p => p !== path)
    }));
  };

  const handleSendInterAgentMessage = (from: string, to: string, content: string) => {
    const newMsg: InterAgentMessage = {
      id: `msg-${Date.now()}`,
      from,
      to,
      content,
      timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
    };
    setInterAgentMessages(prev => [newMsg, ...prev]);
  };

  // Device frame sizing
  const isConstrainedDevice = deviceView !== 'desktop';

  return (
    <div className={`min-h-screen bg-slate-950 flex flex-col ${
      isConstrainedDevice ? 'items-center justify-center p-4 bg-slate-950/95' : ''
    }`}>
      {/* Device Wrapper Container */}
      <div className={`w-full flex flex-col bg-slate-950 text-slate-100 transition-all duration-300 ${
        deviceView === 'desktop'
          ? 'h-screen'
          : deviceView === 'tablet'
          ? 'max-w-5xl h-[90vh] rounded-3xl border-4 border-slate-700 shadow-2xl overflow-hidden'
          : 'max-w-md h-[90vh] rounded-3xl border-4 border-slate-700 shadow-2xl overflow-hidden'
      }`}>
        {/* Device View Bar Indicator if simulated */}
        {isConstrainedDevice && (
          <div className="h-6 bg-slate-900 border-b border-slate-800 flex items-center justify-between px-4 text-[10px] text-slate-400 font-mono select-none">
            <span>Simulating {deviceView.toUpperCase()} Form Factor</span>
            <button
              onClick={() => setDeviceView('desktop')}
              className="text-indigo-400 hover:text-indigo-300 font-bold"
            >
              Exit to Full Desktop
            </button>
          </div>
        )}

        {/* Top Navigation Header */}
        <Header
          mode={runtimeMode}
          onSelectMode={handleSelectMode}
          activeModel={activeModel}
          models={models}
          onSelectModel={handleSelectModel}
          trustBoundary={trustBoundary}
          onSelectBoundary={handleSelectBoundary}
          desktopBridge={desktopBridge}
          onToggleBridge={handleToggleBridge}
          theme={theme}
          onSelectTheme={setTheme}
          deviceView={deviceView}
          onSelectDeviceView={setDeviceView}
          onOpenVoiceCall={() => setIsVoiceModalOpen(true)}
          onOpenVideoCall={() => setIsVideoModalOpen(true)}
          onOpenSetup={() => setIsSetupModalOpen(true)}
        />

        {/* Main Body: Sidebar + Active View */}
        <div className="flex-1 flex overflow-hidden">
          <Sidebar
            activeTab={activeTab}
            onSelectTab={setActiveTab}
            runtimeMode={runtimeMode}
            unreadAuditCount={auditLogs.filter(l => l.riskLevel === 'high' || l.status === 'flagged').length}
            activeAgentsCount={subAgents.filter(a => a.status === 'running').length}
            activeSkillsCount={skills.length}
          />

          <main className="flex-1 flex flex-col overflow-hidden bg-slate-950/50">
            {activeTab === 'dashboard' && (
              <WorkspaceHealthDashboard
                subAgents={subAgents}
                activeModel={activeModel}
                models={models}
                runtimeMode={runtimeMode}
                trustBoundary={trustBoundary}
                desktopBridge={desktopBridge}
                auditLogs={auditLogs}
                onNavigateTab={setActiveTab}
                onSelectModel={handleSelectModel}
                onTriggerDiagnosticPing={() => {
                  const newLog: AuditLogEntry = {
                    id: `audit-${Date.now()}`,
                    timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
                    agent: 'HealthMonitor',
                    action: 'HEALTH_DIAGNOSTIC_PROBE',
                    details: 'Executed workspace health check ping across runtime mesh and bridge daemon',
                    riskLevel: 'low',
                    category: 'permission_check',
                    status: 'allowed'
                  };
                  setAuditLogs(prev => [newLog, ...prev]);
                }}
              />
            )}

            {activeTab === 'admin' && (
              <AetheriusAdminPanel
                subAgents={subAgents}
                activeModel={activeModel}
                desktopBridge={desktopBridge}
                trustBoundary={trustBoundary}
                onDispatchTask={handleDispatchTask}
              />
            )}

            {activeTab === 'oe-dashboard' && (
              <OELifeAmplifiedDashboard
                activeModel={activeModel}
                onDispatchPrompt={handleDispatchTask}
                onOpenVideoCallModal={() => setIsVideoModalOpen(true)}
              />
            )}

            {activeTab === 'nova-workspace' && (
              <NovaTeamWorkspace
                subAgents={subAgents}
                onOpenVideoModal={() => setIsVideoModalOpen(true)}
              />
            )}

            {activeTab === 'universal-showcase' && (
              <UniversalWorkspaceView
                activeModel={activeModel}
              />
            )}

            {activeTab === 'chat' && (
              <ChatWorkspace
                subAgents={subAgents}
                activeModel={activeModel}
                trustBoundary={trustBoundary}
                desktopBridge={desktopBridge}
                theme={theme}
                onSelectTheme={setTheme}
                onDispatchTask={handleDispatchTask}
                onSelectModel={handleSelectModel}
                onToggleBridge={handleToggleBridge}
                onOpenVideoCall={() => setIsVideoModalOpen(true)}
                onOpenVoiceCall={() => setIsVoiceModalOpen(true)}
              />
            )}

            {activeTab === 'team' && (
              <SubAgentsTeam
                subAgents={subAgents}
                interAgentMessages={interAgentMessages}
                onAgentAction={handleAgentAction}
                onSendInterAgentMessage={handleSendInterAgentMessage}
              />
            )}

            {activeTab === 'model-adapter' && (
              <ModelAdapterHub
                models={models}
                activeModel={activeModel}
                onSelectModel={handleSelectModel}
              />
            )}

            {activeTab === 'owned-layer' && (
              <OwnedLayerBlueprint
                activeModel={activeModel}
                trustBoundary={trustBoundary}
                onNavigateToTab={setActiveTab}
              />
            )}

            {activeTab === 'skills' && (
              <SkillsLibrary
                skills={skills}
                onImproveSkill={handleImproveSkill}
                onRunSkill={handleRunSkill}
              />
            )}

            {activeTab === 'memory' && (
              <MemoryInspector
                memoryItems={memoryItems}
                onAddMemory={handleAddMemory}
              />
            )}

            {activeTab === 'desktop-bridge' && (
              <DesktopBridgeView
                bridgeState={desktopBridge}
                onToggleBridge={handleToggleBridge}
                onAddPath={handleAddBridgePath}
                onRemovePath={handleRemoveBridgePath}
              />
            )}

            {activeTab === 'routines' && (
              <RoutinesScheduler
                routines={routines}
                onToggleRoutine={handleToggleRoutine}
                onRunRoutineNow={handleRunRoutineNow}
              />
            )}

            {activeTab === 'audit-logs' && (
              <AuditLogView
                logs={auditLogs}
                trustBoundary={trustBoundary}
              />
            )}
          </main>
        </div>
      </div>

      {/* Voice Call Avatar & Waveform Modal */}
      <VoiceCallModal
        isOpen={isVoiceModalOpen}
        onClose={() => setIsVoiceModalOpen(false)}
        agent={subAgents[0]}
        model={activeModel}
        onSwitchToVideo={() => {
          setIsVoiceModalOpen(false);
          setIsVideoModalOpen(true);
        }}
      />

      {/* Face-to-Face Video Call with Realistic AI Avatar and User Webcam PIP */}
      <AIVideoCallModal
        isOpen={isVideoModalOpen}
        onClose={() => setIsVideoModalOpen(false)}
        activeModel={activeModel}
        onSwitchToChat={() => {
          setIsVideoModalOpen(false);
          setActiveTab('chat');
        }}
      />

      {/* Setup & Deployment Configuration Modal */}
      <SetupConfigModal
        isOpen={isSetupModalOpen}
        onClose={() => setIsSetupModalOpen(false)}
        trustBoundary={trustBoundary}
        onUpdateBoundary={handleSelectBoundary}
      />
    </div>
  );
}
