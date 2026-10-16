import React, { useState } from 'react';
import { 
  Layers, 
  FolderGit2, 
  Wrench, 
  Database, 
  Clock, 
  ShieldCheck, 
  Monitor, 
  Zap, 
  ArrowRight, 
  Sparkles,
  CheckCircle2,
  HardDrive
} from 'lucide-react';
import { ModelAdapterInfo, TrustBoundary } from '../types';

interface OwnedLayerBlueprintProps {
  activeModel: ModelAdapterInfo;
  trustBoundary: TrustBoundary;
  onNavigateToTab: (tab: any) => void;
}

export const OwnedLayerBlueprint: React.FC<OwnedLayerBlueprintProps> = ({
  activeModel,
  trustBoundary,
  onNavigateToTab
}) => {
  const blocks = [
    {
      id: 'files',
      num: '01',
      title: 'FILES',
      subtitle: 'Your Codebase & Data',
      icon: FolderGit2,
      desc: 'Private repositories, local filesystem mounts via Desktop Bridge, and cloud sandbox volumes. Files stay within your cryptographic boundary.',
      stats: '142 files synced • 0 leaks',
      action: 'desktop-bridge',
      actionText: 'Inspect Desktop Bridge'
    },
    {
      id: 'tools',
      num: '02',
      title: 'TOOLS',
      subtitle: 'Execution Capabilities',
      icon: Wrench,
      desc: 'Containerized bash terminal, AST transform engine, headless Chromium browser with DOM repair pagination, and API connectors.',
      stats: '18 active tool bindings',
      action: 'chat',
      actionText: 'Open Terminal & Tools'
    },
    {
      id: 'memory',
      num: '03',
      title: 'MEMORY',
      subtitle: '4-Tier Vector Persistence',
      icon: Database,
      desc: 'Global preferences, project tech stack facts, conversation thread context, and inter-agent AST knowledge bases.',
      stats: '1,420 vector embeddings',
      action: 'memory',
      actionText: 'Explore Memory Tiers'
    },
    {
      id: 'routines',
      num: '04',
      title: 'ROUTINES',
      subtitle: 'Autonomous Schedules',
      icon: Clock,
      desc: 'Scheduled cron routines that trigger sub-agents in background without manual user prompting. Hourly repo sync, nightly tests.',
      stats: '4 routines running',
      action: 'routines',
      actionText: 'Manage Schedules'
    },
    {
      id: 'permissions',
      num: '05',
      title: 'PERMISSIONS',
      subtitle: 'Shared Trust Boundaries',
      icon: ShieldCheck,
      desc: 'Strict, Guarded, or Permissive execution policies. Enforces approval on external socket writes, curl egress, and git pushes.',
      stats: `Current: ${trustBoundary.toUpperCase()}`,
      action: 'audit-logs',
      actionText: 'View Audit Trail'
    },
    {
      id: 'interfaces',
      num: '06',
      title: 'INTERFACES',
      subtitle: 'Omnichannel Accessibility',
      icon: Monitor,
      desc: 'Access your agent runtime from Desktop, Web, Tablet/iPad, Mobile, headless CLI, or real-time voice call avatar.',
      stats: '6 UI surfaces ready',
      action: 'chat',
      actionText: 'Go to Workspace'
    }
  ];

  return (
    <div className="flex-1 p-6 overflow-y-auto space-y-6">
      {/* Banner */}
      <div className="p-6 rounded-2xl bg-gradient-to-r from-slate-900 via-indigo-950/50 to-slate-900 border border-slate-800 flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <div className="flex items-center gap-2">
            <span className="p-1.5 rounded-lg bg-indigo-500/20 text-indigo-400 border border-indigo-500/30">
              <Layers className="w-5 h-5 text-indigo-400" />
            </span>
            <h2 className="text-base font-extrabold text-slate-100 uppercase tracking-tight">
              "Your Owned Layer" - Architecture Blueprint
            </h2>
            <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-300 border border-emerald-500/30">
              Zero Vendor Lock-In
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-1 max-w-2xl leading-relaxed">
            By separating your Files, Tools, Memory, and Routines from the underlying Model Adapter, you retain complete autonomy, data privacy, and the freedom to swap AI engines instantly.
          </p>
        </div>

        <button
          onClick={() => onNavigateToTab('model-adapter')}
          className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-indigo-600 hover:bg-indigo-500 text-white text-xs font-semibold shadow-md shadow-indigo-600/30 transition-all"
        >
          <Zap className="w-3.5 h-3.5 text-amber-300" />
          <span>Swap Engine ({activeModel.name})</span>
        </button>
      </div>

      {/* 6 Core Blocks Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        {blocks.map((b) => {
          const Icon = b.icon;
          return (
            <div
              key={b.id}
              className="p-5 rounded-2xl bg-slate-900/60 border border-slate-800 flex flex-col justify-between hover:border-slate-700 transition-colors"
            >
              <div>
                <div className="flex items-start justify-between gap-2 mb-3">
                  <div className="flex items-center gap-2.5">
                    <div className="p-2 rounded-xl bg-slate-950 border border-slate-800 text-indigo-400">
                      <Icon className="w-5 h-5" />
                    </div>
                    <div>
                      <div className="flex items-center gap-1.5">
                        <span className="font-mono text-indigo-400 text-xs font-bold">{b.num}</span>
                        <h3 className="text-xs font-bold text-slate-100">{b.title}</h3>
                      </div>
                      <p className="text-[11px] text-slate-400">{b.subtitle}</p>
                    </div>
                  </div>

                  <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-slate-800 text-slate-300">
                    {b.stats}
                  </span>
                </div>

                <p className="text-xs text-slate-300 leading-relaxed mb-4">
                  {b.desc}
                </p>
              </div>

              <div className="pt-3 border-t border-slate-800/80 flex items-center justify-between">
                <button
                  onClick={() => onNavigateToTab(b.action)}
                  className="text-xs font-semibold text-indigo-400 hover:text-indigo-300 flex items-center gap-1"
                >
                  <span>{b.actionText}</span>
                  <ArrowRight className="w-3 h-3" />
                </button>
              </div>
            </div>
          );
        })}
      </div>

      {/* Engine Decoupling Diagram Footer */}
      <div className="p-6 rounded-2xl bg-slate-950 border border-indigo-500/30 flex flex-col md:flex-row items-center justify-between gap-6">
        <div className="space-y-1">
          <div className="flex items-center gap-2">
            <CheckCircle2 className="w-4 h-4 text-emerald-400" />
            <span className="text-xs font-bold text-slate-200">Decoupled Model Adapter Router</span>
          </div>
          <p className="text-xs text-slate-400 max-w-xl">
            Currently streaming via <strong className="text-indigo-300">{activeModel.name}</strong>. If a provider experiences downtime or price changes, the runtime falls back automatically to registered secondary engines with 0ms interruption.
          </p>
        </div>

        <div className="flex items-center gap-2">
          <button
            onClick={() => onNavigateToTab('model-adapter')}
            className="px-4 py-2 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-semibold border border-slate-700 transition-colors"
          >
            Manage Model Adapters
          </button>
        </div>
      </div>
    </div>
  );
};
