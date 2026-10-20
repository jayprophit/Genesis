import React, { useState, useMemo } from 'react';
import { 
  Cpu, 
  Layers, 
  Shield, 
  ShieldCheck, 
  Server, 
  Database, 
  Radio, 
  Activity, 
  Zap, 
  HardDrive, 
  Sliders, 
  RefreshCw, 
  RotateCcw, 
  CheckCircle2, 
  AlertTriangle, 
  Laptop, 
  Smartphone, 
  Tablet, 
  Car, 
  Play, 
  Pause, 
  FolderTree, 
  Flame, 
  Lock, 
  Key, 
  Terminal, 
  Share2, 
  ExternalLink, 
  ChevronRight, 
  Sparkles, 
  Box, 
  Globe, 
  FileCode, 
  Binary, 
  Atom, 
  Clock, 
  Check, 
  ChevronDown
} from 'lucide-react';
import { 
  ComputeFabricDomain, 
  ComputeExecutionMode, 
  AetheriusBootMode, 
  AgentNestConfig, 
  AetheriusNode, 
  SubAgent, 
  ModelAdapterInfo, 
  DesktopBridgeState, 
  TrustBoundary 
} from '../types';

interface AetheriusAdminPanelProps {
  subAgents: SubAgent[];
  activeModel: ModelAdapterInfo;
  desktopBridge: DesktopBridgeState;
  trustBoundary: TrustBoundary;
  onDispatchTask?: (prompt: string, targetAgentId: string) => Promise<any>;
}

