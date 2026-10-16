export type RuntimeMode = 'managed-cloud' | 'oss-runtime' | 'cowork-workspace';

export type TrustBoundary = 'strict' | 'guarded' | 'permissive';

export type ThemeMode = 'dark' | 'frost' | 'aurora' | 'amber' | 'forest';

export type WorkspaceMode = 'chat' | 'work';

export type WorkLayoutPreset = 
  | 'work-default'      // Left: AI & Chat, Right: Editor & Avatar, Bottom: Terminal
  | 'vscode-style'      // Left: Explorer, Center: Editor, Right: AI/Chat, Bottom: Dock
  | 'full-avatar'       // All Panels Closed - Pure Avatar Focus
  | 'left-closed'       // More space for work, clean editor and avatar
  | 'right-closed'      // AI Chat and terminal only
  | 'terminal-closed';  // Clean editor and chat (dock hidden)

export type BottomDockTab = 'terminal' | 'problems' | 'output' | 'debug';

export type DeviceView = 'desktop' | 'tablet' | 'mobile';

export interface ModelAdapterInfo {
  id: string;
  name: string;
  provider: string;
  status: 'active' | 'ready' | 'downloading';
  isNativeLive: boolean;
  latencyMs: number;
  tokensPerSec: number;
  contextWindow: string;
  pricing: string;
  description: string;
  scores: {
    coding: number;
    reasoning: number;
    speed: number;
    cost: number;
  };
}

export interface SubAgent {
  id: string;
  name: string;
  role: string;
  avatar: string;
  status: 'idle' | 'running' | 'waiting_approval' | 'done';
  currentTask: string;
  progress: number;
  skills: string[];
  lastAction: string;
  messagesCount: number;
}

export interface InterAgentMessage {
  id: string;
  from: string;
  to: string;
  timestamp: string;
  content: string;
  taskRef?: string;
}

export interface AuditLog {
  id: string;
  timestamp: string;
  agent: string;
  action: string;
  resource?: string;
  category: 'tool_call' | 'file_write' | 'model_swap' | 'bridge_access' | 'permission_check';
  status: 'allowed' | 'approved' | 'blocked' | 'flagged';
  details: string;
  riskScore?: 'low' | 'medium' | 'high';
  riskLevel?: 'low' | 'medium' | 'high';
}

export type AuditLogEntry = AuditLog;

export interface Skill {
  id: string;
  name: string;
  version: string;
  description: string;
  category: 'System' | 'Development' | 'Web & Scrape' | 'Data Pipeline' | 'Desktop Bridge';
  runsCount: number;
  successRate: number;
  parameters: string[];
  selfImprovedTimes: number;
  codeSnippet: string;
  status: 'active' | 'evaluating' | 'deprecated';
}

export interface Routine {
  id: string;
  name: string;
  cron: string;
  schedule?: string;
  assignedAgent: string;
  model: string;
  lastRun: string;
  nextRun: string;
  status: 'active' | 'paused';
  enabled?: boolean;
  triggerType?: string;
  description: string;
  executionCount: number;
}

export interface DesktopBridgeState {
  connected: boolean;
  agentHost: string;
  bridgeVersion: string;
  approvedPaths: string[];
  activePorts: number[];
  syncStatus: 'synced' | 'syncing' | 'offline';
  lastSync: string;
}

export interface ChatMessage {
  id: string;
  sender: 'user' | 'agent' | 'system';
  agentName?: string;
  agentAvatar?: string;
  timestamp: string;
  text: string;
  modelUsed?: string;
  toolsUsed?: string[];
  isGeminiNative?: boolean;
  codeSnippet?: {
    filename: string;
    language: string;
    code: string;
  };
}

export interface MemoryItem {
  id: string;
  tier: 'global' | 'project' | 'thread' | 'agent';
  title: string;
  content: string;
  tags: string[];
  vectorScore?: number;
  updatedAt: string;
}

export type ComputeFabricDomain = 'binary' | 'ternary' | 'quantum' | 'tri-hybrid';
export type ComputeExecutionMode = 'single' | 'pipelined' | 'parallel';
export type AetheriusBootMode = 'native_bare_metal' | 'hosted_virtual';

export interface AgentNestConfig {
  id: string;
  name: string;
  specialistRole: string;
  status: 'active' | 'paused' | 'state_cached' | 'reverting';
  vCpuCores: number;
  vTernaryUnits: number;
  vQpuQubits: number;
  memoryMb: number;
  microVmType: 'firecracker' | 'gvisor' | 'wasm_sandbox';
  stateSnapshotHash: string;
  lastCheckpoint: string;
}

export interface AetheriusNode {
  id: string;
  name: string;
  hierarchyTier: 'parent' | 'child' | 'grandchild';
  deviceType: 'master_host' | 'cloud_layer' | 'desktop' | 'laptop' | 'phone' | 'home_server' | 'vehicle';
  blockchainId: string;
  status: 'online' | 'syncing' | 'offline';
  latencyMs: number;
  capabilities: string[];
}

