import React, { useState } from 'react';
import { ActiveSimulation, SimulationParameters, SimulationEpoch, SimulationReality, SimulationFinding } from '../types';

interface SimulationHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  activeSimulation: ActiveSimulation | null;
  onStartSimulation: (params: SimulationParameters) => void;
  onClearSimulation: () => void;
}

const WorldMap: React.FC = () => (
    <div className="w-full h-48 bg-blue-900/20 rounded-lg relative overflow-hidden border border-blue-500/20">
        <svg className="absolute inset-0 w-full h-full" preserveAspectRatio="xMidYMid slice" viewBox="0 0 1000 500">
            <path d="M499.5,4.7L499.5,4.7c-1.8,0.3-3.6,0.6-5.4,1c-15.3,3.4-30.2,8.8-44.2,16.2C402.2,50.1,353.4,75.2,308.6,98 c-52.7,26.8-111.4,45.3-172.9,54.8C75.2,162.1,19.3,165.7,4.8,175.7c-5.8,4-10.4,9.6-1.9,13.9c25.3,12.7,53.2,21,81.5,27.7 c39.5,9.4,80.1,14.2,120.9,17.4c64.8,5,129.8-1.5,193.8-10.1c56-7.6,111.2-18.7,165.4-33.1c55.3-14.7,109.4-32.6,161.4-53.8 c29.1-11.8,57.5-24.9,84.9-39.2c16.1-8.4,31.8-17.5,46.7-27.6l0,0c-20.2-12.7-41.2-22.9-63.1-30.8c-28-10.1-56.9-17.7-86.4-23.3 C652.1,38.8,575.4,22.2,499.5,4.7z" fill="rgba(6, 78, 110, 0.4)"></path>
            <path d="M4.8,175.7c-5.8,4-10.4,9.6-1.9,13.9c25.3,12.7,53.2,21,81.5,27.7c39.5,9.4,80.1,14.2,120.9,17.4 c64.8,5,129.8-1.5,193.8-10.1c56-7.6,111.2-18.7,165.4-33.1c55.3-14.7,109.4-32.6,161.4-53.8c29.1-11.8,57.5-24.9,84.9-39.2 c16.1-8.4,31.8-17.5,46.7-27.6l0,0c-20.2-12.7-41.2-22.9-63.1-30.8c-28-10.1-56.9-17.7-86.4-23.3C652.1,38.8,575.4,22.2,499.5,4.7 c-75.9-17.5-151.7-17.5-227.6,0.1c-68.9,16-136.2,39.3-197.4,69.5C41.3,90.2,14.9,109.9-1,130.5c-6,7.8-10.2,17.7-4.1,26.4 C9.4,166.4,12.5,170.8,4.8,175.7z" fill="rgba(6, 78, 110, 0.6)"></path>
        </svg>
        <div className="absolute inset-0 grid grid-cols-20 grid-rows-10">
            {[...Array(200)].map((_, i) => <div key={i} className="border border-blue-500/5 opacity-50"></div>)}
        </div>
        <div className="scanline"></div>
        <style>{`.scanline { position: absolute; top: 0; left: 0; width: 100%; height: 3px; background: rgba(0, 255, 255, 0.5); animation: scan 4s linear infinite; } @keyframes scan { 0% { top: 0; } 100% { top: 100%; } }`}</style>
    </div>
);

