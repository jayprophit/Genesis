import React, { useState } from 'react';
import { 
  Folder, 
  FolderOpen, 
  FileCode, 
  FileText, 
  ChevronRight, 
  ChevronDown, 
  Plus, 
  RefreshCw, 
  Search,
  Settings,
  MoreVertical
} from 'lucide-react';

export interface FileItem {
  id: string;
  name: string;
  path: string;
  type: 'file' | 'folder';
  children?: FileItem[];
  size?: string;
  language?: string;
}

interface FileExplorerTreeProps {
  activeFilePath: string;
  onSelectFile: (file: FileItem) => void;
}

export const initialProjectTree: FileItem = {
  id: 'root',
  name: 'MyProject',
  path: '/workspace/MyProject',
  type: 'folder',
  children: [
    {
      id: 'src',
      name: 'src',
      path: '/workspace/MyProject/src',
      type: 'folder',
      children: [
        {
          id: 'main.py',
          name: 'main.py',
          path: '/workspace/MyProject/src/main.py',
          type: 'file',
          size: '1.2 KB',
          language: 'python'
        },
        {
          id: 'utils.py',
          name: 'utils.py',
          path: '/workspace/MyProject/src/utils.py',
          type: 'file',
          size: '2.8 KB',
          language: 'python'
        },
        {
          id: 'runtime.config.ts',
          name: 'runtime.config.ts',
          path: '/workspace/MyProject/src/runtime.config.ts',
          type: 'file',
          size: '3.4 KB',
          language: 'typescript'
        }
      ]
    },
    {
      id: 'tests',
      name: 'tests',
      path: '/workspace/MyProject/tests',
      type: 'folder',
      children: [
        {
          id: 'test_runtime.py',
          name: 'test_runtime.py',
          path: '/workspace/MyProject/tests/test_runtime.py',
          type: 'file',
          size: '1.9 KB',
          language: 'python'
        }
      ]
    },
    {
      id: 'README.md',
      name: 'README.md',
      path: '/workspace/MyProject/README.md',
      type: 'file',
      size: '2.1 KB',
      language: 'markdown'
    },
    {
      id: 'requirements.txt',
      name: 'requirements.txt',
      path: '/workspace/MyProject/requirements.txt',
      type: 'file',
      size: '412 B',
      language: 'text'
    },
    {
      id: 'docker-compose.yml',
      name: 'docker-compose.yml',
      path: '/workspace/MyProject/docker-compose.yml',
      type: 'file',
      size: '1.8 KB',
      language: 'yaml'
    }
  ]
};

export const FileExplorerTree: React.FC<FileExplorerTreeProps> = ({
  activeFilePath,
  onSelectFile
}) => {
  const [openFolders, setOpenFolders] = useState<Record<string, boolean>>({
    root: true,
    src: true,
    tests: false
  });
  const [searchFilter, setSearchFilter] = useState('');

  const toggleFolder = (folderId: string) => {
    setOpenFolders(prev => ({
      ...prev,
      [folderId]: !prev[folderId]
    }));
  };

  const renderNode = (node: FileItem, depth: number = 0) => {
    if (node.type === 'folder') {
      const isOpen = openFolders[node.id] ?? false;
      return (
        <div key={node.id}>
          <div
            onClick={() => toggleFolder(node.id)}
            style={{ paddingLeft: `${depth * 12 + 8}px` }}
            className="flex items-center gap-1.5 py-1 px-2 text-xs text-slate-300 hover:text-slate-100 hover:bg-slate-900/80 cursor-pointer select-none rounded transition-colors group"
          >
            {isOpen ? <ChevronDown className="w-3.5 h-3.5 text-slate-500" /> : <ChevronRight className="w-3.5 h-3.5 text-slate-500" />}
            {isOpen ? <FolderOpen className="w-3.5 h-3.5 text-indigo-400" /> : <Folder className="w-3.5 h-3.5 text-indigo-400/80" />}
            <span className="font-medium group-hover:text-indigo-300">{node.name}</span>
          </div>

          {isOpen && node.children && (
            <div>
              {node.children
                .filter(child => !searchFilter || child.name.toLowerCase().includes(searchFilter.toLowerCase()))
                .map(child => renderNode(child, depth + 1))}
            </div>
          )}
        </div>
      );
    }

    const isSelected = activeFilePath.endsWith(node.name) || activeFilePath === node.path;
    return (
      <div
        key={node.id}
        onClick={() => onSelectFile(node)}
        style={{ paddingLeft: `${depth * 12 + 16}px` }}
        className={`flex items-center justify-between py-1 px-2 text-xs cursor-pointer select-none rounded transition-colors ${
          isSelected 
            ? 'bg-indigo-600/20 text-indigo-300 font-semibold border-l-2 border-indigo-400' 
            : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900/50'
        }`}
      >
        <div className="flex items-center gap-2 truncate">
          {node.language === 'python' ? (
            <span className="text-amber-400 font-bold text-[10px]">PY</span>
          ) : node.language === 'typescript' ? (
            <span className="text-sky-400 font-bold text-[10px]">TS</span>
          ) : (
            <FileCode className="w-3.5 h-3.5 text-slate-400" />
          )}
          <span className="truncate">{node.name}</span>
        </div>
        {node.size && <span className="text-[10px] text-slate-600 font-mono hidden group-hover:inline">{node.size}</span>}
      </div>
    );
  };

  return (
    <div className="w-60 border-r border-slate-800 bg-slate-950 flex flex-col shrink-0 select-none">
      {/* Explorer Header */}
      <div className="h-9 px-3 border-b border-slate-800/80 flex items-center justify-between text-[11px] font-bold uppercase tracking-wider text-slate-400">
        <span>Explorer</span>
        <div className="flex items-center gap-1">
          <button 
            title="New File"
            className="p-1 rounded hover:bg-slate-800 text-slate-400 hover:text-slate-200"
          >
            <Plus className="w-3.5 h-3.5" />
          </button>
          <button 
            title="Refresh"
            className="p-1 rounded hover:bg-slate-800 text-slate-400 hover:text-slate-200"
          >
            <RefreshCw className="w-3 h-3" />
          </button>
        </div>
      </div>

      {/* Search Input Filter */}
      <div className="p-2 border-b border-slate-800/60">
        <div className="flex items-center gap-1.5 px-2 py-1 rounded bg-slate-900 border border-slate-800 text-xs">
          <Search className="w-3 h-3 text-slate-500" />
          <input
            type="text"
            value={searchFilter}
            onChange={(e) => setSearchFilter(e.target.value)}
            placeholder="Search files..."
            className="w-full bg-transparent text-slate-200 placeholder-slate-600 focus:outline-none text-[11px]"
          />
        </div>
      </div>

      {/* Tree view */}
      <div className="flex-1 overflow-y-auto p-1.5 space-y-0.5 font-mono">
        {renderNode(initialProjectTree)}
      </div>

      {/* Bottom Outline / Git Branch Status */}
      <div className="h-7 px-3 border-t border-slate-800/80 bg-slate-950 flex items-center justify-between text-[10px] text-slate-500 font-mono">
        <span>Branch: main*</span>
        <span>UTF-8</span>
      </div>
    </div>
  );
};
