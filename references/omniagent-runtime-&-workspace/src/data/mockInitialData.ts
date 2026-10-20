import { SubAgent, ModelAdapterInfo, Skill, Routine, AuditLog, MemoryItem, InterAgentMessage, DesktopBridgeState } from '../types';

export const initialModels: ModelAdapterInfo[] = [
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
    description: 'Nuanced technical drafting, architectural reasoning, artifact workflows',
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
    description: 'Omni intelligence, strong structured tool invocation, legacy parity',
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
    description: 'RL reasoning chain, self-verification traces, competitive code synthesis',
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

export const initialSubAgents: SubAgent[] = [
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
    messagesCount: 42
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
    messagesCount: 88
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
    messagesCount: 35
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
    messagesCount: 51
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
    messagesCount: 63
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
    messagesCount: 29
  }
];

export const initialInterAgentMessages: InterAgentMessage[] = [
  {
    id: 'iam-1',
    from: 'Atlas',
    to: 'Dev Engineering',
    timestamp: '10:14 AM',
    content: 'Found a new dataset schema change in the local repository. Can you update the adapter types?',
    taskRef: 'TS-AST-412'
  },
  {
    id: 'iam-2',
    from: 'Dev Engineering',
    to: 'Atlas',
    timestamp: '10:15 AM',
    content: 'On it! Synchronizing types over desktop bridge now. Will notify Nova once compilation passes.',
    taskRef: 'TS-AST-412'
  },
  {
    id: 'iam-3',
    from: 'Luna',
    to: 'Nova',
    timestamp: '10:16 AM',
    content: 'Here are the updated layout tokens for the responsive inspector panel. Ready for QA check.',
    taskRef: 'UI-TOKENS-88'
  },
  {
    id: 'iam-4',
    from: 'Nova',
    to: 'Luna',
    timestamp: '10:17 AM',
    content: 'Contrast ratios checked: 7.2:1 passes WCAG AAA. Visual regression suite running on Chrome sandbox.',
    taskRef: 'UI-TOKENS-88'
  },
  {
    id: 'iam-5',
    from: 'Orion',
    to: 'Sage',
    timestamp: '10:19 AM',
    content: 'Ingested 48 research PDFs from the data pipeline into Tier 2 (Project Memory). Cosine similarity index updated.',
    taskRef: 'DATA-VEC-901'
  }
];

export const initialSkills: Skill[] = [
  {
    id: 'skill-1',
    name: 'web-crawler-pagination',
    version: '2.4.1',
    description: 'Autonomous headless browser with smart pagination, anti-bot bypass, and DOM selector repair',
    category: 'Web & Scrape',
    runsCount: 342,
    successRate: 98.4,
    parameters: ['url', 'maxPages', 'extractSchema', 'stealthMode'],
    selfImprovedTimes: 5,
    codeSnippet: `// Autonomous crawler with pagination recovery
export async function crawlWithPagination(browser, config) {
  let page = await browser.newPage();
  let results = [];
  for (let idx = 1; idx <= config.maxPages; idx++) {
    const data = await page.evaluate(extractPageMetrics);
    results.push(...data);
    const nextBtn = await findNextPaginationTarget(page);
    if (!nextBtn) break;
    await nextBtn.click();
    await page.waitForNetworkIdle();
  }
  return results;
}`,
    status: 'active'
  },
  {
    id: 'skill-2',
    name: 'desktop-bridge-sync',
    version: '1.9.0',
    description: 'Secured RPC bridge to local filesystem and development terminal with HMAC SHA-256 auth',
    category: 'Desktop Bridge',
    runsCount: 1208,
    successRate: 99.8,
    parameters: ['localPath', 'direction', 'diffOnly', 'approvedHashes'],
    selfImprovedTimes: 3,
    codeSnippet: `export const syncBridge = async (rpcClient, mountPoint) => {
  const localManifest = await rpcClient.call('scan_tree', { mountPoint });
  const remoteManifest = await getCloudSandboxManifest();
  const patches = computeDiff(localManifest, remoteManifest);
  return rpcClient.streamPatches(patches);
};`,
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
    codeSnippet: `export class ModelAdapter {
  async dispatch(prompt, opts) {
    try {
      return await this.activeProvider.stream(prompt, opts);
    } catch (err) {
      this.logger.warn('Failing over to fallback engine...');
      return await this.fallbackProvider.stream(prompt, opts);
    }
  }
}`,
    status: 'active'
  },
  {
    id: 'skill-4',
    name: 'vps-docker-deployer',
    version: '1.4.2',
    description: 'Self-healing container orchestrator for VPS, Docker Compose, or serverless workers',
    category: 'Development',
    runsCount: 182,
    successRate: 96.5,
    parameters: ['dockerfile', 'composeConfig', 'envSecrets', 'healthEndpoint'],
    selfImprovedTimes: 2,
    codeSnippet: `export async function deployContainer(vpsTarget, spec) {
  const compose = generateComposeYaml(spec);
  await vpsTarget.ssh.exec(\`docker compose -f \${spec.file} up -d --remove-orphans\`);
  return await verifyHealthCheck(spec.healthEndpoint);
}`,
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
    codeSnippet: `export function commitAuditBlock(logEntry) {
  const prevHash = this.chain.last().hash;
  const hash = crypto.createHmac('sha256', this.signingKey)
    .update(prevHash + JSON.stringify(logEntry)).digest('hex');
  return this.chain.append({ ...logEntry, hash });
}`,
    status: 'active'
  }
];