export const AetheriusAdminPanel: React.FC<AetheriusAdminPanelProps> = ({
  subAgents,
  activeModel,
  desktopBridge,
  trustBoundary,
  onDispatchTask
}) => {
  // Active Admin Sub-Tab
  const [adminTab, setAdminTab] = useState<'tri_compute' | 'hyper_vm' | 'hierarchy_fabric' | 'kernel_subsystems' | 'file_bridge'>('tri_compute');

  // Tri-Compute Fabric States
  const [activeDomain, setActiveDomain] = useState<ComputeFabricDomain>('tri-hybrid');
  const [executionMode, setExecutionMode] = useState<ComputeExecutionMode>('pipelined');
  const [isSimulatingPipeline, setIsSimulatingPipeline] = useState(false);
  const [pipelineOutput, setPipelineOutput] = useState<string | null>(null);

  // Compute Slider allocations
  const [binaryAllocation, setBinaryAllocation] = useState(65);
  const [ternaryAllocation, setTernaryAllocation] = useState(25);
  const [quantumAllocation, setQuantumAllocation] = useState(10);

  // Boot Mode (from diagram: Mode A Native/Bare-Metal vs Mode B Hosted/Virtual)
  const [bootMode, setBootMode] = useState<AetheriusBootMode>('hosted_virtual');

  // Agent Nests (from diagram: Worker Agent Nest 01 - Nest 06 with microVMs)
  const [agentNests, setAgentNests] = useState<AgentNestConfig[]>([
    {
      id: 'nest-01',
      name: 'Agent Nest 01 (Atlas)',
      specialistRole: 'Coding & Architecture',
      status: 'active',
      vCpuCores: 4,
      vTernaryUnits: 2,
      vQpuQubits: 8,
      memoryMb: 2048,
      microVmType: 'firecracker',
      stateSnapshotHash: '0x8f2a...7c9b',
      lastCheckpoint: '2 mins ago'
    },
    {
      id: 'nest-02',
      name: 'Agent Nest 02 (Nova)',
      specialistRole: 'Master Hive Orchestrator',
      status: 'active',
      vCpuCores: 8,
      vTernaryUnits: 4,
      vQpuQubits: 16,
      memoryMb: 4096,
      microVmType: 'firecracker',
      stateSnapshotHash: '0x3e1d...914a',
      lastCheckpoint: 'Just now'
    },
    {
      id: 'nest-03',
      name: 'Agent Nest 03 (Dev)',
      specialistRole: 'Simulation & Refactoring',
      status: 'active',
      vCpuCores: 4,
      vTernaryUnits: 2,
      vQpuQubits: 4,
      memoryMb: 1536,
      microVmType: 'gvisor',
      stateSnapshotHash: '0x7b54...20ef',
      lastCheckpoint: '5 mins ago'
    },
    {
      id: 'nest-04',
      name: 'Agent Nest 04 (Luna)',
      specialistRole: 'Research & Autonomous Web',
      status: 'state_cached',
      vCpuCores: 2,
      vTernaryUnits: 1,
      vQpuQubits: 4,
      memoryMb: 1024,
      microVmType: 'wasm_sandbox',
      stateSnapshotHash: '0x992c...ff12',
      lastCheckpoint: '12 mins ago'
    },
    {
      id: 'nest-05',
      name: 'Agent Nest 05 (Orion)',
      specialistRole: 'Zero-Trust Security & Audits',
      status: 'active',
      vCpuCores: 4,
      vTernaryUnits: 2,
      vQpuQubits: 8,
      memoryMb: 2048,
      microVmType: 'firecracker',
      stateSnapshotHash: '0x55dc...a43b',
      lastCheckpoint: '1 min ago'
    },
    {
      id: 'nest-06',
      name: 'Agent Nest 06 (Sage)',
      specialistRole: 'Data & Reusable Skills Loop',
      status: 'active',
      vCpuCores: 2,
      vTernaryUnits: 2,
      vQpuQubits: 6,
      memoryMb: 1536,
      microVmType: 'gvisor',
      stateSnapshotHash: '0x12bb...887e',
      lastCheckpoint: '4 mins ago'
    }
  ]);

  // Hierarchy Nodes (Parent -> Child -> Grandchild)
  const [nodes, setNodes] = useState<AetheriusNode[]>([
    {
      id: 'node-root-01',
      name: 'Aetherius Master Host (Parent)',
      hierarchyTier: 'parent',
      deviceType: 'master_host',
      blockchainId: 'bc1q_parent_902f',
      status: 'online',
      latencyMs: 0,
      capabilities: ['Root Authority', 'Genesis AI', 'Full Tri-Compute', 'HyperVM Master']
    },
    {
      id: 'node-child-01',
      name: 'Online Cloud Mesh (Child)',
      hierarchyTier: 'child',
      deviceType: 'cloud_layer',
      blockchainId: 'bc1q_child_cloud_41a',
      status: 'online',
      latencyMs: 14,
      capabilities: ['Distributed Sync', 'Cloud Cache', 'Backup Node']
    },
    {
      id: 'node-gc-01',
      name: 'Workstation ThinkPad (Grandchild)',
      hierarchyTier: 'grandchild',
      deviceType: 'laptop',
      blockchainId: 'bc1q_gc_laptop_77c',
      status: 'online',
      latencyMs: 6,
      capabilities: ['Local IDE', 'Desktop Bridge', 'Native NVMe']
    },
    {
      id: 'node-gc-02',
      name: 'Edge Home Server (Grandchild)',
      hierarchyTier: 'grandchild',
      deviceType: 'home_server',
      blockchainId: 'bc1q_gc_server_33d',
      status: 'online',
      latencyMs: 4,
      capabilities: ['GPU Array', '24/7 Routines', 'ZFS Storage']
    },
    {
      id: 'node-gc-03',
      name: 'Autonomous Vehicle Telemetry (Grandchild)',
      hierarchyTier: 'grandchild',
      deviceType: 'vehicle',
      blockchainId: 'bc1q_gc_veh_99e',
      status: 'syncing',
      latencyMs: 38,
      capabilities: ['CAN Bus', 'Vision NPU', 'Low-Power Edge']
    }
  ]);

  // Kernel Subsystems state (all 17 subsystems from diagram)
  const [kernelSubsystems, setKernelSubsystems] = useState([
    { name: 'Hardware Abstraction Layer (HAL)', status: 'ACTIVE', load: '14%', icon: Server },
    { name: 'Memory Manager (Virtual & NUMA)', status: 'OPTIMAL', load: '48%', icon: Database },
    { name: 'Intelligent Compute Scheduler', status: 'ACTIVE', load: '32%', icon: Clock },
    { name: 'Unified ZFS/WASM Filesystem', status: 'MOUNTED', load: '22%', icon: HardDrive },
    { name: 'Device Manager & Bus Arbiter', status: 'READY', load: '8%', icon: Laptop },
    { name: 'Hardware Driver Layer', status: 'LOADED', load: '12%', icon: Sliders },
    { name: 'Zero-Copy Virtual Networking', status: 'ACTIVE', load: '26%', icon: Radio },
    { name: 'Inter-Process Communication (IPC)', status: 'SYNCED', load: '18%', icon: Share2 },
    { name: 'Zero-Trust Security Engine', status: 'ENFORCED', load: '10%', icon: ShieldCheck },
    { name: 'Decentralized Identity Fabric', status: 'VERIFIED', load: '5%', icon: Key },
    { name: 'Granular Capability Permissions', status: 'ENFORCED', load: '7%', icon: Lock },
    { name: 'HyperVM Virtualization Manager', status: 'ACTIVE', load: '41%', icon: Box },
    { name: 'Tri-Compute Orchestrator', status: 'BALANCED', load: '35%', icon: Cpu },
    { name: 'Autonomous Agent Runtime', status: 'ACTIVE', load: '54%', icon: Sparkles },
    { name: 'Resource Quota Manager', status: 'CONSTRAINED', load: '29%', icon: Activity },
    { name: 'Universal Storage & Persistence', status: 'SYNCED', load: '61%', icon: FolderTree },
    { name: 'Power & Thermal Awareness', status: '48°C NOMINAL', load: '38W', icon: Flame },
    { name: 'Instant Recovery & Rollback', status: 'STANDBY (0s)', load: 'READY', icon: RotateCcw }
  ]);

  // Toggle Nest Status
  const handleToggleNestStatus = (nestId: string) => {
    setAgentNests(prev => prev.map(n => {
      if (n.id === nestId) {
        const nextStatus = n.status === 'active' ? 'paused' : 'active';
        return { ...n, status: nextStatus };
      }
      return n;
    }));
  };

  // Revert Nest State Snapshot
  const handleRevertNest = (nestId: string) => {
    setAgentNests(prev => prev.map(n => {
      if (n.id === nestId) {
        return { 
          ...n, 
          status: 'reverting',
          lastCheckpoint: 'Reverted just now' 
        };
      }
      return n;
    }));

    setTimeout(() => {
      setAgentNests(prev => prev.map(n => {
        if (n.id === nestId) {
          return { ...n, status: 'active' };
        }
        return n;
      }));
    }, 800);
  };

  // Simulate Tri-Compute Pipeline Run
  const handleRunTriComputeSimulation = () => {
    setIsSimulatingPipeline(true);
    setPipelineOutput('Initializing Aetherius Tri-Compute Bridge...\n[1/3] Binary Compute: Classical NPU compiling tensor AST (FP16)...\n');

    setTimeout(() => {
      setPipelineOutput(prev => prev + '[2/3] Ternary Compute: Quantizing BitNet 1.58b ternary weights (-1, 0, +1). Energy savings: 72%...\n');
    }, 600);

    setTimeout(() => {
      setPipelineOutput(prev => prev + '[3/3] Quantum Compute: Mapping variational state to virtual QPU (32 qubits). Entanglement fidelity: 99.8%...\n');
    }, 1200);

    setTimeout(() => {
      setPipelineOutput(prev => prev + '>>> Consensus verified. Verification Hash: 0x9f18e8... Tri-Compute synthesis complete in 42ms.\n');
      setIsSimulatingPipeline(false);
    }, 1800);
  };

  return (
    <div className="flex-1 flex flex-col overflow-y-auto bg-slate-950 text-slate-100 p-4 sm:p-6 space-y-6">
      {/* OS BRANDING & HEADER */}
      <div className="flex flex-col lg:flex-row lg:items-center lg:justify-between gap-4 pb-4 border-b border-slate-800/80">
        <div className="flex items-center gap-3">
          <div className="p-3 rounded-2xl bg-gradient-to-br from-indigo-500/20 via-purple-500/20 to-cyan-500/20 text-cyan-400 border border-cyan-500/30 shadow-lg shadow-cyan-500/10">
            <Atom className="w-6 h-6 animate-spin" style={{ animationDuration: '18s' }} />
          </div>
          <div>
            <div className="flex items-center gap-2 flex-wrap">
              <h1 className="text-lg sm:text-xl font-black tracking-tight text-slate-100 uppercase font-mono">
                Aetherius OS — Admin Console
              </h1>
              <span className="text-[10px] px-2.5 py-0.5 rounded-full bg-indigo-500/20 text-indigo-300 font-mono font-semibold border border-indigo-500/30">
                PARENT / ROOT AUTHORITY
              </span>
              <span className="text-[10px] px-2 py-0.5 rounded-full bg-emerald-500/20 text-emerald-300 font-mono">
                GENESIS AI ACTIVE
              </span>
            </div>
            <p className="text-xs text-slate-400 mt-0.5">
              Recursive Compute Architecture • Tri-Compute Fabric (Binary + Ternary + Quantum) • Persistent MicroVM Nests
            </p>
          </div>
        </div>

        {/* Boot Mode & Quick Host Status */}
        <div className="flex items-center gap-2 flex-wrap">
          <div className="flex items-center bg-slate-900 border border-slate-800 rounded-xl p-1 text-xs">
            <button
              onClick={() => setBootMode('hosted_virtual')}
              className={`px-3 py-1.5 rounded-lg font-medium transition-all ${
                bootMode === 'hosted_virtual'
                  ? 'bg-indigo-600/30 text-indigo-300 border border-indigo-500/40 font-semibold shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              Mode B: Hosted / Virtual
            </button>
            <button
              onClick={() => setBootMode('native_bare_metal')}
              className={`px-3 py-1.5 rounded-lg font-medium transition-all ${
                bootMode === 'native_bare_metal'
                  ? 'bg-emerald-600/30 text-emerald-300 border border-emerald-500/40 font-semibold shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              Mode A: Native Bare-Metal
            </button>
          </div>

          <div className="px-3 py-1.5 rounded-xl bg-slate-900/90 border border-slate-800 text-xs font-mono text-slate-300 flex items-center gap-2">
            <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
            <span className="text-[11px]">HAL: 17/17 Subsystems Live</span>
          </div>
        </div>
      </div>

      {/* ADMIN NAVIGATION TABS */}
      <div className="flex items-center gap-1.5 overflow-x-auto pb-1 border-b border-slate-800/80 text-xs no-scrollbar">
        <button
          onClick={() => setAdminTab('tri_compute')}
          className={`flex items-center gap-2 px-3.5 py-2 rounded-xl font-medium transition-all whitespace-nowrap ${
            adminTab === 'tri_compute'
              ? 'bg-gradient-to-r from-indigo-600/30 to-cyan-600/20 text-indigo-300 border border-indigo-500/40 font-semibold shadow-sm'
              : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900/60'
          }`}
        >
          <Cpu className="w-4 h-4 text-cyan-400" />
          <span>Tri-Compute Fabric</span>
          <span className="text-[10px] px-1.5 py-0.2 rounded bg-cyan-500/20 text-cyan-300 font-mono">0 / 1 • -1/0/+1 • Qubits</span>
        </button>

        <button
          onClick={() => setAdminTab('hyper_vm')}
          className={`flex items-center gap-2 px-3.5 py-2 rounded-xl font-medium transition-all whitespace-nowrap ${
            adminTab === 'hyper_vm'
              ? 'bg-gradient-to-r from-indigo-600/30 to-cyan-600/20 text-indigo-300 border border-indigo-500/40 font-semibold shadow-sm'
              : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900/60'
          }`}
        >
          <Box className="w-4 h-4 text-indigo-400" />
          <span>HyperVM & Agent Nests</span>
          <span className="text-[10px] px-1.5 py-0.2 rounded bg-indigo-500/20 text-indigo-300 font-mono">6 microVMs</span>
        </button>

        <button
          onClick={() => setAdminTab('hierarchy_fabric')}
          className={`flex items-center gap-2 px-3.5 py-2 rounded-xl font-medium transition-all whitespace-nowrap ${
            adminTab === 'hierarchy_fabric'
              ? 'bg-gradient-to-r from-indigo-600/30 to-cyan-600/20 text-indigo-300 border border-indigo-500/40 font-semibold shadow-sm'
              : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900/60'
          }`}
        >
          <Share2 className="w-4 h-4 text-purple-400" />
          <span>Parent / Child / Grandchild Hierarchy</span>
          <span className="text-[10px] px-1.5 py-0.2 rounded bg-purple-500/20 text-purple-300 font-mono">Blockchain Mesh</span>
        </button>

        <button
          onClick={() => setAdminTab('kernel_subsystems')}
          className={`flex items-center gap-2 px-3.5 py-2 rounded-xl font-medium transition-all whitespace-nowrap ${
            adminTab === 'kernel_subsystems'
              ? 'bg-gradient-to-r from-indigo-600/30 to-cyan-600/20 text-indigo-300 border border-indigo-500/40 font-semibold shadow-sm'
              : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900/60'
          }`}
        >
          <Sliders className="w-4 h-4 text-emerald-400" />
          <span>Kernel Subsystems (17)</span>
        </button>

        <button
          onClick={() => setAdminTab('file_bridge')}
          className={`flex items-center gap-2 px-3.5 py-2 rounded-xl font-medium transition-all whitespace-nowrap ${
            adminTab === 'file_bridge'
              ? 'bg-gradient-to-r from-indigo-600/30 to-cyan-600/20 text-indigo-300 border border-indigo-500/40 font-semibold shadow-sm'
              : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900/60'
          }`}
        >
          <FolderTree className="w-4 h-4 text-amber-400" />
          <span>Universal File Bridge</span>
          <span className="text-[10px] px-1.5 py-0.2 rounded bg-amber-500/20 text-amber-300 font-mono">Portable Drive</span>
        </button>
      </div>

      {/* TAB 1: TRI-COMPUTE FABRIC CONTROLLER */}
      {adminTab === 'tri_compute' && (
        <div className="space-y-6">
          {/* Top Banner: Genesis AI & Intelligent Compute Scheduler */}
          <div className="p-5 rounded-2xl bg-gradient-to-br from-slate-900/90 via-indigo-950/20 to-slate-950 border border-slate-800 shadow-md space-y-3">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
              <div className="flex items-center gap-3">
                <div className="p-2.5 rounded-xl bg-indigo-500/20 text-indigo-400 border border-indigo-500/30">
                  <Zap className="w-5 h-5" />
                </div>
                <div>
                  <h3 className="text-sm font-bold text-slate-100 uppercase tracking-wider font-mono">
                    Aetherius Tri-Compute Fabric
                  </h3>
                  <p className="text-xs text-slate-400">
                    Unified Compute • Intelligent Scheduler • Dynamic Resource Allocation across 3 Domains
                  </p>
                </div>
              </div>

              {/* Execution Mode Selector */}
              <div className="flex items-center gap-1.5 bg-slate-950 border border-slate-800 p-1 rounded-xl text-xs">
                <span className="text-[10px] text-slate-500 px-2 uppercase font-mono">Mode:</span>
                <button
                  onClick={() => setExecutionMode('single')}
                  className={`px-2.5 py-1 rounded-lg font-medium transition-all ${
                    executionMode === 'single' ? 'bg-indigo-600 text-white' : 'text-slate-400 hover:text-slate-200'
                  }`}
                >
                  Single Domain
                </button>
                <button
                  onClick={() => setExecutionMode('pipelined')}
                  className={`px-2.5 py-1 rounded-lg font-medium transition-all ${
                    executionMode === 'pipelined' ? 'bg-indigo-600 text-white' : 'text-slate-400 hover:text-slate-200'
                  }`}
                >
                  Pipelined (B → T → Q)
                </button>
                <button
                  onClick={() => setExecutionMode('parallel')}
                  className={`px-2.5 py-1 rounded-lg font-medium transition-all ${
                    executionMode === 'parallel' ? 'bg-indigo-600 text-white' : 'text-slate-400 hover:text-slate-200'
                  }`}
                >
                  Parallel Consensus
                </button>
              </div>
            </div>
          </div>

          {/* 3 COMPUTE DOMAIN CARDS (From Diagram: Binary, Ternary, Quantum) */}
          <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
            {/* DOMAIN 1: BINARY COMPUTE (0 / 1) */}
            <div className="p-4 rounded-2xl bg-slate-900/80 border border-slate-800/90 space-y-3 relative overflow-hidden group hover:border-cyan-500/40 transition-all">
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-2">
                  <div className="p-2 rounded-xl bg-cyan-500/10 text-cyan-400 border border-cyan-500/20">
                    <Binary className="w-5 h-5" />
                  </div>
                  <div>
                    <h4 className="text-xs font-bold text-slate-100 uppercase tracking-wide">Binary Compute</h4>
                    <span className="text-[10px] font-mono text-cyan-400 font-semibold">0 / 1 (Classical)</span>
                  </div>
                </div>
                <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-cyan-500/20 text-cyan-300">
                  {binaryAllocation}% Quota
                </span>
              </div>

              <div className="space-y-1 text-[11px] text-slate-300 font-mono pt-1">
                <div className="flex justify-between">
                  <span className="text-slate-500">Accelerators:</span>
                  <span className="text-slate-200">CPU + GPU + NPU</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">Throughput:</span>
                  <span className="text-cyan-400 font-bold">142 TFLOPS (FP16)</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">Role:</span>
                  <span className="text-slate-300">Compilation & Tokenizer</span>
                </div>
              </div>

              {/* Range Slider */}
              <div className="pt-2">
                <div className="flex justify-between text-[10px] text-slate-500 font-mono mb-1">
                  <span>Scheduler Weight</span>
                  <span>{binaryAllocation}%</span>
                </div>
                <input
                  type="range"
                  min={10}
                  max={90}
                  value={binaryAllocation}
                  onChange={(e) => setBinaryAllocation(Number(e.target.value))}
                  className="w-full accent-cyan-500 cursor-pointer"
                />
              </div>
            </div>

            {/* DOMAIN 2: TERNARY COMPUTE (-1 / 0 / +1) */}
            <div className="p-4 rounded-2xl bg-slate-900/80 border border-slate-800/90 space-y-3 relative overflow-hidden group hover:border-emerald-500/40 transition-all">
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-2">
                  <div className="p-2 rounded-xl bg-emerald-500/10 text-emerald-400 border border-emerald-500/20">
                    <Cpu className="w-5 h-5" />
                  </div>
                  <div>
                    <h4 className="text-xs font-bold text-slate-100 uppercase tracking-wide">Ternary Compute</h4>
                    <span className="text-[10px] font-mono text-emerald-400 font-semibold">-1 / 0 / +1 (BitNet)</span>
                  </div>
                </div>
                <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-300">
                  {ternaryAllocation}% Quota
                </span>
              </div>

              <div className="space-y-1 text-[11px] text-slate-300 font-mono pt-1">
                <div className="flex justify-between">
                  <span className="text-slate-500">Accelerators:</span>
                  <span className="text-slate-200">Ternary CPU / 1.58b ASIC</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">Energy Savings:</span>
                  <span className="text-emerald-400 font-bold">-71.4% Watt/Token</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">Role:</span>
                  <span className="text-slate-300">Quantized Reasoning</span>
                </div>
              </div>

              {/* Range Slider */}
              <div className="pt-2">
                <div className="flex justify-between text-[10px] text-slate-500 font-mono mb-1">
                  <span>Scheduler Weight</span>
                  <span>{ternaryAllocation}%</span>
                </div>
                <input
                  type="range"
                  min={5}
                  max={80}
                  value={ternaryAllocation}
                  onChange={(e) => setTernaryAllocation(Number(e.target.value))}
                  className="w-full accent-emerald-500 cursor-pointer"
                />
              </div>
            </div>

            {/* DOMAIN 3: QUANTUM COMPUTE (Qubits) */}
            <div className="p-4 rounded-2xl bg-slate-900/80 border border-slate-800/90 space-y-3 relative overflow-hidden group hover:border-purple-500/40 transition-all">
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-2">
                  <div className="p-2 rounded-xl bg-purple-500/10 text-purple-400 border border-purple-500/20">
                    <Atom className="w-5 h-5" />
                  </div>
                  <div>
                    <h4 className="text-xs font-bold text-slate-100 uppercase tracking-wide">Quantum Compute</h4>
                    <span className="text-[10px] font-mono text-purple-400 font-semibold">Qubits (Virtual QPU)</span>
                  </div>
                </div>
                <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-purple-500/20 text-purple-300">
                  {quantumAllocation}% Quota
                </span>
              </div>

              <div className="space-y-1 text-[11px] text-slate-300 font-mono pt-1">
                <div className="flex justify-between">
                  <span className="text-slate-500">Engine:</span>
                  <span className="text-slate-200">QPU Sim & State Vector</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">Active Qubits:</span>
                  <span className="text-purple-400 font-bold">32 Virtual Qubits</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-slate-500">Role:</span>
                  <span className="text-slate-300">Combinatorial Optimization</span>
                </div>
              </div>

              {/* Range Slider */}
              <div className="pt-2">
                <div className="flex justify-between text-[10px] text-slate-500 font-mono mb-1">
                  <span>Scheduler Weight</span>
                  <span>{quantumAllocation}%</span>
                </div>
                <input
                  type="range"
                  min={2}
                  max={50}
                  value={quantumAllocation}
                  onChange={(e) => setQuantumAllocation(Number(e.target.value))}
                  className="w-full accent-purple-500 cursor-pointer"
                />
              </div>
            </div>
          </div>

          {/* INTERACTIVE SCHEDULER TEST BENCH */}
          <div className="p-5 rounded-2xl bg-slate-900/60 border border-slate-800 space-y-4">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
              <div>
                <h4 className="text-xs font-bold text-slate-200 uppercase tracking-wider flex items-center gap-2">
                  <Terminal className="w-4 h-4 text-indigo-400" />
                  Tri-Compute Bridge & Intelligent Scheduler Pipeline
                </h4>
                <p className="text-[11px] text-slate-400">
                  Evaluates Task • Hardware • Energy • Latency • Accuracy • Cost • Availability • Permissions
                </p>
              </div>

              <button
                onClick={handleRunTriComputeSimulation}
                disabled={isSimulatingPipeline}
                className="flex items-center gap-2 px-4 py-2 rounded-xl bg-indigo-600 hover:bg-indigo-500 disabled:bg-slate-800 text-white text-xs font-semibold shadow-md transition-all self-start sm:self-auto"
              >
                <Play className={`w-3.5 h-3.5 ${isSimulatingPipeline ? 'animate-spin' : ''}`} />
                <span>{isSimulatingPipeline ? 'Executing Pipeline...' : 'Run Compute Pipeline Benchmark'}</span>
              </button>
            </div>

            {/* Terminal output box */}
            <div className="rounded-xl border border-slate-800 bg-[#0a0e17] p-3.5 font-mono text-xs text-slate-300 h-36 overflow-y-auto leading-relaxed">
              {pipelineOutput ? (
                <pre className="whitespace-pre-wrap">{pipelineOutput}</pre>
              ) : (
                <div className="text-slate-500 flex items-center justify-center h-full">
                  Click "Run Compute Pipeline Benchmark" to evaluate Binary → Ternary → Quantum dispatch latency.
                </div>
              )}
            </div>
          </div>
        </div>
      )}

      {/* TAB 2: HYPERVM & AGENT NESTS */}
      {adminTab === 'hyper_vm' && (
        <div className="space-y-6">
          <div className="p-5 rounded-2xl bg-gradient-to-br from-slate-900/90 to-slate-950 border border-slate-800 space-y-2">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2.5">
                <div className="p-2 rounded-xl bg-indigo-500/20 text-indigo-400">
                  <Box className="w-5 h-5" />
                </div>
                <div>
                  <h3 className="text-sm font-bold text-slate-100 uppercase tracking-wider font-mono">
                    Aetherius HyperVM (Recursive Virtual Compute)
                  </h3>
                  <p className="text-xs text-slate-400">
                    Parent VM (System) orchestrates isolated Worker Agent Nests with private microVMs, memory caches, and temporal state snapshots.
                  </p>
                </div>
              </div>

              <span className="text-xs font-mono px-2.5 py-1 rounded-lg bg-emerald-500/20 text-emerald-300 border border-emerald-500/30">
                HYPERVISOR: KVM + Firecracker
              </span>
            </div>
          </div>

          {/* GRID OF AGENT NESTS (from diagram: Worker Agent Nest 01 - N) */}
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
            {agentNests.map((nest) => (
              <div 
                key={nest.id} 
                className="p-4 rounded-2xl bg-slate-900/70 border border-slate-800/90 space-y-3.5 hover:border-slate-700 transition-all"
              >
                <div className="flex items-center justify-between">
                  <div>
                    <h4 className="text-xs font-bold text-slate-100">{nest.name}</h4>
                    <p className="text-[10px] text-slate-400">{nest.specialistRole}</p>
                  </div>
                  <span className={`text-[10px] font-mono px-2 py-0.5 rounded capitalize ${
                    nest.status === 'active'
                      ? 'bg-emerald-500/20 text-emerald-300 border border-emerald-500/30'
                      : nest.status === 'state_cached'
                      ? 'bg-indigo-500/20 text-indigo-300 border border-indigo-500/30'
                      : 'bg-amber-500/20 text-amber-300 border border-amber-500/30'
                  }`}>
                    {nest.status}
                  </span>
                </div>

                {/* Virtual Compute Allocations */}
                <div className="p-2.5 rounded-xl bg-slate-950/80 border border-slate-800/80 space-y-1.5 text-[10px] font-mono">
                  <div className="flex justify-between text-slate-400">
                    <span>MicroVM Sandbox:</span>
                    <span className="text-slate-200 uppercase">{nest.microVmType}</span>
                  </div>
                  <div className="flex justify-between text-slate-400">
                    <span>Virtual CPU:</span>
                    <span className="text-cyan-400 font-bold">{nest.vCpuCores} vCPU Cores</span>
                  </div>
                  <div className="flex justify-between text-slate-400">
                    <span>Ternary Units:</span>
                    <span className="text-emerald-400 font-bold">{nest.vTernaryUnits} vTernary</span>
                  </div>
                  <div className="flex justify-between text-slate-400">
                    <span>Quantum Virtual:</span>
                    <span className="text-purple-400 font-bold">{nest.vQpuQubits} vQubits</span>
                  </div>
                  <div className="flex justify-between text-slate-400">
                    <span>RAM Working Set:</span>
                    <span className="text-slate-200">{nest.memoryMb} MB</span>
                  </div>
                </div>

                {/* State Snapshot & Rollback */}
                <div className="flex items-center justify-between text-[10px] font-mono text-slate-400 pt-1 border-t border-slate-800/60">
                  <span>Hash: {nest.stateSnapshotHash}</span>
                  <span className="text-slate-500">{nest.lastCheckpoint}</span>
                </div>

                {/* Interactive Nest Controls (from diagram User Options: Active, Output, State Cached, Inspect, Revert, Resumed) */}
                <div className="flex items-center gap-2 pt-1">
                  <button
                    onClick={() => handleToggleNestStatus(nest.id)}
                    className="flex-1 flex items-center justify-center gap-1 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-medium transition-colors"
                  >
                    {nest.status === 'active' ? (
                      <>
                        <Pause className="w-3 h-3 text-amber-400" />
                        <span>Pause</span>
                      </>
                    ) : (
                      <>
                        <Play className="w-3 h-3 text-emerald-400" />
                        <span>Resume</span>
                      </>
                    )}
                  </button>

                  <button
                    onClick={() => handleRevertNest(nest.id)}
                    className="flex-1 flex items-center justify-center gap-1 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-medium transition-colors"
                    title="Rollback nest to clean state snapshot"
                  >
                    <RotateCcw className="w-3 h-3 text-indigo-400" />
                    <span>Revert</span>
                  </button>
                </div>
              </div>
            ))}
          </div>
        </div>
      )}

      {/* TAB 3: HIERARCHY & BLOCKCHAIN NETWORK FABRIC */}
      {adminTab === 'hierarchy_fabric' && (
        <div className="space-y-6">
          {/* Overview of Parent / Child / Grandchild */}
          <div className="p-5 rounded-2xl bg-slate-900/80 border border-slate-800 space-y-3">
            <div className="flex items-center gap-2.5">
              <div className="p-2 rounded-xl bg-purple-500/20 text-purple-400">
                <Share2 className="w-5 h-5" />
              </div>
              <div>
                <h3 className="text-sm font-bold text-slate-100 uppercase tracking-wider font-mono">
                  Parent → Child → Grandchild Hierarchy
                </h3>
                <p className="text-xs text-slate-400">
                  Recursive compute trust mesh connecting Root Authority (Parent), Online Cloud Layer (Child), and Authorized Devices (Grandchild Nodes).
                </p>
              </div>
            </div>

            {/* Cryptographic Identity Badge */}
            <div className="grid grid-cols-2 sm:grid-cols-4 gap-2 pt-2 text-[11px] font-mono">
              <div className="p-2 rounded-xl bg-slate-950 border border-slate-800">
                <span className="text-slate-500 block text-[10px]">User ID</span>
                <span className="text-slate-200 font-bold">usr_aeth_89201</span>
              </div>
              <div className="p-2 rounded-xl bg-slate-950 border border-slate-800">
                <span className="text-slate-500 block text-[10px]">AI Master ID</span>
                <span className="text-indigo-400 font-bold">ai_genesis_root</span>
              </div>
              <div className="p-2 rounded-xl bg-slate-950 border border-slate-800">
                <span className="text-slate-500 block text-[10px]">Blockchain ID</span>
                <span className="text-cyan-400 font-bold">bc1q_aeth_fabric</span>
              </div>
              <div className="p-2 rounded-xl bg-slate-950 border border-slate-800">
                <span className="text-slate-500 block text-[10px]">Smart Contract</span>
                <span className="text-emerald-400 font-bold">0x48...9e21 (OK)</span>
              </div>
            </div>
          </div>

          {/* Connected Nodes in the Mesh */}
          <div className="rounded-2xl border border-slate-800 bg-slate-900/60 p-5 space-y-4">
            <div className="flex items-center justify-between">
              <h4 className="text-xs font-bold text-slate-200 uppercase tracking-wider flex items-center gap-2">
                <Radio className="w-4 h-4 text-emerald-400" />
                Enrolled Nodes & Authorized Device Fabric
              </h4>
              <span className="text-[10px] font-mono text-slate-400">
                Zero-Trust Capability Enforced
              </span>
            </div>

            <div className="space-y-2 font-mono text-xs">
              {nodes.map((node) => {
                const isParent = node.hierarchyTier === 'parent';
                const isChild = node.hierarchyTier === 'child';
                return (
                  <div 
                    key={node.id}
                    className="p-3.5 rounded-xl bg-slate-950 border border-slate-800 flex flex-col sm:flex-row sm:items-center justify-between gap-3 hover:border-slate-700 transition-colors"
                  >
                    <div className="flex items-center gap-3">
                      <div className={`p-2 rounded-lg ${
                        isParent ? 'bg-indigo-500/20 text-indigo-400' : isChild ? 'bg-purple-500/20 text-purple-400' : 'bg-slate-800 text-slate-300'
                      }`}>
                        {node.deviceType === 'master_host' && <Server className="w-4 h-4" />}
                        {node.deviceType === 'cloud_layer' && <Globe className="w-4 h-4" />}
                        {node.deviceType === 'laptop' && <Laptop className="w-4 h-4" />}
                        {node.deviceType === 'home_server' && <HardDrive className="w-4 h-4" />}
                        {node.deviceType === 'vehicle' && <Car className="w-4 h-4" />}
                      </div>

                      <div>
                        <div className="flex items-center gap-2">
                          <span className="font-bold text-slate-200">{node.name}</span>
                          <span className={`text-[10px] px-2 py-0.2 rounded uppercase font-bold ${
                            isParent ? 'bg-indigo-900/60 text-indigo-300 border border-indigo-700' : isChild ? 'bg-purple-900/60 text-purple-300' : 'bg-slate-800 text-slate-400'
                          }`}>
                            {node.hierarchyTier}
                          </span>
                        </div>
                        <div className="flex items-center gap-2 text-[10px] text-slate-400 mt-0.5">
                          <span>ID: {node.blockchainId}</span>
                          <span>•</span>
                          <span>Latency: {node.latencyMs}ms</span>
                        </div>
                      </div>
                    </div>

                    <div className="flex items-center gap-2 self-end sm:self-auto">
                      <div className="hidden md:flex items-center gap-1 text-[10px] text-slate-500">
                        {node.capabilities.map((cap, i) => (
                          <span key={i} className="px-1.5 py-0.5 rounded bg-slate-900 border border-slate-800 text-slate-400">
                            {cap}
                          </span>
                        ))}
                      </div>

                      <span className="text-[10px] px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-300 uppercase font-bold">
                        {node.status}
                      </span>
                    </div>
                  </div>
                );
              })}
            </div>
          </div>
        </div>
      )}

      {/* TAB 4: KERNEL SUBSYSTEMS (THE 17 SUBSYSTEMS) */}
      {adminTab === 'kernel_subsystems' && (
        <div className="space-y-6">
          <div className="p-5 rounded-2xl bg-slate-900/80 border border-slate-800 space-y-2">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2.5">
                <div className="p-2 rounded-xl bg-emerald-500/20 text-emerald-400">
                  <Sliders className="w-5 h-5" />
                </div>
                <div>
                  <h3 className="text-sm font-bold text-slate-100 uppercase tracking-wider font-mono">
                    Aetherius Kernel Subsystems
                  </h3>
                  <p className="text-xs text-slate-400">
                    Live operational state across all 17 fundamental OS architecture modules (from HAL to Recovery & Rollback).
                  </p>
                </div>
              </div>

              <span className="text-xs font-mono px-2.5 py-1 rounded-lg bg-emerald-500/20 text-emerald-300">
                KERNEL: AETHERIUS-LTS 6.12-AI
              </span>
            </div>
          </div>

          {/* Grid of 17 Subsystems */}
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-3">
            {kernelSubsystems.map((sub, idx) => {
              const Icon = sub.icon;
              return (
                <div 
                  key={idx}
                  className="p-3.5 rounded-xl bg-slate-950/80 border border-slate-800/80 hover:border-slate-700 transition-all flex items-center justify-between"
                >
                  <div className="flex items-center gap-2.5">
                    <div className="p-2 rounded-lg bg-slate-900 text-indigo-400 border border-slate-800">
                      <Icon className="w-4 h-4" />
                    </div>
                    <div>
                      <h5 className="text-xs font-semibold text-slate-200">{sub.name}</h5>
                      <span className="text-[10px] font-mono text-slate-500">Load: {sub.load}</span>
                    </div>
                  </div>

                  <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-300 font-bold">
                    {sub.status}
                  </span>
                </div>
              );
            })}
          </div>
        </div>
      )}

      {/* TAB 5: UNIVERSAL FILE BRIDGE & PORTABLE DRIVE */}
      {adminTab === 'file_bridge' && (
        <div className="space-y-6">
          <div className="p-5 rounded-2xl bg-gradient-to-br from-slate-900/90 to-slate-950 border border-slate-800 space-y-3">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2.5">
                <div className="p-2 rounded-xl bg-amber-500/20 text-amber-400">
                  <FolderTree className="w-5 h-5" />
                </div>
                <div>
                  <h3 className="text-sm font-bold text-slate-100 uppercase tracking-wider font-mono">
                    Universal File Bridge & Master Drive
                  </h3>
                  <p className="text-xs text-slate-400">
                    Shared workspace across Windows, Linux, macOS, Android, Tablet, and Aetherius Bare-Metal.
                  </p>
                </div>
              </div>

              <span className="text-xs font-mono px-2.5 py-1 rounded-lg bg-indigo-500/20 text-indigo-300">
                DRAG • DROP • COPY • SYNC
              </span>
            </div>
          </div>

          {/* Directory Grid (From Diagram: Desktop, Code, CAD, Documents, Prompts, AI Models, Pictures, Packages, Firmware, Archives) */}
          <div className="grid grid-cols-2 sm:grid-cols-4 lg:grid-cols-5 gap-3">
            {[
              { name: 'Desktop', files: '14 files', icon: Laptop },
              { name: 'Code', files: '1,420 files', icon: FileCode },
              { name: 'CAD / Eng', files: '38 assets', icon: Box },
              { name: 'Documents', files: '85 docs', icon: HardDrive },
              { name: 'Prompts', files: '240 prompts', icon: Sparkles },
              { name: 'AI Models', files: '6 weights', icon: Cpu },
              { name: 'Pictures', files: '512 imgs', icon: HardDrive },
              { name: 'Packages', files: '94 wheels', icon: Box },
              { name: 'Firmware', files: '12 binaries', icon: Terminal },
              { name: 'Archives', files: '48 zips', icon: FolderTree }
            ].map((dir, idx) => {
              const Icon = dir.icon;
              return (
                <div 
                  key={idx}
                  className="p-3.5 rounded-xl bg-slate-950 border border-slate-800/80 hover:border-amber-500/40 transition-all text-center space-y-1.5 cursor-pointer group"
                >
                  <div className="w-8 h-8 rounded-lg bg-slate-900 group-hover:bg-amber-500/10 group-hover:text-amber-400 text-slate-300 border border-slate-800 mx-auto flex items-center justify-center transition-colors">
                    <Icon className="w-4 h-4" />
                  </div>
                  <div className="text-xs font-bold text-slate-200 group-hover:text-amber-300 transition-colors">
                    {dir.name}
                  </div>
                  <div className="text-[10px] font-mono text-slate-500">
                    {dir.files}
                  </div>
                </div>
              );
            })}
          </div>

          {/* Bridge Status Summary */}
          <div className="p-4 rounded-xl bg-slate-900/60 border border-slate-800 flex flex-col sm:flex-row sm:items-center justify-between gap-3 text-xs font-mono">
            <div className="flex items-center gap-2 text-slate-300">
              <span className="w-2.5 h-2.5 rounded-full bg-emerald-400 animate-pulse" />
              <span>Desktop Bridge Active: {desktopBridge.agentHost}</span>
            </div>
            <div className="flex items-center gap-3 text-slate-400">
              <span>Sync Status: <strong className="text-emerald-400 uppercase">{desktopBridge.syncStatus}</strong></span>
              <span>•</span>
              <span>HMAC SHA-256 Validated</span>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
