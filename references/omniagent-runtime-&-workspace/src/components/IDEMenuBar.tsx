import React, { useState, useRef, useEffect } from 'react';
import { 
  Cpu, 
  Terminal, 
  Layout, 
  Eye, 
  FileCode, 
  Play, 
  HelpCircle, 
  Check, 
  Layers, 
  PanelLeftClose, 
  PanelLeftOpen,
  PanelRightClose,
  PanelRightOpen,
  PanelBottomClose,
  PanelBottomOpen,
  User,
  Sliders,
  ChevronDown,
  Sparkles
} from 'lucide-react';
import { WorkspaceMode, WorkLayoutPreset, ThemeMode } from '../types';

interface IDEMenuBarProps {
  workspaceMode: WorkspaceMode;
  onSelectWorkspaceMode: (mode: WorkspaceMode) => void;
  layoutPreset: WorkLayoutPreset;
  onSelectLayoutPreset: (preset: WorkLayoutPreset) => void;
  isLeftPanelOpen: boolean;
  onToggleLeftPanel: () => void;
  isRightPanelOpen: boolean;
  onToggleRightPanel: () => void;
  isBottomDockOpen: boolean;
  onToggleBottomDock: () => void;
  theme: ThemeMode;
  onSelectTheme: (theme: ThemeMode) => void;
  onQuickAction?: (action: string) => void;
}