export const initialRoutines: Routine[] = [
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

export const initialAuditLogs: AuditLog[] = [
  {
    id: 'log-101',
    timestamp: '10:24:12 AM',
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
    timestamp: '10:21:40 AM',
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
    timestamp: '10:18:05 AM',
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
    timestamp: '10:14:50 AM',
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
    timestamp: '10:09:22 AM',
    agent: 'Orion Data',
    action: 'Memory Vector Write',
    resource: 'memory://global/user-preferences',
    category: 'file_write',
    status: 'allowed',
    details: 'Saved user architecture decision: "Prefer self-hosted VPS with Docker runtime"',
    riskScore: 'low'
  }
];

export const initialMemoryItems: MemoryItem[] = [
  {
    id: 'mem-1',
    tier: 'global',
    title: 'Core Architecture Preference',
    content: 'User requires strict model freedom: switch model providers (Gemini, Claude, Codex, Local Llama) without code changes. Deployment runs on VPS Docker or serverless.',
    tags: ['preferences', 'architecture', 'runtime'],
    vectorScore: 0.98,
    updatedAt: 'Today 09:30 AM'
  },
  {
    id: 'mem-2',
    tier: 'project',
    title: 'Active Project: fintech-dashboard',
    content: 'Repository mounted via Desktop Bridge at /Users/workspace/projects/fintech-dashboard. Stack: React 19, TypeScript, Vite, Tailwind CSS, Express backend.',
    tags: ['project-manifest', 'desktop-bridge', 'stack'],
    vectorScore: 0.94,
    updatedAt: 'Today 10:00 AM'
  },
  {
    id: 'mem-3',
    tier: 'thread',
    title: 'Sub-agent Parallel Task Context',
    content: 'Atlas delegating TypeScript AST refactor to Dev Engineering while Nova runs QA tests in isolated temporary cloud sandbox.',
    tags: ['active-context', 'subagents', 'thread'],
    vectorScore: 0.89,
    updatedAt: 'Just now'
  },
  {
    id: 'mem-4',
    tier: 'agent',
    title: 'Dev Engineering Code Knowledge',
    content: 'Compiled AST signatures for ModelAdapter interface. Knows port forward bindings on 3000, 5432, and 8080.',
    tags: ['dev-agent', 'ast-cache', 'ports'],
    vectorScore: 0.91,
    updatedAt: '12 mins ago'
  }
];

export const sampleWorkspaceFiles = [
  {
    name: 'main.py',
    path: '/src/main.py',
    size: '1.2 KB',
    content: `"""
Nexus AI / Nova Autonomous Runtime Agent
Entry point daemon for local & cloud execution
"""
import os
import sys
import asyncio
from nexus.runtime import OmniAgentRuntime
from nexus.adapters import GeminiAdapter, ClaudeAdapter

async def main():
    print("[Nova Agent] Initializing neural orchestrator...")
    runtime = OmniAgentRuntime(
        mode=os.getenv("RUNTIME_MODE", "managed-cloud"),
        trust_boundary="guarded"
    )
    
    # Register hot-swappable model adapters
    runtime.register_adapter("gemini-3.8-flash", GeminiAdapter(live=True))
    runtime.register_adapter("claude-3-5-sonnet", ClaudeAdapter())
    
    # Establish desktop bridge socket connection
    await runtime.connect_desktop_bridge(port=9876)
    print("[Nova Agent] Daemon online. Awaiting tasks from chat stream.")
    await runtime.serve_forever()

if __name__ == "__main__":
    asyncio.run(main())
`
  },
  {
    name: 'utils.py',
    path: '/src/utils.py',
    size: '2.8 KB',
    content: `"""
Cryptographic verification and sandbox tokenization utilities
"""
import hmac
import hashlib
import time

def verify_bridge_signature(payload: bytes, signature: str, secret_key: str) -> bool:
    """Verifies HMAC SHA-256 signature for desktop bridge RPC messages"""
    computed = hmac.new(secret_key.encode(), payload, hashlib.sha256).hexdigest()
    return hmac.compare_digest(computed, signature)

def compute_ast_checksum(code_str: str) -> str:
    """Calculates SHA-256 hash for code verification in audit trail"""
    return hashlib.sha256(code_str.encode('utf-8')).hexdigest()[:16]
`
  },
  {
    name: 'runtime.config.ts',
    path: '/src/runtime/runtime.config.ts',
    size: '2.4 KB',
    content: `// OmniAgent Runtime Configuration
export const runtimeConfig = {
  mode: 'managed-cloud', // 'managed-cloud' | 'oss-runtime' | 'cowork-workspace'
  modelAdapter: {
    primary: 'gemini-3.8-flash',
    fallback: 'claude-3-5-sonnet',
    localOllamaEndpoint: 'http://localhost:11434',
    hotSwapEnabled: true,
  },
  trustBoundary: {
    level: 'guarded',
    requireApprovalFor: ['raw_bash', 'unmapped_socket', 'file_delete'],
  },
  desktopBridge: {
    port: 9876,
    hmacValidation: true,
    syncIntervalMs: 5000,
  }
};`
  },
  {
    name: 'docker-compose.yml',
    path: '/docker-compose.yml',
    size: '1.8 KB',
    content: `version: '3.8'
services:
  omniagent-runtime:
    image: ghcr.io/omniagent/runtime:latest
    container_name: omniagent-core
    restart: unless-stopped
    ports:
      - "3000:3000"
      - "9876:9876" # Desktop Bridge RPC
    environment:
      - GEMINI_API_KEY=\${GEMINI_API_KEY}
      - TRUST_BOUNDARY=guarded
      - RUNTIME_MODE=oss-runtime
    volumes:
      - ./data/memory:/app/memory
      - ./data/skills:/app/skills
      - /var/run/docker.sock:/var/run/docker.sock
    networks:
      - agent-mesh

networks:
  agent-mesh:
    driver: bridge`
  },
  {
    name: 'model-adapter.ts',
    path: '/src/engine/model-adapter.ts',
    size: '3.1 KB',
    content: `import { GoogleGenAI } from '@google/genai';

export class ModelAdapterEngine {
  private activeEngine = 'gemini-3.8-flash';

  public async execute(prompt: string, context: Record<string, any>) {
    console.log(\`[Adapter] Routing task to \${this.activeEngine}\`);
    // Hot-swap model engine without codebase modifications
    return {
      success: true,
      engine: this.activeEngine,
      executedAt: Date.now()
    };
  }
}`
  }
];

export const initialDesktopBridge: DesktopBridgeState = {
  connected: true,
  agentHost: 'MacBook-Pro.local (192.168.1.142)',
  bridgeVersion: 'v2.8.4-darwin-arm64',
  approvedPaths: [
    '/Users/workspace/projects/fintech-dashboard',
    '/Users/workspace/projects/agent-skills'
  ],
  activePorts: [3000, 5432, 8080],
  syncStatus: 'synced',
  lastSync: '4 seconds ago'
};

export const sampleBrowserCrawlData = {
  url: 'https://news.ycombinator.com/item?id=ai-agent-runtimes',
  title: 'Autonomous Multi-Agent Runtimes & Desktop Bridges in 2026',
  page: 2,
  totalPages: 14,
  extractedItems: [
    { title: 'Hermes Agent: Zero-config OSS runtime with skill self-improvement loop', points: 342, comments: 89 },
    { title: 'Claude Cowork: Desktop bridge bridging local folders to cloud sandboxes', points: 418, comments: 134 },
    { title: 'Model Freedom: Swapping providers between Gemini, Claude, and Local Llama', points: 521, comments: 201 },
    { title: 'Shared Trust Boundaries: Securing autonomous sub-agent execution', points: 289, comments: 74 }
  ]
};
