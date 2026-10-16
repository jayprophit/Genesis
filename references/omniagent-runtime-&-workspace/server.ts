import express from 'express';
import path from 'path';
import { fileURLToPath } from 'url';
import { createServer as createViteServer } from 'vite';
import { GoogleGenAI } from '@google/genai';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
const PORT = 3000;

app.use(express.json());

// Lazy-initialize Gemini client
let geminiClient: GoogleGenAI | null = null;
function getGeminiClient(): GoogleGenAI | null {
  const apiKey = process.env.GEMINI_API_KEY;
  if (!apiKey || apiKey === 'MY_GEMINI_API_KEY' || apiKey.trim() === '') {
    return null;
  }
  if (!geminiClient) {
    try {
      geminiClient = new GoogleGenAI({ apiKey });
    } catch (e) {
      console.error('Failed to initialize GoogleGenAI client:', e);
      return null;
    }
  }
  return geminiClient;
}

// Runtime in-memory state
interface SubAgent {
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

interface AuditLog {
  id: string;
  timestamp: string;
  agent: string;
  action: string;
  resource: string;
  category: 'tool_call' | 'file_write' | 'model_swap' | 'bridge_access' | 'permission_check';
  status: 'allowed' | 'approved' | 'blocked' | 'flagged';
  details: string;
  riskScore: 'low' | 'medium' | 'high';
}

interface Skill {
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

interface Routine {
  id: string;
  name: string;
  cron: string;
  assignedAgent: string;
  model: string;
  lastRun: string;
  nextRun: string;
  status: 'active' | 'paused';
  description: string;
  executionCount: number;
}

let activeRuntimeMode: 'managed-cloud' | 'oss-runtime' | 'cowork-workspace' = 'managed-cloud';
let selectedModelId = 'gemini-3.8-flash';
let trustBoundary: 'strict' | 'guarded' | 'permissive' = 'guarded';

let desktopBridgeState = {
  connected: true,
  agentHost: 'Darwin arm64 (macOS Sonoma 15.2)',
  bridgeVersion: 'v2.8.4',
  approvedPaths: ['/Users/workspace/projects/fintech-dashboard', '/var/data/models'],
  activePorts: [3000, 5432, 8080],
  syncStatus: 'synced' as 'synced' | 'syncing' | 'offline',
  lastSync: new Date().toISOString(),
};

let subAgents: SubAgent[] = [
  {
    id: 'atlas',
    name: 'Atlas',
    role: 'Project Orchestrator',
    avatar: 'https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=150&auto=format&fit=crop&q=80',
    status: 'running',
    currentTask: 'Supervising parallel sub-agent execution & task decomposition',
    progress: 78,
    skills: ['routine-scheduler', 'trust-boundary-enforcer'],
    lastAction: 'Delegated code refactoring to Dev Agent',
    messagesCount: 42,
  },
  {
    id: 'dev',
    name: 'Dev Engineering',
    role: 'Code & Runtime Specialist',
    avatar: 'https://images.unsplash.com/photo-1507003211169-0a1dd7228f2d?w=150&auto=format&fit=crop&q=80',
    status: 'running',
    currentTask: 'Optimizing Model Adapter zero-downtime hot-swap pipeline',
    progress: 92,
    skills: ['docker-compose-generator', 'git-repo-analyzer'],
    lastAction: 'Compiled TypeScript AST & updated schema bindings',
    messagesCount: 88,
  },
  {
    id: 'luna',
    name: 'Luna',
    role: 'Design & Visual Systems',
    avatar: 'https://images.unsplash.com/photo-1517841905240-472988babdf9?w=150&auto=format&fit=crop&q=80',
    status: 'idle',
    currentTask: 'Polishing responsive canvas and high-contrast dark theme',
    progress: 100,
    skills: ['design-token-parser', 'layout-optimizer'],
    lastAction: 'Generated SVG icons for desktop bridge node',
    messagesCount: 35,
  },
  {
    id: 'orion',
    name: 'Orion',
    role: 'Data Pipelines & Memory',
    avatar: 'https://images.unsplash.com/photo-1500648767791-00dcc994a43e?w=150&auto=format&fit=crop&q=80',
    status: 'running',
    currentTask: 'Vectorizing local embeddings & sync to 4-tier memory',
    progress: 64,
    skills: ['sql-schema-migrator', 'vector-indexer'],
    lastAction: 'Synced 1,420 vectors across global memory tier',
    messagesCount: 51,
  },
  {
    id: 'nova',
    name: 'Nova',
    role: 'QA & Verification',
    avatar: 'https://images.unsplash.com/photo-1544005313-94ddf0286df2?w=150&auto=format&fit=crop&q=80',
    status: 'waiting_approval',
    currentTask: 'Awaiting permission to run integration tests against VPS sandbox',
    progress: 45,
    skills: ['api-test-suite', 'security-auditor'],
    lastAction: 'Flagged 1 external socket connection for review',
    messagesCount: 63,
  },
  {
    id: 'sage',
    name: 'Sage',
    role: 'Strategy & Architecture',
    avatar: 'https://images.unsplash.com/photo-1573496359142-b8d87734a5a2?w=150&auto=format&fit=crop&q=80',
    status: 'idle',
    currentTask: 'Reviewing cross-platform runtime latency benchmarks',
    progress: 100,
    skills: ['architecture-evaluator', 'cost-optimizer'],
    lastAction: 'Evaluated Gemini 3.8 Flash vs Claude 3.5 Sonnet latency delta',
    messagesCount: 29,
  }
];

let skillsList: Skill[] = [
  {
    id: 'skill-1',
    name: 'web-crawler-pagination',
    version: '2.4.1',
    description: 'Autonomous headless browser with smart pagination and dom selector repair',
    category: 'Web & Scrape',
    runsCount: 342,
    successRate: 98.4,
    parameters: ['url', 'maxPages', 'extractSchema', 'stealthMode'],
    selfImprovedTimes: 5,
    codeSnippet: 'async function crawlWithPagination(browser, { url, maxPages, extractSchema }) { ... }',
    status: 'active'
  },
  {
    id: 'skill-2',
    name: 'desktop-bridge-sync',
    version: '1.9.0',
    description: 'Secured RPC bridge to local filesystem and development terminal with HMAC auth',
    category: 'Desktop Bridge',
    runsCount: 1208,
    successRate: 99.8,
    parameters: ['localPath', 'direction', 'diffOnly', 'approvedHashes'],
    selfImprovedTimes: 3,
    codeSnippet: 'export const syncBridge = async (rpcClient, mountPoint) => { ... }',
    status: 'active'
  },
  {
    id: 'skill-3',
    name: 'model-adapter-fallback',
    version: '3.1.0',
    description: 'Zero-latency dynamic provider failover with streaming token buffer preservation',
    category: 'System',
    runsCount: 894,
    successRate: 100.0,
    parameters: ['primaryModel', 'fallbackModel', 'retryAttempts', 'temperature'],
    selfImprovedTimes: 8,
    codeSnippet: 'class ModelAdapter { async dispatch(prompt, options) { ... } }',
    status: 'active'
  },
  {
    id: 'skill-4',
    name: 'vps-docker-deployer',
    version: '1.4.2',
    description: 'Self-healing container orchestrator for VPS, Hetzner, AWS, or serverless workers',
    category: 'Development',
    runsCount: 182,
    successRate: 96.5,
    parameters: ['dockerfile', 'composeConfig', 'envSecrets', 'healthEndpoint'],
    selfImprovedTimes: 2,
    codeSnippet: 'async function deployContainer(vpsTarget, spec) { ... }',
    status: 'active'
  },
  {
    id: 'skill-5',
    name: 'audit-log-validator',
    version: '2.0.0',
    description: 'Tamper-proof append-only merkle tree audit logger for agent action compliance',
    category: 'System',
    runsCount: 2490,
    successRate: 100.0,
    parameters: ['actor', 'actionHash', 'signature', 'trustBoundary'],
    selfImprovedTimes: 4,
    codeSnippet: 'function commitAuditBlock(logEntry) { return crypto.createHmac(...); }',
    status: 'active'
  }
];

let routinesList: Routine[] = [
  {
    id: 'rt-1',
    name: 'Hourly Local Repo Sync & Vulnerability Sweep',
    cron: '0 * * * *',
    assignedAgent: 'Dev Engineering',
    model: 'gemini-3.8-flash',
    lastRun: '12 mins ago',
    nextRun: 'in 48 mins',
    status: 'active',
    description: 'Pull git diff via desktop bridge, check npm audit, auto-create fix branch',
    executionCount: 724
  },
  {
    id: 'rt-2',
    name: 'Nightly Vector Memory Consolidation',
    cron: '0 2 * * *',
    assignedAgent: 'Orion Data',
    model: 'gemini-3.8-flash',
    lastRun: 'Yesterday 02:00',
    nextRun: 'Today 02:00',
    status: 'active',
    description: 'Compress temporary chat memories into persistent global vector index',
    executionCount: 118
  },
  {
    id: 'rt-3',
    name: 'Competitive Intelligence & Web Digest',
    cron: '0 8 * * 1-5',
    assignedAgent: 'Sage Strategy',
    model: 'claude-3-5-sonnet',
    lastRun: 'Today 08:00',
    nextRun: 'Tomorrow 08:00',
    status: 'active',
    description: 'Crawl target domains with pagination, extract key AI runtime advances',
    executionCount: 89
  },
  {
    id: 'rt-4',
    name: 'Sub-agent Health & Container Keep-Alive',
    cron: '*/5 * * * *',
    assignedAgent: 'Atlas',
    model: 'gemini-3.8-flash',
    lastRun: '2 mins ago',
    nextRun: 'in 3 mins',
    status: 'active',
    description: 'Ping parallel workers, check memory usage, restart sluggish containers',
    executionCount: 4320
  }
];

let auditLogs: AuditLog[] = [
  {
    id: 'log-101',
    timestamp: new Date(Date.now() - 1000 * 60 * 2).toLocaleTimeString(),
    agent: 'Atlas (Orchestrator)',
    action: 'Dispatched Sub-agent Task',
    resource: 'subagent://dev/refactor-pipeline',
    category: 'tool_call',
    status: 'allowed',
    details: 'Auto-routed TypeScript AST task to Dev Engineering based on model adapter availability',
    riskScore: 'low'
  },
  {
    id: 'log-102',
    timestamp: new Date(Date.now() - 1000 * 60 * 5).toLocaleTimeString(),
    agent: 'Dev Engineering',
    action: 'Desktop Bridge Read',
    resource: 'file:///Users/workspace/projects/fintech-dashboard/package.json',
    category: 'bridge_access',
    status: 'allowed',
    details: 'Verified SHA-256 fingerprint against approved bridge whitelist',
    riskScore: 'low'
  },
  {
    id: 'log-103',
    timestamp: new Date(Date.now() - 1000 * 60 * 9).toLocaleTimeString(),
    agent: 'Nova (QA)',
    action: 'External Socket Connection Request',
    resource: 'tcp://192.168.1.100:9222 (Remote Chrome DevTools)',
    category: 'permission_check',
    status: 'flagged',
    details: 'Sub-agent requested unmapped debugging port. Paused for user approval in Guarded boundary mode',
    riskScore: 'high'
  },
  {
    id: 'log-104',
    timestamp: new Date(Date.now() - 1000 * 60 * 14).toLocaleTimeString(),
    agent: 'System Router',
    action: 'Model Engine Swap',
    resource: 'adapter://claude-3-5-sonnet -> adapter://gemini-3.8-flash',
    category: 'model_swap',
    status: 'allowed',
    details: 'Zero-downtime hot-swap triggered. Latency dropped from 840ms to 280ms',
    riskScore: 'low'
  },
  {
    id: 'log-105',
    timestamp: new Date(Date.now() - 1000 * 60 * 20).toLocaleTimeString(),
    agent: 'Orion Data',
    action: 'Memory Vector Write',
    resource: 'memory://global/user-preferences',
    category: 'file_write',
    status: 'allowed',
    details: 'Saved user architecture decision: "Prefer self-hosted VPS with Docker runtime"',
    riskScore: 'low'
  }
];

// Available models in model adapter
const modelAdapters = [
  {
    id: 'gemini-3.8-flash',
    name: 'Gemini 3.8 Flash',
    provider: 'Google DeepMind',
    status: 'active',
    isNativeLive: true,
    latencyMs: 240,
    tokensPerSec: 142,
    contextWindow: '1M tokens',
    pricing: '$0.10 / 1M input',
    description: 'Ultra-low latency, multimodal, native tool calling & live streaming',
    scores: { coding: 94, reasoning: 92, speed: 99, cost: 98 }
  },
  {
    id: 'gemini-3.1-pro-preview',
    name: 'Gemini 3.1 Pro',
    provider: 'Google DeepMind',
    status: 'ready',
    isNativeLive: true,
    latencyMs: 580,
    tokensPerSec: 86,
    contextWindow: '2M tokens',
    pricing: '$1.25 / 1M input',
    description: 'Deep analytical reasoning, complex code generation, mathematical STEM',
    scores: { coding: 98, reasoning: 99, speed: 82, cost: 86 }
  },
  {
    id: 'claude-3-5-sonnet',
    name: 'Claude 3.5 Sonnet',
    provider: 'Anthropic',
    status: 'ready',
    isNativeLive: false,
    latencyMs: 620,
    tokensPerSec: 78,
    contextWindow: '200K tokens',
    pricing: '$3.00 / 1M input',
    description: 'High nuanced tone, artifact generation, architecture reasoning',
    scores: { coding: 97, reasoning: 96, speed: 76, cost: 74 }
  },
  {
    id: 'gpt-4o',
    name: 'GPT-4o (Codex Engine)',
    provider: 'OpenAI',
    status: 'ready',
    isNativeLive: false,
    latencyMs: 490,
    tokensPerSec: 92,
    contextWindow: '128K tokens',
    pricing: '$2.50 / 1M input',
    description: 'Omni intelligence, robust instruction following, functional schema tools',
    scores: { coding: 95, reasoning: 94, speed: 88, cost: 78 }
  },
  {
    id: 'deepseek-r1',
    name: 'DeepSeek R1',
    provider: 'DeepSeek AI',
    status: 'ready',
    isNativeLive: false,
    latencyMs: 780,
    tokensPerSec: 64,
    contextWindow: '128K tokens',
    pricing: '$0.55 / 1M input',
    description: 'Reinforcement learning reasoning model, deep step-by-step thinking traces',
    scores: { coding: 96, reasoning: 98, speed: 65, cost: 95 }
  },
  {
    id: 'llama-3.3-70b-local',
    name: 'Llama 3.3 70B (Local vLLM / Ollama)',
    provider: 'Local VPS / Self-Hosted',
    status: 'ready',
    isNativeLive: false,
    latencyMs: 310,
    tokensPerSec: 110,
    contextWindow: '128K tokens',
    pricing: '$0.00 (Self-Hosted DIY)',
    description: '100% private, on-premise execution, zero data egress, offline capability',
    scores: { coding: 91, reasoning: 90, speed: 94, cost: 100 }
  }
];

// API Endpoints
app.get('/api/health', (req, res) => {
  const hasGeminiKey = Boolean(process.env.GEMINI_API_KEY && process.env.GEMINI_API_KEY !== 'MY_GEMINI_API_KEY');
  res.json({
    status: 'operational',
    version: '3.4.0-omni',
    mode: activeRuntimeMode,
    activeModel: selectedModelId,
    trustBoundary,
    hasGeminiKey,
    uptimeSeconds: Math.floor(process.uptime()),
    subAgentsActive: subAgents.filter(a => a.status === 'running').length,
    desktopBridge: desktopBridgeState,
    memoryTiers: {
      globalEntries: 84,
      projectFilesSynced: 142,
      activeThreads: 6,
      subAgentSharedVectors: 1420
    }
  });
});

app.get('/api/runtime/overview', (req, res) => {
  res.json({
    mode: activeRuntimeMode,
    activeModelId: selectedModelId,
    trustBoundary,
    models: modelAdapters,
    subAgents,
    skills: skillsList,
    routines: routinesList,
    auditLogs: auditLogs.slice(0, 20),
    desktopBridge: desktopBridgeState
  });
});

app.post('/api/runtime/set-mode', (req, res) => {
  const { mode } = req.body;
  if (['managed-cloud', 'oss-runtime', 'cowork-workspace'].includes(mode)) {
    activeRuntimeMode = mode;
    auditLogs.unshift({
      id: `log-${Date.now()}`,
      timestamp: new Date().toLocaleTimeString(),
      agent: 'User (Admin)',
      action: 'Switched Runtime Mode',
      resource: `runtime://${mode}`,
      category: 'model_swap',
      status: 'allowed',
      details: `Runtime environment shifted to ${mode.toUpperCase()}. Reconfigured trust boundaries and container isolates.`,
      riskScore: 'low'
    });
    return res.json({ success: true, mode: activeRuntimeMode });
  }
  res.status(400).json({ error: 'Invalid mode' });
});

app.post('/api/runtime/set-model', (req, res) => {
  const { modelId } = req.body;
  const match = modelAdapters.find(m => m.id === modelId);
  if (!match) {
    return res.status(400).json({ error: 'Model not found' });
  }
  const oldModel = selectedModelId;
  selectedModelId = modelId;

  auditLogs.unshift({
    id: `log-${Date.now()}`,
    timestamp: new Date().toLocaleTimeString(),
    agent: 'Model Adapter Router',
    action: 'Swapped Engine Zero-Downtime',
    resource: `model://${oldModel} -> model://${modelId}`,
    category: 'model_swap',
    status: 'allowed',
    details: `Engine switched to ${match.name} (${match.provider}). Context size: ${match.contextWindow}. No codebase changes required.`,
    riskScore: 'low'
  });

  res.json({ success: true, activeModel: match });
});

app.post('/api/runtime/set-boundary', (req, res) => {
  const { boundary } = req.body;
  if (['strict', 'guarded', 'permissive'].includes(boundary)) {
    trustBoundary = boundary;
    auditLogs.unshift({
      id: `log-${Date.now()}`,
      timestamp: new Date().toLocaleTimeString(),
      agent: 'Security Gateway',
      action: 'Updated Trust Boundary',
      resource: `boundary://${boundary}`,
      category: 'permission_check',
      status: 'allowed',
      details: `Trust boundary changed to ${boundary.toUpperCase()}. ${
        boundary === 'strict'
          ? 'All file writes and socket connections require explicit confirmation.'
          : boundary === 'guarded'
          ? 'Approved paths auto-execute; external sockets and raw bash prompt for review.'
          : 'High autonomy enabled for automated sub-agent pipelines.'
      }`,
      riskScore: boundary === 'permissive' ? 'medium' : 'low'
    });
    return res.json({ success: true, trustBoundary });
  }
  res.status(400).json({ error: 'Invalid boundary' });
});

app.post('/api/bridge/toggle', (req, res) => {
  desktopBridgeState.connected = !desktopBridgeState.connected;
  desktopBridgeState.lastSync = new Date().toISOString();
  auditLogs.unshift({
    id: `log-${Date.now()}`,
    timestamp: new Date().toLocaleTimeString(),
    agent: 'Desktop Bridge Manager',
    action: desktopBridgeState.connected ? 'Bridge Connected' : 'Bridge Severed',
    resource: 'bridge://localhost:rpc',
    category: 'bridge_access',
    status: desktopBridgeState.connected ? 'allowed' : 'blocked',
    details: desktopBridgeState.connected
      ? 'Desktop bridge established connection with host macOS daemon. Mounted 2 approved project trees.'
      : 'Desktop bridge paused. Local file systems detached from cloud agents.',
    riskScore: 'low'
  });
  res.json({ success: true, desktopBridge: desktopBridgeState });
});

app.post('/api/subagents/action', (req, res) => {
  const { agentId, action } = req.body;
  const agent = subAgents.find(a => a.id === agentId);
  if (!agent) {
    return res.status(404).json({ error: 'Sub-agent not found' });
  }
  if (action === 'pause') {
    agent.status = 'idle';
  } else if (action === 'resume') {
    agent.status = 'running';
  } else if (action === 'approve') {
    agent.status = 'running';
    agent.lastAction = 'User approved pending task execution';
  }
  res.json({ success: true, agent });
});

// Self-improving skill loop
app.post('/api/skills/improve', (req, res) => {
  const { skillId } = req.body;
  const skill = skillsList.find(s => s.id === skillId);
  if (!skill) {
    return res.status(404).json({ error: 'Skill not found' });
  }
  skill.selfImprovedTimes += 1;
  skill.successRate = Math.min(100, Number((skill.successRate + 0.3).toFixed(1)));
  const versionParts = skill.version.split('.').map(Number);
  versionParts[2] += 1;
  skill.version = versionParts.join('.');

  auditLogs.unshift({
    id: `log-${Date.now()}`,
    timestamp: new Date().toLocaleTimeString(),
    agent: 'Hermes Self-Improvement Loop',
    action: 'Refactored & Improved Skill',
    resource: `skill://${skill.name}@v${skill.version}`,
    category: 'tool_call',
    status: 'allowed',
    details: `Executed autonomous AST optimization on ${skill.name}. Success rate improved to ${skill.successRate}%.`,
    riskScore: 'low'
  });

  res.json({ success: true, skill });
});

// Main chat/agent execution endpoint with real Gemini integration
app.post('/api/agent/dispatch', async (req, res) => {
  const { prompt, targetAgentId, activeTab, systemContext } = req.body;
  if (!prompt || typeof prompt !== 'string') {
    return res.status(400).json({ error: 'Prompt is required' });
  }

  const agent = subAgents.find(a => a.id === targetAgentId) || subAgents[0];
  const startTime = Date.now();

  let responseText = '';
  let usedModel = selectedModelId;
  let isRealGemini = false;
  let toolCallsExecuted: string[] = [];

  const gemini = getGeminiClient();

  if (gemini && (selectedModelId.startsWith('gemini') || !selectedModelId)) {
    try {
      isRealGemini = true;
      const modelToUse = selectedModelId.includes('pro') ? 'gemini-3.1-pro-preview' : 'gemini-3.8-flash';
      usedModel = modelToUse;

      const systemInstruction = `You are ${agent.name}, ${agent.role} within the OmniAgent Runtime and Managed Cloud Assistant Workspace.
Current Runtime Mode: ${activeRuntimeMode} (Managed Cloud VM, Open-Source Hermes Runtime, or Claude Cowork).
Trust Boundary: ${trustBoundary}.
Active Sub-Agents: ${subAgents.map(a => `${a.name} (${a.role})`).join(', ')}.
Available tools: Model Adapter (engine swap), Desktop Bridge (reach local files and terminal), Headless Browser Crawler, Vector Memory Tier, Reusable Skills, Scheduled Routines.
Respond with high technical precision, structured thinking, clear code/actionable outputs, and realistic agent step logs. Keep formatting clean and markdown-styled.`;

      const result = await gemini.models.generateContent({
        model: modelToUse,
        contents: prompt,
        config: {
          systemInstruction,
          temperature: 0.7,
        }
      });

      responseText = result.text || 'Task completed successfully.';
      toolCallsExecuted = ['desktop_bridge.inspect_manifest', 'model_adapter.verify_latency', 'audit_logger.sign_entry'];
    } catch (err: any) {
      console.warn('Gemini API call failed, falling back to simulated engine:', err?.message);
      isRealGemini = false;
    }
  }

  // Fallback response generator if Gemini key not set or alternate model simulated
  if (!responseText) {
    const elapsed = Date.now() - startTime;
    const modelObj = modelAdapters.find(m => m.id === selectedModelId) || modelAdapters[0];
    
    // Craft intelligent specialized response
    if (prompt.toLowerCase().includes('model') || prompt.toLowerCase().includes('swap') || prompt.toLowerCase().includes('adapter')) {
      responseText = `### 🔄 Model Adapter Engine Dispatch
**Swapping Execution Engine to ${modelObj.name}** without code alterations.

- **Provider**: ${modelObj.provider}
- **Context Window**: ${modelObj.contextWindow}
- **Latency Benchmark**: ${modelObj.latencyMs}ms | **Speed**: ${modelObj.tokensPerSec} tokens/sec
- **Trust Boundary Verified**: \`${trustBoundary.toUpperCase()}\`

\`\`\`typescript
// Dynamic Model Adapter Dispatch Interface
import { ModelAdapter } from '@/runtime/adapter';

export const agentPipeline = new ModelAdapter({
  provider: '${modelObj.id}',
  fallback: 'gemini-3.8-flash',
  trustBoundary: '${trustBoundary}',
  bridgeAuthorization: true
});

const result = await agentPipeline.execute({
  task: ${JSON.stringify(prompt.slice(0, 80))},
  subAgents: ['${agent.id}', 'dev', 'nova']
});
\`\`\`
All sub-agents have updated their telemetry bindings to reflect **${modelObj.name}**.`;
      toolCallsExecuted = ['adapter.hot_swap', 'bridge.verify_tokens', 'audit.log_swap'];
    } else if (prompt.toLowerCase().includes('bridge') || prompt.toLowerCase().includes('file') || prompt.toLowerCase().includes('local')) {
      responseText = `### 🌉 Desktop Bridge Action: Local Resource Sync
Connected to local host **${desktopBridgeState.agentHost}** via **HMAC SHA-256 RPC**.

**Mounted Directories:**
- \`/Users/workspace/projects/fintech-dashboard\` (142 files tracked, 0 conflicts)
- \`/var/data/models\` (Shared local weights & embeddings)

**Actions Executed:**
1. Ran fast diff across project tree.
2. Synchronized 3 changed TypeScript files into the Cloud Sandbox.
3. Local port forwarder active on \`localhost:3000\` and \`localhost:5432\`.
4. Stored audit proof into the Merkle log.`;
      toolCallsExecuted = ['desktop_bridge.diff_tree', 'desktop_bridge.stream_patch', 'audit_logger.commit'];
    } else if (prompt.toLowerCase().includes('subagent') || prompt.toLowerCase().includes('parallel') || prompt.toLowerCase().includes('team')) {
      responseText = `### 🤖 Sub-Agent Swarm Orchestration
**Atlas** has decomposed the goal into parallel worker pipelines:

1. **Dev Engineering (Code Specialist)**:
   - Branch: \`feature/agent-runtime-adapter\`
   - Running compilation passes & automated unit tests.
2. **Nova (QA & Verification)**:
   - Running security sandbox audit on external RPC connectors.
3. **Orion (Memory & Vectors)**:
   - Indexing session facts into **Tier 2 (Project Memory)** and **Tier 4 (Agent Memory)**.
4. **Luna (UI Systems)**:
   - Updating responsive workspace viewport and active telemetry dials.`;
      toolCallsExecuted = ['swarm.decompose', 'subagent.dispatch_parallel', 'memory.index_embeddings'];
    } else {
      responseText = `### ✅ Task Executed by ${agent.name} (${agent.role})
**Runtime Mode:** ${activeRuntimeMode} | **Engine:** ${modelObj.name}

I have processed your request:
> "${prompt}"

**Execution Highlights:**
- **Step 1:** Ingested query into **Tier 3 (Thread Memory)** with parent trace \`#TR-${Math.floor(Math.random() * 89999 + 10000)}\`.
- **Step 2:** Verified permissions against **${trustBoundary.toUpperCase()}** trust boundary. Zero security violations detected.
- **Step 3:** Coordinated with **Dev Engineering** to stage local code patches via the **Desktop Bridge**.
- **Step 4:** Logged cryptographic proof into the audit journal.

You can inspect the live work surface on the right for terminal output, files, and live browser automation.`;
      toolCallsExecuted = ['memory.retrieve_context', 'runtime.evaluate_boundary', 'agent.synthesize'];
    }
  }

  // Update agent status & messages
  agent.messagesCount += 1;
  agent.lastAction = `Processed request: "${prompt.slice(0, 30)}..."`;

  // Append new audit log
  const newLog: AuditLog = {
    id: `log-${Date.now()}`,
    timestamp: new Date().toLocaleTimeString(),
    agent: agent.name,
    action: `Prompt Executed [${usedModel}]`,
    resource: `runtime://${agent.id}`,
    category: 'tool_call',
    status: 'allowed',
    details: `Generated response (${responseText.length} chars) using ${usedModel}. Tools: ${toolCallsExecuted.join(', ')}`,
    riskScore: 'low'
  };
  auditLogs.unshift(newLog);

  res.json({
    reply: responseText,
    agent: agent.name,
    agentRole: agent.role,
    model: usedModel,
    isRealGemini,
    latencyMs: Date.now() - startTime,
    toolCalls: toolCallsExecuted,
    auditLogId: newLog.id
  });
});

async function startServer() {
  // Vite middleware in dev
  if (process.env.NODE_ENV !== 'production') {
    const vite = await createViteServer({
      server: { middlewareMode: true },
      appType: 'spa',
    });
    app.use(vite.middlewares);
  } else {
    const distPath = path.join(process.cwd(), 'dist');
    app.use(express.static(distPath));
    app.get('*', (req, res) => {
      res.sendFile(path.join(distPath, 'index.html'));
    });
  }

  app.listen(PORT, '0.0.0.0', () => {
    console.log(`[OmniAgent] Server running on http://0.0.0.0:${PORT}`);
  });
}

startServer();
