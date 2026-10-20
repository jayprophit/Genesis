import React, { useState } from 'react';
import { 
  Clock, 
  Play, 
  Pause, 
  CheckCircle2, 
  Plus, 
  Sparkles, 
  Calendar, 
  ArrowRight,
  RefreshCw,
  Cpu,
  Layers
} from 'lucide-react';
import { Routine } from '../types';

interface RoutinesSchedulerProps {
  routines: Routine[];
  onToggleRoutine: (routineId: string) => void;
  onRunRoutineNow: (routineId: string) => void;
}

export const RoutinesScheduler: React.FC<RoutinesSchedulerProps> = ({
  routines,
  onToggleRoutine,
  onRunRoutineNow
}) => {
  const [runningId, setRunningId] = useState<string | null>(null);

  const handleRun = (id: string) => {
    setRunningId(id);
    setTimeout(() => {
      onRunRoutineNow(id);
      setRunningId(null);
    }, 700);
  };

  return (
    <div className="flex-1 p-6 overflow-y-auto space-y-6">
      {/* Banner */}
      <div className="p-6 rounded-2xl bg-gradient-to-r from-slate-900 via-slate-900 to-indigo-950/40 border border-slate-800 flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <div className="flex items-center gap-2">
            <span className="p-1.5 rounded-lg bg-indigo-500/20 text-indigo-400 border border-indigo-500/30">
              <Clock className="w-5 h-5 text-indigo-400" />
            </span>
            <h2 className="text-base font-extrabold text-slate-100 uppercase tracking-tight">
              Scheduled Routines & Background Cron
            </h2>
            <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-indigo-500/20 text-indigo-300 border border-indigo-500/30">
              Self-Executing Daemons
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-1 max-w-2xl leading-relaxed">
            Configure autonomous agent tasks that run on VPS or serverless triggers: repository sync, vector memory indexing, competitive web scraping, and multi-agent health checks.
          </p>
        </div>

        <div className="flex items-center gap-2">
          <button
            onClick={() => alert("Routine template added: 'Nightly AST Security Audit'")}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-indigo-600 hover:bg-indigo-500 text-white text-xs font-semibold shadow-md shadow-indigo-600/30 transition-all"
          >
            <Plus className="w-3.5 h-3.5" />
            <span>Create Routine</span>
          </button>
        </div>
      </div>

      {/* Routines Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        {routines.map((routine) => {
          const isExecuting = runningId === routine.id;
          const isActive = routine.status === 'active' || routine.enabled;

          return (
            <div
              key={routine.id}
              className={`p-5 rounded-2xl border transition-all flex flex-col justify-between ${
                isActive
                  ? 'bg-slate-900/60 border-slate-800 hover:border-slate-700'
                  : 'bg-slate-950/40 border-slate-900 opacity-60'
              }`}
            >
              <div>
                <div className="flex items-start justify-between gap-2 mb-2">
                  <div className="flex items-center gap-2">
                    <h3 className="text-xs font-bold text-slate-100">{routine.name}</h3>
                    <span className="text-[10px] font-mono px-1.5 py-0.2 rounded bg-indigo-950 text-indigo-300 border border-indigo-800">
                      {routine.cron || routine.schedule}
                    </span>
                  </div>

                  <span className={`text-[10px] font-mono px-2 py-0.5 rounded-full border ${
                    isActive
                      ? 'bg-emerald-500/10 text-emerald-300 border-emerald-500/20'
                      : 'bg-slate-800 text-slate-400 border-slate-700'
                  }`}>
                    {isActive ? 'Active Cron' : 'Disabled'}
                  </span>
                </div>

                <p className="text-xs text-slate-300 leading-relaxed mb-4">
                  {routine.description}
                </p>

                <div className="grid grid-cols-2 gap-2 text-[11px] font-mono p-2.5 rounded-xl bg-slate-950 border border-slate-800 mb-4">
                  <div>
                    <span className="text-[10px] text-slate-500 block">Assigned Worker:</span>
                    <span className="text-indigo-300 font-semibold">{routine.assignedAgent}</span>
                  </div>
                  <div>
                    <span className="text-[10px] text-slate-500 block">Trigger Mechanism:</span>
                    <span className="text-slate-300 capitalize">{routine.triggerType || 'Background Cron'}</span>
                  </div>
                  <div>
                    <span className="text-[10px] text-slate-500 block">Last Run:</span>
                    <span className="text-slate-400">{routine.lastRun}</span>
                  </div>
                  <div>
                    <span className="text-[10px] text-slate-500 block">Next Scheduled:</span>
                    <span className="text-emerald-400 font-semibold">{routine.nextRun}</span>
                  </div>
                </div>
              </div>

              <div className="flex items-center justify-between pt-3 border-t border-slate-800/80">
                <button
                  onClick={() => onToggleRoutine(routine.id)}
                  className="text-xs font-semibold text-slate-400 hover:text-slate-200"
                >
                  {isActive ? 'Pause Routine' : 'Enable Routine'}
                </button>

                <button
                  onClick={() => handleRun(routine.id)}
                  disabled={isExecuting}
                  className="flex items-center gap-1.5 px-3 py-1 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-semibold border border-slate-700 transition-colors disabled:opacity-50"
                >
                  <RefreshCw className={`w-3 h-3 ${isExecuting ? 'animate-spin' : ''}`} />
                  <span>{isExecuting ? 'Running...' : 'Run Now'}</span>
                </button>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
};
