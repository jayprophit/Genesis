import React, { useState } from 'react';
import { 
  Database, 
  Search, 
  Plus, 
  Trash2, 
  Share2, 
  Sparkles, 
  Lock, 
  FileText, 
  Tag, 
  CheckCircle,
  FolderGit2,
  Cpu,
  Layers
} from 'lucide-react';
import { MemoryItem } from '../types';

interface MemoryInspectorProps {
  memoryItems: MemoryItem[];
  onAddMemory: (tier: MemoryItem['tier'], title: string, content: string, tags: string[]) => void;
}

export const MemoryInspector: React.FC<MemoryInspectorProps> = ({
  memoryItems,
  onAddMemory
}) => {
  const [activeTier, setActiveTier] = useState<MemoryItem['tier'] | 'all'>('all');
  const [searchQuery, setSearchQuery] = useState('');
  const [showAddModal, setShowAddModal] = useState(false);
  
  // Add modal fields
  const [newTier, setNewTier] = useState<MemoryItem['tier']>('global');
  const [newTitle, setNewTitle] = useState('');
  const [newContent, setNewContent] = useState('');
  const [newTags, setNewTags] = useState('');

  const tiers: { id: MemoryItem['tier']; label: string; desc: string; icon: string }[] = [
    { id: 'global', label: 'Global Memory', desc: 'Your preferences, identity, long-term architectural facts', icon: '🌐' },
    { id: 'project', label: 'Project Memory', desc: 'Active workspace repository configs and files', icon: '📁' },
    { id: 'thread', label: 'Thread Memory', desc: 'Context preserved across active multi-agent conversation', icon: '💬' },
    { id: 'agent', label: 'Agent Memory', desc: 'Specialized AST caches and knowledge workers share', icon: '🤖' }
  ];

  const filteredItems = memoryItems.filter(item => {
    const matchesTier = activeTier === 'all' || item.tier === activeTier;
    const matchesQuery = !searchQuery || 
      item.title.toLowerCase().includes(searchQuery.toLowerCase()) ||
      item.content.toLowerCase().includes(searchQuery.toLowerCase()) ||
      item.tags.some(t => t.toLowerCase().includes(searchQuery.toLowerCase()));
    return matchesTier && matchesQuery;
  });

  const handleCreate = (e: React.FormEvent) => {
    e.preventDefault();
    if (!newTitle.trim() || !newContent.trim()) return;
    const tagsArray = newTags.split(',').map(t => t.trim()).filter(Boolean);
    onAddMemory(newTier, newTitle, newContent, tagsArray);
    setNewTitle('');
    setNewContent('');
    setNewTags('');
    setShowAddModal(false);
  };

  return (
    <div className="flex-1 p-6 overflow-y-auto space-y-6">
      {/* Banner */}
      <div className="p-6 rounded-2xl bg-gradient-to-r from-slate-900 via-slate-900 to-indigo-950/40 border border-slate-800 flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <div className="flex items-center gap-2">
            <span className="p-1.5 rounded-lg bg-indigo-500/20 text-indigo-400 border border-indigo-500/30">
              <Database className="w-5 h-5 text-indigo-400" />
            </span>
            <h2 className="text-base font-extrabold text-slate-100 uppercase tracking-tight">
              4-Tier Shared Memory & Local Pipelines
            </h2>
            <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-indigo-500/20 text-indigo-300 border border-indigo-500/30">
              1,420 Vector Dimensions
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-1 max-w-2xl leading-relaxed">
            As highlighted in the architecture slides: multiple chat threads share persistent vector memory divided into Global, Project, Thread, and Agent partitions.
          </p>
        </div>

        <button
          onClick={() => setShowAddModal(true)}
          className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-indigo-600 hover:bg-indigo-500 text-white text-xs font-semibold shadow-md shadow-indigo-600/30 transition-all"
        >
          <Plus className="w-3.5 h-3.5" />
          <span>Add Memory Fact</span>
        </button>
      </div>

      {/* Tier Selector Cards */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-3">
        {tiers.map((t) => {
          const isActive = activeTier === t.id;
          const count = memoryItems.filter(m => m.tier === t.id).length;

          return (
            <div
              key={t.id}
              onClick={() => setActiveTier(isActive ? 'all' : t.id)}
              className={`p-3.5 rounded-2xl border transition-all cursor-pointer flex flex-col justify-between ${
                isActive
                  ? 'bg-indigo-600/20 border-indigo-500 ring-1 ring-indigo-500/40'
                  : 'bg-slate-900/60 border-slate-800 hover:border-slate-700'
              }`}
            >
              <div>
                <div className="flex items-center justify-between mb-1.5">
                  <span className="text-lg">{t.icon}</span>
                  <span className="text-[10px] font-mono px-2 py-0.5 rounded-full bg-slate-800 text-slate-300">
                    {count} facts
                  </span>
                </div>
                <h4 className="text-xs font-bold text-slate-100">{t.label}</h4>
                <p className="text-[11px] text-slate-400 mt-0.5 leading-snug">{t.desc}</p>
              </div>

              <div className="mt-3 pt-2 border-t border-slate-800/80 text-[10px] text-indigo-400 font-mono">
                {isActive ? 'Filtered Active' : 'Click to filter'}
              </div>
            </div>
          );
        })}
      </div>

      {/* Search Bar */}
      <div className="flex items-center gap-3">
        <div className="relative flex-1">
          <Search className="w-4 h-4 text-slate-400 absolute left-3.5 top-1/2 -translate-y-1/2" />
          <input
            type="text"
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            placeholder="Search vector memory by keyword, tag, or semantic query..."
            className="w-full bg-slate-900 border border-slate-800 rounded-xl pl-9 pr-4 py-2 text-xs text-slate-100 placeholder-slate-500 focus:outline-none focus:border-indigo-500"
          />
        </div>
        {activeTier !== 'all' && (
          <button
            onClick={() => setActiveTier('all')}
            className="px-3 py-2 rounded-xl bg-slate-800 text-slate-300 text-xs font-medium hover:bg-slate-700"
          >
            Clear Filter ({activeTier})
          </button>
        )}
      </div>

      {/* Memory Items List */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        {filteredItems.map((item) => (
          <div
            key={item.id}
            className="p-4 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-3 hover:border-slate-700 transition-colors"
          >
            <div className="flex items-start justify-between gap-2">
              <div>
                <div className="flex items-center gap-2">
                  <span className="text-xs font-bold text-slate-100">{item.title}</span>
                  <span className="text-[10px] font-mono uppercase px-1.5 py-0.2 rounded bg-indigo-950 text-indigo-300 border border-indigo-800">
                    Tier: {item.tier}
                  </span>
                </div>
                <span className="text-[10px] text-slate-500 font-mono">Updated: {item.updatedAt}</span>
              </div>

              {item.vectorScore && (
                <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-emerald-500/10 text-emerald-300 border border-emerald-500/20">
                  Cosine {(item.vectorScore * 100).toFixed(0)}%
                </span>
              )}
            </div>

            <p className="text-xs text-slate-300 leading-relaxed bg-slate-950/60 p-3 rounded-xl border border-slate-800/80">
              {item.content}
            </p>

            <div className="flex flex-wrap gap-1">
              {item.tags.map((tag, idx) => (
                <span key={idx} className="text-[10px] font-mono px-2 py-0.5 rounded bg-slate-800 text-slate-400">
                  #{tag}
                </span>
              ))}
            </div>
          </div>
        ))}
      </div>

      {/* Add Memory Modal */}
      {showAddModal && (
        <div className="fixed inset-0 bg-slate-950/80 backdrop-blur-sm z-50 flex items-center justify-center p-4">
          <div className="bg-slate-900 border border-slate-800 rounded-2xl w-full max-w-lg p-6 space-y-4 shadow-2xl">
            <div className="flex items-center justify-between pb-3 border-b border-slate-800">
              <h3 className="text-sm font-bold text-slate-100">Add Fact to 4-Tier Memory</h3>
              <button
                onClick={() => setShowAddModal(false)}
                className="text-slate-400 hover:text-slate-200 text-xs"
              >
                Close
              </button>
            </div>

            <form onSubmit={handleCreate} className="space-y-3 text-xs">
              <div>
                <label className="block text-slate-400 mb-1 font-medium">Memory Partition Tier</label>
                <select
                  value={newTier}
                  onChange={(e) => setNewTier(e.target.value as any)}
                  className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-slate-200"
                >
                  <option value="global">Global Memory (User preferences, identity)</option>
                  <option value="project">Project Memory (Active repository, stack)</option>
                  <option value="thread">Thread Memory (Conversation session)</option>
                  <option value="agent">Agent Memory (Sub-agent shared knowledge)</option>
                </select>
              </div>

              <div>
                <label className="block text-slate-400 mb-1 font-medium">Fact Title</label>
                <input
                  type="text"
                  value={newTitle}
                  onChange={(e) => setNewTitle(e.target.value)}
                  placeholder="e.g. VPS Docker Compose Deployment Standard"
                  className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-slate-200 focus:outline-none focus:border-indigo-500"
                />
              </div>

              <div>
                <label className="block text-slate-400 mb-1 font-medium">Knowledge Content</label>
                <textarea
                  value={newContent}
                  onChange={(e) => setNewContent(e.target.value)}
                  rows={3}
                  placeholder="Describe the rule, configuration, or architectural constraint..."
                  className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-slate-200 focus:outline-none focus:border-indigo-500 resize-none"
                />
              </div>

              <div>
                <label className="block text-slate-400 mb-1 font-medium">Tags (comma separated)</label>
                <input
                  type="text"
                  value={newTags}
                  onChange={(e) => setNewTags(e.target.value)}
                  placeholder="docker, vps, deploy, architecture"
                  className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-slate-200 focus:outline-none focus:border-indigo-500"
                />
              </div>

              <div className="pt-3 border-t border-slate-800 flex justify-end gap-2">
                <button
                  type="button"
                  onClick={() => setShowAddModal(false)}
                  className="px-3 py-1.5 rounded-xl bg-slate-800 text-slate-300 font-medium hover:bg-slate-700"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  className="px-4 py-1.5 rounded-xl bg-indigo-600 hover:bg-indigo-500 text-white font-semibold shadow"
                >
                  Commit Memory Vector
                </button>
              </div>
            </form>
          </div>
        </div>
      )}
    </div>
  );
};