const BreakthroughCard: React.FC<{ finding: SimulationFinding }> = ({ finding }) => {
    const getDomainStyle = (domain: SimulationFinding['domain']) => {
        switch(domain) {
            case 'Medicine': return { icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path d="M3 4a1 1 0 011-1h12a1 1 0 011 1v2a1 1 0 01-1 1H4a1 1 0 01-1-1V4zM3 10a1 1 0 011-1h6a1 1 0 011 1v6a1 1 0 01-1 1H4a1 1 0 01-1-1v-6zM14 9a1 1 0 00-1 1v6a1 1 0 001 1h2a1 1 0 001-1v-6a1 1 0 00-1-1h-2z" /></svg>, color: 'text-cyan-300', border: 'border-cyan-500/30' };
            case 'Technology': return { icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M11.49 3.17c-.38-1.56-2.6-1.56-2.98 0a1.532 1.532 0 01-2.286.948c-1.372-.836-2.942.734-2.106 2.106.54.962.062 2.18-.948 2.287-1.561.379-1.561 2.6 0 2.978a1.532 1.532 0 01.947 2.287c-.836 1.372.734 2.942 2.106 2.106a1.532 1.532 0 012.287.947c.379 1.561 2.6 1.561 2.978 0a1.533 1.533 0 012.287-.947c1.372.836 2.942-.734 2.106-2.106a1.533 1.533 0 01.947-2.287c1.561-.379 1.561-2.6 0-2.978a1.532 1.532 0 01-.947-2.287c.836-1.372-.734-2.942-2.106-2.106a1.532 1.532 0 01-2.287-.947zM10 13a3 3 0 100-6 3 3 0 000 6z" clipRule="evenodd" /></svg>, color: 'text-yellow-300', border: 'border-yellow-500/30' };
            case 'Socio-Economics': return { icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path d="M13 6a3 3 0 11-6 0 3 3 0 016 0zM18 8a2 2 0 11-4 0 2 2 0 014 0zM14 15a4 4 0 00-8 0v3h8v-3zM6 8a2 2 0 11-4 0 2 2 0 014 0zM16 18v-3a5.972 5.972 0 00-.75-2.906A3.005 3.005 0 0119 15v3h-3zM4.75 12.094A5.973 5.973 0 004 15v3H1v-3a3 3 0 013.75-2.906z" /></svg>, color: 'text-purple-300', border: 'border-purple-500/30' };
            case 'Environment': return { icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path d="M9 2a1 1 0 000 2h2a1 1 0 100-2H9z" /><path fillRule="evenodd" d="M4 5a2 2 0 012-2 3 3 0 003 3h2a3 3 0 003-3 2 2 0 012 2v11a2 2 0 01-2 2H6a2 2 0 01-2-2V5zm3 4a1 1 0 000 2h.01a1 1 0 100-2H7zm3 0a1 1 0 000 2h3a1 1 0 100-2h-3z" clipRule="evenodd" /></svg>, color: 'text-green-300', border: 'border-green-500/30' };
            default: return { icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7-4a1 1 0 11-2 0 1 1 0 012 0zM9 9a1 1 0 000 2v3a1 1 0 001 1h1a1 1 0 100-2v-3a1 1 0 00-1-1H9z" clipRule="evenodd" /></svg>, color: 'text-gray-300', border: 'border-gray-500/30' };
        }
    }
    const { icon, color, border } = getDomainStyle(finding.domain);
    return (
        <div className={`p-4 bg-gray-800/50 rounded-lg border ${border}`}>
            <div className={`flex items-center font-semibold text-lg ${color} mb-2`}>
                {icon}
                <span className="ml-2">{finding.domain} Breakthrough</span>
            </div>
            <div className="space-y-3 text-sm">
                <div>
                    <h4 className="font-semibold text-gray-300">Finding:</h4>
                    <p className="text-gray-400">{finding.finding}</p>
                </div>
                 <div>
                    <h4 className="font-semibold text-gray-300">Implication:</h4>
                    <p className="text-gray-400">{finding.implication}</p>
                </div>
                 <div>
                    <h4 className="font-semibold text-gray-300">Recommendation:</h4>
                    <p className="text-gray-200 bg-gray-900/50 p-2 rounded-md border-l-2 border-blue-400">{finding.recommendation}</p>
                </div>
            </div>
        </div>
    )
};


const SimulationHubModal: React.FC<SimulationHubModalProps> = ({ isOpen, onClose, activeSimulation, onStartSimulation, onClearSimulation }) => {
  const [params, setParams] = useState<SimulationParameters>({
    epoch: 'Future',
    reality: 'Speculative Future',
    variables: 'High Technology, Environmental Strain, Global Cooperation',
    query: ''
  });

  if (!isOpen) return null;

  const handleStart = () => {
    if (params.query.trim()) {
      onStartSimulation(params);
    }
  };
  
  const renderConfiguration = () => (
    <>
      <div className="p-6 flex-shrink-0">
        <p className="text-gray-400 mb-4">Define the parameters of a virtual world to run a simulation and generate potential solutions for complex problems.</p>
        <div className="space-y-4">
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
              <label className="block text-sm font-medium text-gray-300 mb-1">Epoch</label>
              <select value={params.epoch} onChange={e => setParams(p => ({...p, epoch: e.target.value as SimulationEpoch}))} className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500">
                <option>Past</option><option>Present</option><option>Future</option>
              </select>
            </div>
            <div>
              <label className="block text-sm font-medium text-gray-300 mb-1">Base Reality</label>
              <select value={params.reality} onChange={e => setParams(p => ({...p, reality: e.target.value as SimulationReality}))} className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500">
                <option>Earth History</option><option>Alternate History</option><option>Speculative Future</option>
              </select>
            </div>
          </div>
          <div>
            <label className="block text-sm font-medium text-gray-300 mb-1">Core Variables</label>
            <input type="text" value={params.variables} onChange={e => setParams(p => ({...p, variables: e.target.value}))} placeholder="e.g., Resource Scarcity, AI Governance..." className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500" />
          </div>
          <div>
            <label className="block text-sm font-medium text-gray-300 mb-1">Test Scenario / Query</label>
            <textarea value={params.query} onChange={e => setParams(p => ({...p, query: e.target.value}))} rows={4} placeholder="Describe the problem to solve..." className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500 resize-none" />
          </div>
        </div>
      </div>
      <footer className="p-6 flex-shrink-0 border-t border-blue-500/20 text-right">
        <button onClick={handleStart} disabled={!params.query.trim()} className="bg-blue-600 hover:bg-blue-500 text-white font-bold px-6 py-2 rounded-lg transition-colors disabled:opacity-50 disabled:cursor-not-allowed">
          Launch Simulation
        </button>
      </footer>
    </>
  );

  const renderMonitoring = () => {
    if (!activeSimulation) return null;
    const isFinished = activeSimulation.status === 'Complete' || activeSimulation.status === 'Error';
    return (
      <div className="p-6 flex-1 flex flex-col min-h-0">
        <div className="flex-shrink-0">
          <WorldMap />
          <div className="my-4 text-center">
              <h3 className="text-lg font-semibold text-blue-300 truncate" title={activeSimulation.parameters.query}>Simulating: {activeSimulation.parameters.query}</h3>
          </div>
          <div className="my-2 space-y-2">
              <div className="flex justify-between text-sm text-gray-400">
                  <span>Status: <span className="font-semibold text-cyan-300">{activeSimulation.status}...</span></span>
                  <span>Elapsed Time: <span className="font-semibold text-purple-300">{activeSimulation.elapsedTime}</span></span>
              </div>
              <div className="w-full bg-gray-700/50 rounded-full h-2.5">
                  <div className="bg-gradient-to-r from-blue-500 to-purple-500 h-2.5 rounded-full transition-all duration-500" style={{width: `${activeSimulation.progress}%`}}></div>
              </div>
          </div>
        </div>
        <div className="flex-1 mt-4 bg-black/30 rounded-lg p-3 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent min-h-0">
            {isFinished ? (
                 <div className="space-y-6">
                    <div>
                        <h4 className="text-xl font-bold text-gray-200 mb-2">Simulation Summary</h4>
                        <p className="text-gray-300 whitespace-pre-wrap">{activeSimulation.result?.summary}</p>
                    </div>
                     <div>
                        <h4 className="text-xl font-bold text-gray-200 mb-2">Comparative Analysis</h4>
                        <p className="text-gray-300 whitespace-pre-wrap">{activeSimulation.result?.comparativeAnalysis}</p>
                    </div>
                     <div>
                        <h4 className="text-xl font-bold text-gray-200 mb-3">Identified Breakthroughs</h4>
                        <div className="space-y-4">
                           {activeSimulation.result?.breakthroughs.map((finding, i) => <BreakthroughCard key={i} finding={finding} />)}
                        </div>
                    </div>
                 </div>
            ) : (
                <div className="font-mono text-xs text-green-400">
                    <p>&gt; Quantum Core synchronized.</p>
                    <p>&gt; Initializing reality parameters from Epoch: {activeSimulation.parameters.epoch}.</p>
                    <p>&gt; Weaving core variables into space-time matrix...</p>
                    {activeSimulation.progress > 20 && <p>&gt; [Year 200] Anomaly detected: Unexpected technological leap.</p>}
                    {activeSimulation.progress > 50 && <p>&gt; [Year 550] Adjusting societal models based on emergent behavior.</p>}
                    {activeSimulation.progress > 80 && <p>&gt; [Year 820] Reaching predicted outcome threshold.</p>}
                    {activeSimulation.status === 'Analyzing' && <p className="text-yellow-400">&gt; Simulation complete. Collating data streams for analysis...</p>}
                </div>
            )}
        </div>
         {isFinished && (
            <div className="mt-4 text-center flex-shrink-0">
                <button onClick={onClearSimulation} className="bg-blue-600 hover:bg-blue-500 text-white font-bold px-6 py-2 rounded-lg transition-colors">
                    New Simulation
                </button>
            </div>
         )}
      </div>
    );
  };

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-3xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(500px, 90vh, 800px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <h2 className="text-2xl font-bold text-blue-300">Simulation Hub</h2>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        
        {activeSimulation ? renderMonitoring() : renderConfiguration()}

        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default SimulationHubModal;