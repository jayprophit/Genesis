import React, { useState } from 'react';
import { 
  Activity,
  Atom,
  LayoutDashboard,
  MessageSquare, 
  Users, 
  Zap, 
  Layers, 
  Wrench, 
  Database, 
  Laptop, 
  Clock, 
  ShieldCheck, 
  Terminal, 
  Sparkles,
  ChevronRight,
  FolderGit2,
  PanelLeftClose,
  PanelLeftOpen
} from 'lucide-react';
import { RuntimeMode } from '../types';

export type ActiveTab = 
  | 'oe-dashboard'
  | 'chat' 
  | 'nova-workspace'
  | 'universal-showcase'
  | 'dashboard'
  | 'admin'
  | 'team' 
  | 'model-adapter' 
  | 'owned-layer' 
  | 'skills' 
  | 'memory' 
  | 'desktop-bridge' 
  | 'routines' 
  | 'audit-logs';

interface SidebarProps {
  activeTab: ActiveTab;
  onSelectTab: (tab: ActiveTab) => void;
  runtimeMode: RuntimeMode;
  unreadAuditCount?: number;
  activeAgentsCount?: number;
  activeSkillsCount?: number;
}

export const Sidebar: React.FC<SidebarProps> = ({
  activeTab,
  onSelectTab,
  runtimeMode,
  unreadAuditCount = 0,
  activeAgentsCount = 4,
  activeSkillsCount = 5
}) => {
  const [isCollapsed, setIsCollapsed] = useState(false);
  const menuItems = [
    {
      id: 'oe-dashboard' as ActiveTab,
      label: 'OE Life Amplified',
      sublabel: 'Human Avatar & Bento Grid (Image 2)',
      icon: Sparkles,
      badge: 'Image 2 • Sarah AI'
    },
    {
      id: 'chat' as ActiveTab,
      label: 'Cascading Workspace',
      sublabel: '3-Panel Chat, Code, Video & Browser (Image 1)',
      icon: MessageSquare,
      badge: 'Image 1 • Live AI'
    },
    {
      id: 'nova-workspace' as ActiveTab,
      label: 'NOVA AI Team',
      sublabel: '12-Agent Grid & Boardroom (Image 4)',
      icon: Users,
      badge: 'Image 4 • 12 Live'
    },
    {
      id: 'universal-showcase' as ActiveTab,
      label: 'Universal Showcase',
      sublabel: '12 Creative & Tech Studios (Image 3)',
      icon: Layers,
      badge: 'Image 3 • Studios'
    },
    {
      id: 'dashboard' as ActiveTab,
      label: 'Health & Metrics',
      sublabel: 'Summary Dashboard & Vital Signs',
      icon: LayoutDashboard,
      badge: '99.4% OK'
    },
    {
      id: 'admin' as ActiveTab,
      label: 'Aetherius OS Admin',
      sublabel: 'Tri-Compute & HyperVM Backend',
      icon: Atom,
      badge: 'Root Parent'
    },
    {
      id: 'team' as ActiveTab,
      label: 'Sub-Agent Swarm',
      sublabel: 'Parallel Specialists',
      icon: Users,
      badge: `${activeAgentsCount} active`
    },
    {
      id: 'model-adapter' as ActiveTab,
      label: 'Model Adapter',
      sublabel: 'Engine Swapping Hub',
      icon: Zap,
      badge: 'Zero-Code'
    },
    {
      id: 'owned-layer' as ActiveTab,
      label: 'Your Owned Layer',
      sublabel: 'Files, Tools, Memory, Routines',
      icon: Layers,
      badge: 'Core'
    },
    {
      id: 'skills' as ActiveTab,
      label: 'Skills & Self-Loop',
      sublabel: 'Reusable Autonomous Code',
      icon: Wrench,
      badge: `${activeSkillsCount} skills`
    },
    {
      id: 'memory' as ActiveTab,
      label: '4-Tier Memory',
      sublabel: 'Global, Project, Thread, Agent',
      icon: Database,
      badge: '1.4k vecs'
    },
    {
      id: 'desktop-bridge' as ActiveTab,
      label: 'Desktop Bridge',
      sublabel: 'Approved Local Files & Ports',
      icon: Laptop,
      badge: 'RPC Live'
    },
    {
      id: 'routines' as ActiveTab,
      label: 'Routines & Cron',
      sublabel: 'Background Automation',
      icon: Clock,
      badge: '4 tasks'
    },
    {
      id: 'audit-logs' as ActiveTab,
      label: 'Audit & Trust Logs',
      sublabel: 'Tamper-Proof Audit Trail',
      icon: ShieldCheck,
      badge: unreadAuditCount > 0 ? `${unreadAuditCount} new` : 'Secured'
    }
  ];

  return (
    <aside className={`${
      isCollapsed ? 'w-16' : 'w-64'
    } border-r border-slate-800/80 bg-slate-950/85 backdrop-blur-md flex flex-col justify-between shrink-0 select-none transition-all duration-200 z-10`}>
      <div className="p-2 sm:p-3 space-y-1 overflow-y-auto no-scrollbar">
        {/* Header row with Title and Collapse Button */}
        <div className="px-2 py-1.5 flex items-center justify-between">
          {!isCollapsed && (
            <span className="text-[10px] font-bold tracking-wider text-slate-500 uppercase truncate">
              Modules
            </span>
          )}
          <button
            onClick={() => setIsCollapsed(!isCollapsed)}
            className="p-1 rounded-lg hover:bg-slate-800/80 text-slate-400 hover:text-slate-200 transition-colors mx-auto"
            title={isCollapsed ? "Expand sidebar menu" : "Collapse sidebar menu"}
          >
            {isCollapsed ? <PanelLeftOpen className="w-4 h-4" /> : <PanelLeftClose className="w-4 h-4" />}
          </button>
        </div>

        {menuItems.map((item) => {
          const isActive = activeTab === item.id;
          const Icon = item.icon;
          return (
            <button
              key={item.id}
              onClick={() => onSelectTab(item.id)}
              className={`w-full text-left rounded-xl text-xs flex items-center transition-all group relative ${
                isCollapsed 
                  ? 'p-2.5 justify-center' 
                  : 'px-3 py-2.5 justify-between'
              } ${
                isActive
                  ? 'bg-gradient-to-r from-indigo-600/25 to-violet-600/15 border border-indigo-500/40 text-slate-100 shadow-sm shadow-indigo-500/10'
                  : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900/60'
              }`}
              title={isCollapsed ? `${item.label} — ${item.sublabel}` : undefined}
            >
              <div className="flex items-center gap-3 min-w-0">
                <div className={`p-1.5 rounded-lg transition-colors shrink-0 ${
                  isActive ? 'bg-indigo-500 text-white' : 'bg-slate-900 text-slate-400 group-hover:text-slate-200'
                }`}>
                  <Icon className="w-4 h-4" />
                </div>
                {!isCollapsed && (
                  <div className="min-w-0 break-words">
                    <div className="font-semibold truncate">{item.label}</div>
                    <div className="text-[10px] text-slate-400 truncate">{item.sublabel}</div>
                  </div>
                )}
              </div>

              {!isCollapsed && item.badge && (
                <span className={`text-[10px] px-1.5 py-0.5 rounded font-mono font-medium shrink-0 ml-1.5 ${
                  isActive 
                    ? 'bg-indigo-500/30 text-indigo-200 border border-indigo-500/30' 
                    : 'bg-slate-900 text-slate-400 border border-slate-800'
                }`}>
                  {item.badge}
                </span>
              )}

              {/* Collapsed active dot */}
              {isCollapsed && isActive && (
                <span className="absolute right-1 top-1 w-2 h-2 rounded-full bg-indigo-500" />
              )}
            </button>
          );
        })}
      </div>

      {/* Bottom Mini Diagnostics */}
      {!isCollapsed ? (
        <button 
          onClick={() => onSelectTab('dashboard')}
          className="text-left p-3 border-t border-slate-800/80 bg-slate-950/90 m-2 rounded-xl border hover:border-indigo-500/50 hover:bg-slate-900/80 transition-all group cursor-pointer"
          title="View full workspace health and metrics dashboard"
        >
          <div className="flex items-center justify-between text-[11px] font-medium text-slate-300 mb-1.5">
            <span className="flex items-center gap-1.5">
              <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
              <span className="group-hover:text-indigo-300 transition-colors">Agent Mesh</span>
            </span>
            <span className="font-mono text-emerald-400 text-[10px] flex items-center gap-1">
              <span>ONLINE</span>
              <ChevronRight className="w-3 h-3 text-slate-500 group-hover:text-indigo-400 transition-colors" />
            </span>
          </div>
          <div className="space-y-1 text-[10px] text-slate-400 font-mono">
            <div className="flex justify-between">
              <span>Runtime:</span>
              <span className="text-slate-300">HyperVM</span>
            </div>
            <div className="flex justify-between">
              <span>Model Sync:</span>
              <span className="text-indigo-400">18ms RPC</span>
            </div>
          </div>
        </button>
      ) : (
        <button
          onClick={() => onSelectTab('dashboard')}
          className="p-3 border-t border-slate-800/80 bg-slate-950/90 m-2 rounded-xl border flex items-center justify-center text-emerald-400 hover:bg-slate-900 transition-colors"
          title="Agent Mesh Status: Healthy (18ms RPC)"
        >
          <span className="w-2.5 h-2.5 rounded-full bg-emerald-400 animate-pulse" />
        </button>
      )}
    </aside>
  );
};
