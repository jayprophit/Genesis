import React, { useState } from 'react';
import { 
  Terminal as TerminalIcon, 
  AlertTriangle, 
  FileText, 
  PlaySquare, 
  Trash2, 
  ChevronUp, 
  ChevronDown, 
  X, 
  Maximize2, 
  CheckCircle2,
  Bug,
  Info
} from 'lucide-react';
import { BottomDockTab } from '../types';

interface BottomDockPanelProps {
  isOpen: boolean;
  onClose: () => void;
  activeModelName: string;
}

export const BottomDockPanel: React.FC<BottomDockPanelProps> = ({
  isOpen,
  onClose,
  activeModelName
}) => {
  const [activeTab, setActiveTab] = useState<BottomDockTab>('terminal');
  const [isMaximized, setIsMaximized] = useState(false);
  
  // Terminal state
  const [terminalInput, setTerminalInput] = useState('');
  const [terminalLogs, setTerminalLogs] = useState<string[]>([
    'PS C:\\Projects\\Nexus> python -m venv .venv',
    'PS C:\\Projects\\Nexus> .\\.venv\\Scripts\\Activate.ps1',
    '(.venv) PS C:\\Projects\\Nexus> pip install -r requirements.txt',
    '[omniagent-bridge] Local workstation bridge verified. HMAC: OK',
    '[adapter:route] Streaming via ' + activeModelName + ' (zero downtime hot-swap ready)',
    '(.venv) PS C:\\Projects\\Nexus> _'
  ]);

  // Problems state
  const problems = [
    {
      severity: 'warning',
      file: 'src/main.py',
      line: 12,
      column: 1,
      code: 'W0611',
      message: "Unused import 'sys' - (unused-import)",
      source: 'Pylance'
    },
    {
      severity: 'info',
      file: 'src/runtime.config.ts',
      line: 4,
      column: 18,
      code: 'TS2304',
      message: "Type assertion recommended for dynamic ModelAdapter dispatch",
      source: 'TypeScript'
    }
  ];

  // Output logs
  const outputLogs = [
    '[Build System] Incremental compilation completed in 214ms.',
    '[Vite] HMR disabled per container orchestration directives.',
    '[Desktop Bridge] 142 local repository files indexed. Inotify file watcher: ACTIVE.',
    '[Model Router] Active adapter: ' + activeModelName + '. Context window: 1M tokens.',
    '[Sub-Agents] Mesh cluster initialized. 6 agents registered in speaking queue.'
  ];

  // Debug Console state
  const [debugInput, setDebugInput] = useState('');
  const [debugLogs, setDebugLogs] = useState<string[]>([
    'OmniAgent Interactive Debug Console v3.4',
    '> typeof OmniAgentRuntime',
    '  "function"',
    '> runtime.getActiveEngine()',
    '  { name: "' + activeModelName + '", status: "online", latencyMs: 142 }'
  ]);

  if (!isOpen) return null;

  const handleTerminalSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (!terminalInput.trim()) return;
    const cmd = terminalInput.trim();
    setTerminalInput('');

    if (cmd === 'clear' || cmd === 'cls') {
      setTerminalLogs(['(.venv) PS C:\\Projects\\Nexus> _']);
      return;
    }

    setTerminalLogs(prev => [
      ...prev.slice(0, -1),
      `(.venv) PS C:\\Projects\\Nexus> ${cmd}`,
      `[exec] Command finished with exit code 0.`,
      '(.venv) PS C:\\Projects\\Nexus> _'
    ]);
  };

  const handleDebugSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (!debugInput.trim()) return;
    const exp = debugInput.trim();
    setDebugInput('');

    let result = '';
    try {
      if (exp.includes('+') || exp.includes('*') || exp.includes('-') || !isNaN(Number(exp))) {
        result = String(eval(exp));
      } else {
        result = `Evaluated: [${exp}] -> OK (200)`;
      }
    } catch (e: any) {
      result = `SyntaxError: ${e?.message}`;
    }

    setDebugLogs(prev => [
      ...prev,
      `> ${exp}`,
      `  ${result}`
    ]);
  };

  return (
    <div className={`border-t border-slate-800 bg-slate-950 flex flex-col shrink-0 transition-all duration-200 select-none ${
      isMaximized ? 'h-96' : 'h-52 sm:h-56'
    }`}>
      {/* Dock Header Tabs */}
      <div className="h-9 px-3 border-b border-slate-800/80 bg-slate-950 flex items-center justify-between text-xs">
        <div className="flex items-center gap-1">
          <button
            onClick={() => setActiveTab('terminal')}
            className={`flex items-center gap-1.5 px-3 py-1 rounded text-xs font-mono font-medium transition-colors ${
              activeTab === 'terminal'
                ? 'bg-slate-800/80 text-indigo-300 font-semibold border-b-2 border-indigo-500'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
            }`}
          >
            <TerminalIcon className="w-3.5 h-3.5" />
            <span className="uppercase text-[11px] tracking-wider">Terminal</span>
          </button>

          <button
            onClick={() => setActiveTab('problems')}
            className={`flex items-center gap-1.5 px-3 py-1 rounded text-xs font-mono font-medium transition-colors ${
              activeTab === 'problems'
                ? 'bg-slate-800/80 text-indigo-300 font-semibold border-b-2 border-indigo-500'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
            }`}
          >
            <AlertTriangle className="w-3.5 h-3.5 text-amber-400" />
            <span className="uppercase text-[11px] tracking-wider">Problems</span>
            <span className="text-[10px] px-1.5 py-0.2 rounded-full bg-amber-500/20 text-amber-300 font-bold">
              {problems.length}
            </span>
          </button>

          <button
            onClick={() => setActiveTab('output')}
            className={`flex items-center gap-1.5 px-3 py-1 rounded text-xs font-mono font-medium transition-colors ${
              activeTab === 'output'
                ? 'bg-slate-800/80 text-indigo-300 font-semibold border-b-2 border-indigo-500'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
            }`}
          >
            <FileText className="w-3.5 h-3.5" />
            <span className="uppercase text-[11px] tracking-wider">Output</span>
          </button>

          <button
            onClick={() => setActiveTab('debug')}
            className={`flex items-center gap-1.5 px-3 py-1 rounded text-xs font-mono font-medium transition-colors ${
              activeTab === 'debug'
                ? 'bg-slate-800/80 text-indigo-300 font-semibold border-b-2 border-indigo-500'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
            }`}
          >
            <PlaySquare className="w-3.5 h-3.5 text-emerald-400" />
            <span className="uppercase text-[11px] tracking-wider">Debug Console</span>
          </button>
        </div>

        {/* Dock Controls */}
        <div className="flex items-center gap-1 text-slate-400">
          <button
            onClick={() => setIsMaximized(!isMaximized)}
            className="p-1 rounded hover:bg-slate-800 hover:text-slate-200"
            title={isMaximized ? "Restore dock size" : "Maximize dock panel"}
          >
            {isMaximized ? <ChevronDown className="w-3.5 h-3.5" /> : <ChevronUp className="w-3.5 h-3.5" />}
          </button>
          <button
            onClick={onClose}
            className="p-1 rounded hover:bg-slate-800 hover:text-rose-400"
            title="Close dock (re-open anytime)"
          >
            <X className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>

      {/* TAB 1: Terminal */}
      {activeTab === 'terminal' && (
        <div className="flex-1 flex flex-col font-mono text-xs overflow-hidden bg-slate-950 p-2.5">
          <div className="flex-1 overflow-y-auto space-y-1 text-slate-300">
            {terminalLogs.map((log, idx) => (
              <div key={idx} className="leading-relaxed">
                {log.startsWith('PS') || log.startsWith('(') ? (
                  <span className="text-cyan-400">{log}</span>
                ) : log.includes('error') ? (
                  <span className="text-rose-400">{log}</span>
                ) : log.includes('OK') || log.includes('verified') ? (
                  <span className="text-emerald-400">{log}</span>
                ) : (
                  <span>{log}</span>
                )}
              </div>
            ))}
          </div>

          <form onSubmit={handleTerminalSubmit} className="pt-2 border-t border-slate-900 flex items-center gap-2">
            <span className="text-cyan-400 font-bold select-none">(.venv) PS C:\Projects\Nexus&gt;</span>
            <input
              type="text"
              value={terminalInput}
              onChange={(e) => setTerminalInput(e.target.value)}
              placeholder="Run command ('git status', 'pytest', 'pip install', 'clear')..."
              className="flex-1 bg-transparent text-slate-100 placeholder-slate-600 focus:outline-none text-xs"
            />
          </form>
        </div>
      )}

      {/* TAB 2: Problems */}
      {activeTab === 'problems' && (
        <div className="flex-1 overflow-y-auto p-3 font-mono text-xs divide-y divide-slate-800/60">
          {problems.map((p, idx) => (
            <div key={idx} className="py-2 flex items-start justify-between gap-4 hover:bg-slate-900/40 px-2 rounded">
              <div className="flex items-start gap-2">
                <AlertTriangle className="w-3.5 h-3.5 text-amber-400 shrink-0 mt-0.5" />
                <div>
                  <span className="text-slate-200">{p.message}</span>
                  <span className="text-slate-500 ml-2">[{p.source} {p.code}]</span>
                </div>
              </div>
              <span className="text-indigo-400 shrink-0 text-[11px]">
                {p.file}:{p.line}:{p.column}
              </span>
            </div>
          ))}
        </div>
      )}

      {/* TAB 3: Output */}
      {activeTab === 'output' && (
        <div className="flex-1 overflow-y-auto p-3 font-mono text-xs space-y-1.5 text-slate-400">
          {outputLogs.map((log, idx) => (
            <div key={idx} className="leading-relaxed">
              <span className="text-slate-600">[{new Date().toLocaleTimeString()}]</span> {log}
            </div>
          ))}
        </div>
      )}

      {/* TAB 4: Debug Console */}
      {activeTab === 'debug' && (
        <div className="flex-1 flex flex-col font-mono text-xs overflow-hidden p-2.5">
          <div className="flex-1 overflow-y-auto space-y-1 text-slate-300">
            {debugLogs.map((log, idx) => (
              <div key={idx} className={log.startsWith('>') ? 'text-indigo-300 font-bold' : 'text-slate-400 pl-2'}>
                {log}
              </div>
            ))}
          </div>

          <form onSubmit={handleDebugSubmit} className="pt-2 border-t border-slate-900 flex items-center gap-2">
            <span className="text-indigo-400 font-bold">&gt;</span>
            <input
              type="text"
              value={debugInput}
              onChange={(e) => setDebugInput(e.target.value)}
              placeholder="Evaluate expression in sandbox runtime..."
              className="flex-1 bg-transparent text-slate-100 placeholder-slate-600 focus:outline-none text-xs"
            />
          </form>
        </div>
      )}
    </div>
  );
};