export const IDEMenuBar: React.FC<IDEMenuBarProps> = ({
  workspaceMode,
  onSelectWorkspaceMode,
  layoutPreset,
  onSelectLayoutPreset,
  isLeftPanelOpen,
  onToggleLeftPanel,
  isRightPanelOpen,
  onToggleRightPanel,
  isBottomDockOpen,
  onToggleBottomDock,
  theme,
  onSelectTheme,
  onQuickAction
}) => {
  const [activeMenu, setActiveMenu] = useState<string | null>(null);
  const [isWindowMaximized, setIsWindowMaximized] = useState(false);
  const menuBarRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    function handleClickOutside(e: MouseEvent) {
      if (menuBarRef.current && !menuBarRef.current.contains(e.target as Node)) {
        setActiveMenu(null);
      }
    }
    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
  }, []);

  const handleMenuClick = (menuName: string) => {
    setActiveMenu(activeMenu === menuName ? null : menuName);
  };

  const layoutPresets = [
    { id: 'work-default', label: 'Default Work Mode (Chat + Editor + Terminal)' },
    { id: 'vscode-style', label: 'VS Code Style (Explorer + Editor + Chat + Dock)' },
    { id: 'full-avatar', label: 'Full Avatar Focus (Pure AI Companion Focus)' },
    { id: 'left-closed', label: 'Left Panel Closed (Max Work Focus)' },
    { id: 'right-closed', label: 'Right Panel Closed (AI Chat & Terminal Only)' },
    { id: 'terminal-closed', label: 'Terminal Dock Closed (Clean Editor View)' }
  ];

  const themesList: { id: ThemeMode; label: string; badge: string }[] = [
    { id: 'dark', label: 'Graphite Grey Theme', badge: 'Modern Grey' },
    { id: 'frost', label: 'Frosted Glass Theme', badge: 'Light & Airy' },
    { id: 'amber', label: 'Amber Terminal Theme', badge: 'Warm Amber' },
    { id: 'aurora', label: 'Aurora Theme', badge: 'Vibrant Neon' },
    { id: 'forest', label: 'Forest Theme', badge: 'Emerald Green' }
  ];

  return (
    <div 
      ref={menuBarRef}
      className="h-8 bg-slate-950 border-b border-slate-800/80 px-3 flex items-center justify-between text-xs select-none z-40 relative text-slate-300"
    >
      {/* Left: Window Dots / Brand & Menu Items */}
      <div className="flex items-center gap-3">
        {/* macOS style traffic light window controls */}
        <div className="flex items-center gap-1.5 mr-1">
          <span className="w-3 h-3 rounded-full bg-rose-500/80 hover:bg-rose-500 cursor-pointer shadow-sm" title="Close" />
          <span className="w-3 h-3 rounded-full bg-amber-500/80 hover:bg-amber-500 cursor-pointer shadow-sm" title="Minimize" />
          <span className="w-3 h-3 rounded-full bg-emerald-500/80 hover:bg-emerald-500 cursor-pointer shadow-sm" title="Maximize" />
        </div>

        {/* Brand */}
        <div className="flex items-center gap-1.5 font-bold text-slate-100 pr-2 border-r border-slate-800">
          <span className="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" />
          <span className="tracking-tight text-xs font-black">NOVA / NEXUS AI</span>
        </div>

        {/* Dropdown Menus */}
        <div className="flex items-center space-x-1">
          {/* FILE */}
          <div className="relative">
            <button
              onClick={() => handleMenuClick('file')}
              className={`px-2 py-0.5 rounded text-[11px] hover:bg-slate-800 transition-colors ${
                activeMenu === 'file' ? 'bg-slate-800 text-white font-medium' : 'text-slate-300'
              }`}
            >
              File
            </button>
            {activeMenu === 'file' && (
              <div className="absolute top-full left-0 mt-1 w-52 bg-slate-900 border border-slate-800 rounded-lg shadow-2xl py-1 z-50 text-xs">
                <button 
                  onClick={() => { setActiveMenu(null); onQuickAction?.('new-file'); }}
                  className="w-full text-left px-3 py-1.5 hover:bg-slate-800 flex items-center justify-between"
                >
                  <span>New File</span>
                  <span className="text-[10px] text-slate-500 font-mono">Ctrl+N</span>
                </button>
                <button 
                  onClick={() => { setActiveMenu(null); onQuickAction?.('sync-bridge'); }}
                  className="w-full text-left px-3 py-1.5 hover:bg-slate-800 flex items-center justify-between"
                >
                  <span>Sync Desktop Bridge</span>
                  <span className="text-[10px] text-slate-500 font-mono">Ctrl+O</span>
                </button>
                <button 
                  onClick={() => { setActiveMenu(null); onQuickAction?.('save-file'); }}
                  className="w-full text-left px-3 py-1.5 hover:bg-slate-800 flex items-center justify-between"
                >
                  <span>Save Workspace</span>
                  <span className="text-[10px] text-slate-500 font-mono">Ctrl+S</span>
                </button>
                <div className="h-[1px] bg-slate-800 my-1" />
                <button 
                  onClick={() => { setActiveMenu(null); onQuickAction?.('export-zip'); }}
                  className="w-full text-left px-3 py-1.5 hover:bg-slate-800 text-slate-300"
                >
                  Export Project Bundle
                </button>
              </div>
            )}
          </div>

          {/* EDIT */}
          <div className="relative">
            <button
              onClick={() => handleMenuClick('edit')}
              className={`px-2 py-0.5 rounded text-[11px] hover:bg-slate-800 transition-colors ${
                activeMenu === 'edit' ? 'bg-slate-800 text-white font-medium' : 'text-slate-300'
              }`}
            >
              Edit
            </button>
            {activeMenu === 'edit' && (
              <div className="absolute top-full left-0 mt-1 w-48 bg-slate-900 border border-slate-800 rounded-lg shadow-2xl py-1 z-50 text-xs">
                <button onClick={() => setActiveMenu(null)} className="w-full text-left px-3 py-1.5 hover:bg-slate-800 flex items-center justify-between">
                  <span>Undo</span>
                  <span className="text-[10px] text-slate-500 font-mono">Ctrl+Z</span>
                </button>
                <button onClick={() => setActiveMenu(null)} className="w-full text-left px-3 py-1.5 hover:bg-slate-800 flex items-center justify-between">
                  <span>Redo</span>
                  <span className="text-[10px] text-slate-500 font-mono">Ctrl+Y</span>
                </button>
                <div className="h-[1px] bg-slate-800 my-1" />
                <button onClick={() => setActiveMenu(null)} className="w-full text-left px-3 py-1.5 hover:bg-slate-800 flex items-center justify-between">
                  <span>Format Code</span>
                  <span className="text-[10px] text-slate-500 font-mono">Shift+Alt+F</span>
                </button>
              </div>
            )}
          </div>

          {/* VIEW (Layout Presets & Themes) */}
          <div className="relative">
            <button
              onClick={() => handleMenuClick('view')}
              className={`px-2 py-0.5 rounded text-[11px] hover:bg-slate-800 transition-colors ${
                activeMenu === 'view' ? 'bg-slate-800 text-white font-medium' : 'text-slate-300'
              }`}
            >
              View
            </button>
            {activeMenu === 'view' && (
              <div className="absolute top-full left-0 mt-1 w-64 bg-slate-900 border border-slate-800 rounded-lg shadow-2xl py-1 z-50 text-xs">
                <div className="px-3 py-1 text-[10px] font-mono text-slate-500 uppercase tracking-wider">
                  Layout Presets (Gallery)
                </div>
                {layoutPresets.map(preset => (
                  <button
                    key={preset.id}
                    onClick={() => {
                      onSelectLayoutPreset(preset.id as WorkLayoutPreset);
                      setActiveMenu(null);
                    }}
                    className={`w-full text-left px-3 py-1.5 hover:bg-slate-800 flex items-center justify-between ${
                      layoutPreset === preset.id ? 'text-indigo-400 font-semibold bg-indigo-950/30' : 'text-slate-300'
                    }`}
                  >
                    <span className="truncate">{preset.label}</span>
                    {layoutPreset === preset.id && <Check className="w-3.5 h-3.5 text-indigo-400" />}
                  </button>
                ))}

                <div className="h-[1px] bg-slate-800 my-1" />
                <div className="px-3 py-1 text-[10px] font-mono text-slate-500 uppercase tracking-wider">
                  Themes (10 Themes Collage)
                </div>
                {themesList.map(t => (
                  <button
                    key={t.id}
                    onClick={() => {
                      onSelectTheme(t.id);
                      setActiveMenu(null);
                    }}
                    className={`w-full text-left px-3 py-1.5 hover:bg-slate-800 flex items-center justify-between ${
                      theme === t.id ? 'text-cyan-400 font-semibold bg-cyan-950/30' : 'text-slate-300'
                    }`}
                  >
                    <span>{t.label}</span>
                    <span className="text-[10px] text-slate-500 font-mono">{t.badge}</span>
                  </button>
                ))}

                <div className="h-[1px] bg-slate-800 my-1" />
                <button
                  onClick={() => { onToggleBottomDock(); setActiveMenu(null); }}
                  className="w-full text-left px-3 py-1.5 hover:bg-slate-800 flex items-center justify-between text-slate-300"
                >
                  <span>{isBottomDockOpen ? 'Hide Terminal Dock' : 'Show Terminal Dock'}</span>
                  <span className="text-[10px] text-slate-500 font-mono">Ctrl+`</span>
                </button>
              </div>
            )}
          </div>

          {/* RUN */}
          <div className="relative">
            <button
              onClick={() => handleMenuClick('run')}
              className={`px-2 py-0.5 rounded text-[11px] hover:bg-slate-800 transition-colors ${
                activeMenu === 'run' ? 'bg-slate-800 text-white font-medium' : 'text-slate-300'
              }`}
            >
              Run
            </button>
            {activeMenu === 'run' && (
              <div className="absolute top-full left-0 mt-1 w-52 bg-slate-900 border border-slate-800 rounded-lg shadow-2xl py-1 z-50 text-xs">
                <button onClick={() => { onQuickAction?.('run-code'); setActiveMenu(null); }} className="w-full text-left px-3 py-1.5 hover:bg-slate-800 flex items-center justify-between">
                  <span>Start Without Debugging</span>
                  <span className="text-[10px] text-slate-500 font-mono">Ctrl+F5</span>
                </button>
                <button onClick={() => { onQuickAction?.('run-debug'); setActiveMenu(null); }} className="w-full text-left px-3 py-1.5 hover:bg-slate-800 flex items-center justify-between">
                  <span>Start Debugging</span>
                  <span className="text-[10px] text-slate-500 font-mono">F5</span>
                </button>
                <button onClick={() => { onQuickAction?.('run-cron'); setActiveMenu(null); }} className="w-full text-left px-3 py-1.5 hover:bg-slate-800 flex items-center justify-between">
                  <span>Trigger Routine Cron</span>
                  <span className="text-[10px] text-slate-500 font-mono">Alt+R</span>
                </button>
              </div>
            )}
          </div>

          {/* TERMINAL */}
          <div className="relative">
            <button
              onClick={() => handleMenuClick('terminal')}
              className={`px-2 py-0.5 rounded text-[11px] hover:bg-slate-800 transition-colors ${
                activeMenu === 'terminal' ? 'bg-slate-800 text-white font-medium' : 'text-slate-300'
              }`}
            >
              Terminal
            </button>
            {activeMenu === 'terminal' && (
              <div className="absolute top-full left-0 mt-1 w-48 bg-slate-900 border border-slate-800 rounded-lg shadow-2xl py-1 z-50 text-xs">
                <button 
                  onClick={() => { onToggleBottomDock(); setActiveMenu(null); }} 
                  className="w-full text-left px-3 py-1.5 hover:bg-slate-800"
                >
                  Toggle Terminal
                </button>
                <button 
                  onClick={() => { onQuickAction?.('clear-terminal'); setActiveMenu(null); }} 
                  className="w-full text-left px-3 py-1.5 hover:bg-slate-800"
                >
                  Clear Screen
                </button>
              </div>
            )}
          </div>

          {/* HELP */}
          <div className="relative">
            <button
              onClick={() => handleMenuClick('help')}
              className={`px-2 py-0.5 rounded text-[11px] hover:bg-slate-800 transition-colors ${
                activeMenu === 'help' ? 'bg-slate-800 text-white font-medium' : 'text-slate-300'
              }`}
            >
              Help
            </button>
            {activeMenu === 'help' && (
              <div className="absolute top-full left-0 mt-1 w-48 bg-slate-900 border border-slate-800 rounded-lg shadow-2xl py-1 z-50 text-xs">
                <button onClick={() => { onQuickAction?.('docs'); setActiveMenu(null); }} className="w-full text-left px-3 py-1.5 hover:bg-slate-800">
                  Documentation
                </button>
                <button onClick={() => { onQuickAction?.('shortcuts'); setActiveMenu(null); }} className="w-full text-left px-3 py-1.5 hover:bg-slate-800">
                  Keyboard Shortcuts
                </button>
                <button onClick={() => { onQuickAction?.('about'); setActiveMenu(null); }} className="w-full text-left px-3 py-1.5 hover:bg-slate-800">
                  About Nova / Nexus AI
                </button>
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Center: Layout Presets Shortcut Dropdown */}
      <div className="hidden lg:flex items-center gap-1.5">
        <span className="text-[10px] text-slate-500 font-mono">Layout:</span>
        <select
          value={layoutPreset}
          onChange={(e) => onSelectLayoutPreset(e.target.value as WorkLayoutPreset)}
          className="bg-slate-900 border border-slate-800 rounded px-2 py-0.5 text-[11px] text-slate-300 focus:outline-none focus:border-indigo-500"
        >
          <option value="work-default">Default Work Mode</option>
          <option value="vscode-style">VS Code Style</option>
          <option value="full-avatar">Full Avatar Focus</option>
          <option value="left-closed">Left Panel Closed</option>
          <option value="right-closed">Right Panel Closed</option>
          <option value="terminal-closed">Terminal Closed</option>
        </select>

        {/* Direct Panel Toggle Icons */}
        <div className="flex items-center gap-1 ml-2 border-l border-slate-800 pl-2">
          <button
            onClick={onToggleLeftPanel}
            title={isLeftPanelOpen ? "Close Left Panel" : "Open Left Panel"}
            className={`p-1 rounded ${isLeftPanelOpen ? 'text-indigo-400 bg-slate-800/80' : 'text-slate-500 hover:text-slate-300'}`}
          >
            {isLeftPanelOpen ? <PanelLeftClose className="w-3.5 h-3.5" /> : <PanelLeftOpen className="w-3.5 h-3.5" />}
          </button>
          <button
            onClick={onToggleBottomDock}
            title={isBottomDockOpen ? "Close Terminal Dock" : "Open Terminal Dock"}
            className={`p-1 rounded ${isBottomDockOpen ? 'text-indigo-400 bg-slate-800/80' : 'text-slate-500 hover:text-slate-300'}`}
          >
            {isBottomDockOpen ? <PanelBottomClose className="w-3.5 h-3.5" /> : <PanelBottomOpen className="w-3.5 h-3.5" />}
          </button>
          <button
            onClick={onToggleRightPanel}
            title={isRightPanelOpen ? "Close Right Panel" : "Open Right Panel"}
            className={`p-1 rounded ${isRightPanelOpen ? 'text-indigo-400 bg-slate-800/80' : 'text-slate-500 hover:text-slate-300'}`}
          >
            {isRightPanelOpen ? <PanelRightClose className="w-3.5 h-3.5" /> : <PanelRightOpen className="w-3.5 h-3.5" />}
          </button>
        </div>
      </div>

      {/* Right: Mode Switcher Pills (Chat vs Work) & Window Controls */}
      <div className="flex items-center gap-3">
        {/* Chat | Work Pills as prominent in all images! */}
        <div className="flex items-center bg-slate-900 border border-slate-800 rounded-lg p-0.5">
          <button
            onClick={() => onSelectWorkspaceMode('chat')}
            className={`px-3 py-0.5 rounded text-[11px] font-semibold transition-all ${
              workspaceMode === 'chat'
                ? 'bg-indigo-600 text-white shadow-sm'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            Chat
          </button>
          <button
            onClick={() => onSelectWorkspaceMode('work')}
            className={`px-3 py-0.5 rounded text-[11px] font-semibold transition-all ${
              workspaceMode === 'work'
                ? 'bg-indigo-600 text-white shadow-sm'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            Work
          </button>
        </div>

        {/* Windows style minimize / maximize / close */}
        <div className="flex items-center space-x-1 text-slate-500 pl-1 border-l border-slate-800">
          <button 
            onClick={() => onSelectLayoutPreset('full-avatar')}
            title="Minimize to pure focus"
            className="w-5 h-5 flex items-center justify-center hover:bg-slate-800 hover:text-slate-200 rounded"
          >
            —
          </button>
          <button 
            onClick={() => setIsWindowMaximized(!isWindowMaximized)}
            title="Maximize layout"
            className="w-5 h-5 flex items-center justify-center hover:bg-slate-800 hover:text-slate-200 rounded"
          >
            ▢
          </button>
          <button 
            onClick={() => onSelectLayoutPreset('work-default')}
            title="Reset default view"
            className="w-5 h-5 flex items-center justify-center hover:bg-rose-500/20 hover:text-rose-400 rounded"
          >
            ✕
          </button>
        </div>
      </div>
    </div>
  );
};
