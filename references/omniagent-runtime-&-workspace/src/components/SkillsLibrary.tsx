import React, { useState } from 'react';
import { 
  Wrench, 
  Sparkles, 
  Play, 
  TrendingUp, 
  CheckCircle2, 
  Code2, 
  Plus, 
  RefreshCw,
  Cpu,
  Layers,
  FileCode,
  ShieldCheck
} from 'lucide-react';
import { Skill } from '../types';

interface SkillsLibraryProps {
  skills: Skill[];
  onImproveSkill: (skillId: string) => void;
  onRunSkill: (skillName: string) => void;
}

export const SkillsLibrary: React.FC<SkillsLibraryProps> = ({
  skills,
  onImproveSkill,
  onRunSkill
}) => {
  const [selectedSkill, setSelectedSkill] = useState<Skill>(skills[0]);
  const [improvingId, setImprovingId] = useState<string | null>(null);
  const [filterCategory, setFilterCategory] = useState<string>('All');
  const [showAddModal, setShowAddModal] = useState(false);

  const categories = ['All', 'Web & Scrape', 'Desktop Bridge', 'System', 'Development'];

  const filteredSkills = filterCategory === 'All' 
    ? skills 
    : skills.filter(s => s.category === filterCategory);

  const handleTriggerImprove = (skillId: string) => {
    setImprovingId(skillId);
    setTimeout(() => {
      onImproveSkill(skillId);
      setImprovingId(null);
    }, 600);
  };

  return (
    <div className="flex-1 p-6 overflow-y-auto space-y-6">
      {/* Banner */}
      <div className="p-6 rounded-2xl bg-gradient-to-r from-slate-900 via-slate-900 to-indigo-950/50 border border-slate-800 flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <div className="flex items-center gap-2">
            <span className="p-1.5 rounded-lg bg-indigo-500/20 text-indigo-400 border border-indigo-500/30">
              <Sparkles className="w-5 h-5 text-indigo-400" />
            </span>
            <h2 className="text-base font-extrabold text-slate-100 uppercase tracking-tight">
              Reusable Skills & Self-Improvement Loop
            </h2>
            <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-indigo-500/20 text-indigo-300 border border-indigo-500/30">
              Hermes Agent Pattern
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-1 max-w-2xl leading-relaxed">
            Skills are versioned, modular routines that the agent runtime uses to execute tasks. The autonomous self-improvement loop optimizes DOM selectors, error recovery, and AST transforms after each execution.
          </p>
        </div>

        <div className="flex items-center gap-2">
          <button
            onClick={() => handleTriggerImprove(selectedSkill.id)}
            disabled={Boolean(improvingId)}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-indigo-600 hover:bg-indigo-500 text-white text-xs font-semibold shadow-md shadow-indigo-600/30 transition-all"
          >
            <RefreshCw className={`w-3.5 h-3.5 ${improvingId ? 'animate-spin' : ''}`} />
            <span>{improvingId ? 'Optimizing Code...' : 'Trigger Self-Improvement'}</span>
          </button>
        </div>
      </div>

      {/* Category Pills */}
      <div className="flex items-center gap-2 overflow-x-auto pb-1 text-xs">
        {categories.map((cat) => (
          <button
            key={cat}
            onClick={() => setFilterCategory(cat)}
            className={`px-3 py-1.5 rounded-xl font-medium transition-colors ${
              filterCategory === cat
                ? 'bg-indigo-600 text-white shadow-sm'
                : 'bg-slate-900 text-slate-400 hover:text-slate-200 border border-slate-800'
            }`}
          >
            {cat}
          </button>
        ))}
      </div>

      {/* Skills Grid & Selected Skill Inspector */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Left 2 Cols: Skill Cards */}
        <div className="lg:col-span-2 grid grid-cols-1 sm:grid-cols-2 gap-4">
          {filteredSkills.map((skill) => {
            const isSelected = selectedSkill.id === skill.id;

            return (
              <div
                key={skill.id}
                onClick={() => setSelectedSkill(skill)}
                className={`p-4 rounded-2xl border transition-all cursor-pointer flex flex-col justify-between ${
                  isSelected
                    ? 'bg-slate-900/90 border-indigo-500 ring-1 ring-indigo-500/40 shadow-lg'
                    : 'bg-slate-900/40 border-slate-800 hover:border-slate-700 hover:bg-slate-900/70'
                }`}
              >
                <div>
                  <div className="flex items-start justify-between gap-2 mb-2">
                    <div>
                      <div className="flex items-center gap-1.5">
                        <h4 className="text-xs font-bold text-slate-100">{skill.name}</h4>
                        <span className="text-[10px] font-mono px-1.5 py-0.2 rounded bg-slate-800 text-indigo-300">
                          v{skill.version}
                        </span>
                      </div>
                      <span className="text-[10px] text-slate-400">{skill.category}</span>
                    </div>

                    <span className="text-[10px] font-mono px-1.5 py-0.5 rounded bg-emerald-500/10 text-emerald-300 border border-emerald-500/20">
                      {skill.successRate}% Success
                    </span>
                  </div>

                  <p className="text-xs text-slate-300 leading-relaxed mb-3 line-clamp-2">
                    {skill.description}
                  </p>

                  <div className="flex flex-wrap gap-1 mb-3">
                    {skill.parameters.map((param, idx) => (
                      <span key={idx} className="text-[9px] font-mono px-1.5 py-0.5 rounded bg-slate-950 border border-slate-800 text-slate-400">
                        {param}
                      </span>
                    ))}
                  </div>
                </div>

                <div className="flex items-center justify-between pt-2 border-t border-slate-800/80 text-[10px] text-slate-400 font-mono">
                  <span>Improved {skill.selfImprovedTimes}x</span>
                  <div className="flex items-center gap-1.5">
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        onRunSkill(skill.name);
                      }}
                      className="px-2 py-0.5 rounded bg-slate-800 hover:bg-slate-700 text-slate-200 transition-colors flex items-center gap-1"
                    >
                      <Play className="w-2.5 h-2.5 text-emerald-400" />
                      <span>Run Test</span>
                    </button>
                  </div>
                </div>
              </div>
            );
          })}
        </div>

        {/* Right Col: Code Viewer & Ast Optimization Inspector */}
        <div className="p-5 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-4 flex flex-col justify-between">
          <div className="space-y-3">
            <div className="flex items-center justify-between pb-2 border-b border-slate-800">
              <div>
                <h3 className="text-xs font-bold text-slate-100">{selectedSkill.name}</h3>
                <span className="text-[10px] font-mono text-slate-400">Version {selectedSkill.version} (Active)</span>
              </div>
              <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-indigo-500/20 text-indigo-300">
                {selectedSkill.runsCount} Total Invocations
              </span>
            </div>

            <div className="space-y-1.5">
              <span className="text-[11px] font-semibold text-slate-300">Source Implementation:</span>
              <div className="rounded-xl overflow-hidden border border-slate-800 bg-slate-950 font-mono text-[11px]">
                <pre className="p-3 text-slate-300 overflow-x-auto leading-relaxed max-h-64">
                  <code>{selectedSkill.codeSnippet}</code>
                </pre>
              </div>
            </div>

            <div className="p-3 rounded-xl bg-slate-950 border border-slate-800 text-xs space-y-1.5">
              <div className="flex items-center gap-1.5 text-emerald-400 font-semibold text-[11px]">
                <ShieldCheck className="w-3.5 h-3.5" />
                <span>Self-Improvement Loop Stats</span>
              </div>
              <p className="text-[11px] text-slate-400 leading-relaxed">
                AST rewrite engine has automatically refactored timeout handlers and validated against 12 unit tests without manual human intervention.
              </p>
            </div>
          </div>

          <div className="pt-3 border-t border-slate-800 flex gap-2">
            <button
              onClick={() => handleTriggerImprove(selectedSkill.id)}
              disabled={Boolean(improvingId)}
              className="flex-1 py-2 rounded-xl bg-indigo-600 hover:bg-indigo-500 text-white font-semibold text-xs transition-colors"
            >
              {improvingId ? 'Optimizing...' : 'Self-Improve Skill'}
            </button>
            <button
              onClick={() => onRunSkill(selectedSkill.name)}
              className="px-3 py-2 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-semibold"
            >
              Run In Sandbox
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};
