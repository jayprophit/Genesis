
import React, { useState, useEffect } from 'react';
import { ActiveSimulation, NexusData } from '../types';
import CognitiveNexus from './CognitiveNexus';

interface SystemStatusProps {
    orbState: 'idle' | 'thinking' | 'speaking' | 'listening';
    activeSimulation: ActiveSimulation | null;
    theme: 'light' | 'dark';
    nexusData: NexusData | null;
    isLoading: boolean;
}

const wbeSubstrate = [
    { name: "Neural Activity", unit: "Hz" },
    { name: "Synaptic Density", unit: "%" },
    { name: "Data Throughput", unit: "Pb/s" },
    { name: "Cognitive Fluid", unit: "L" },
];

const genesisLinkStatus = [ "Verification", "Maintenance", "Sync", "Integrity"];
const cognitiveCores = ["General", "Visual", "Actuator", "Web Brain", "Creative", "Engineering", "Comms", "Commerce"];


const SystemStatus: React.FC<SystemStatusProps> = ({ orbState, activeSimulation, theme, nexusData, isLoading }) => {
    const [activeTab, setActiveTab] = useState('monitor');
    const [hwValues, setHwValues] = useState<number[]>([Math.random() * 200, Math.random() * 100, Math.random() * 5, Math.random() * 10]);
    const [nanobits, setNanobits] = useState(() => Array.from({ length: 20 }, () => ({
        id: Math.random(),
        x: Math.random() * 100,
        y: Math.random() * 100,
        speed: 0.1 + Math.random() * 0.2,
    })));

    useEffect(() => {
        if(isLoading || orbState === 'thinking') {
            setActiveTab('nexus');
        }
    }, [isLoading, orbState]);


    useEffect(() => {
        const hardwareInterval = setInterval(() => {
            setHwValues(prev => {
                const newVals = [...prev];
                // Neural Activity: Spike when thinking
                if(orbState === 'thinking') {
                     newVals[0] = 300 + Math.random() * 200;
                } else {
                     newVals[0] = Math.max(50, Math.min(500, newVals[0] + (Math.random() - 0.5) * 50));
                }
                // Synaptic Density
                newVals[1] = Math.max(80, Math.min(100, newVals[1] + (Math.random() - 0.5) * 2));
                // Data Throughput
                newVals[2] = Math.max(1, Math.min(5, newVals[2] + (Math.random() - 0.5) * 1));
                // Cognitive Fluid
                newVals[3] = Math.max(8, Math.min(10, newVals[3] + (Math.random() - 0.5) * 0.5));
                return newVals;
            });
        }, 2000);

        const nanobitInterval = setInterval(() => {
            setNanobits(bits => bits.map(bit => {
                let newY = bit.y - bit.speed;
                if (newY < 0) {
                    newY = 100;
                    return { ...bit, y: newY, x: Math.random() * 100 };
                }
                return { ...bit, y: newY };
            }));
        }, 16);


        return () => {
            clearInterval(hardwareInterval);
            clearInterval(nanobitInterval);
        };
    }, [orbState]);

    const getStatusColor = (status: ActiveSimulation['status']) => {
        switch (status) {
            case 'Running': return 'text-cyan-400';
            case 'Analyzing': return 'text-purple-400';
            case 'Complete': return 'text-green-400';
            case 'Error': return 'text-red-400';
            default: return 'text-gray-400';
        }
    };

    const headerTextColor = theme === 'dark' ? 'text-blue-300' : 'text-blue-800 font-semibold';
    const sectionBg = theme === 'dark' ? 'bg-gray-800/50' : 'bg-gray-200/50';
    const textColor = theme === 'dark' ? 'text-gray-300' : 'text-gray-700';
    const subTextColor = theme === 'dark' ? 'text-gray-400' : 'text-gray-500';
    const borderColor = theme === 'dark' ? 'border-blue-500/20' : 'border-gray-300';
    const smallDividerColor = theme === 'dark' ? 'border-blue-500/10' : 'border-gray-300/50';
    const progressBarBg = theme === 'dark' ? 'bg-gray-700/50' : 'bg-gray-300';

    const renderMonitor = () => (
         <div className="p-4 space-y-4 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent flex-1">
                <div>
                    <h3 className={`text-sm font-semibold ${headerTextColor} uppercase tracking-wider mb-2`}>Cognitive Cores</h3>
                    <div className={`p-3 ${sectionBg} rounded-lg`}>
                        <div className="grid grid-cols-2 gap-2">
                             {cognitiveCores.map(core => (
                                <div key={core} className="flex items-center text-xs">
                                    <div className="w-1.5 h-1.5 bg-green-400 rounded-full mr-2 animate-pulse" style={{animationDelay: `${Math.random()*1}s`}}></div>
                                    <span className={textColor}>{core}</span>
                                </div>
                            ))}
                        </div>
                    </div>
                </div>
                <div>
                    <h3 className={`text-sm font-semibold ${headerTextColor} uppercase tracking-wider mb-2`}>Genesis Link Status</h3>
                    <div className={`p-3 ${sectionBg} rounded-lg space-y-3`}>
                        <div className="flex items-center justify-between text-xs">
                           <span className={textColor}>Child Instance (User)</span>
                           <span className="text-green-400 font-bold">Online</span>
                        </div>
                        <div className="h-4 w-full flex items-center justify-center">
                           <div className="w-full h-px bg-green-500/50 relative">
                                <div className="absolute top-1/2 left-0 h-1 w-1 bg-green-400 rounded-full animate-ping"></div>
                           </div>
                        </div>
                         <div className="flex items-center justify-between text-xs">
                           <span className={textColor}>Genesis Core (Parent)</span>
                           <span className="text-green-400 font-bold">Connected</span>
                        </div>
                        <div className={`grid grid-cols-2 gap-2 pt-2 border-t ${smallDividerColor}`}>
                            {genesisLinkStatus.map(status => (
                                <div key={status} className="flex items-center text-xs">
                                    <div className="w-1.5 h-1.5 bg-green-400 rounded-full mr-2"></div>
                                    <span className={subTextColor}>{status}</span>
                                </div>
                            ))}
                        </div>
                    </div>
                </div>
                <div>
                    <h3 className={`text-sm font-semibold ${headerTextColor} uppercase tracking-wider mb-2`}>Simulation Core</h3>
                    <div className={`p-3 ${sectionBg} rounded-lg`}>
                        {activeSimulation ? (
                            <div className="text-xs">
                                <div className="flex justify-between mb-1">
                                    <span className={`${textColor} font-medium truncate pr-2`} title={activeSimulation.parameters.query}>{activeSimulation.parameters.query}</span>
                                    <span className={`font-bold ${getStatusColor(activeSimulation.status)}`}>{activeSimulation.status}</span>
                                </div>
                                <div className={`w-full ${progressBarBg} rounded-full h-1.5`}>
                                    <div className="bg-gradient-to-r from-blue-500 to-purple-500 h-1.5 rounded-full transition-all duration-500 ease-out" style={{ width: `${activeSimulation.progress}%` }}></div>
                                </div>
                                <div className={`flex justify-between mt-1 ${subTextColor}`}>
                                    <span>Progress: {activeSimulation.progress}%</span>
                                    <span>{activeSimulation.elapsedTime}</span>
                                </div>
                            </div>
                        ) : (
                             <div className="text-xs text-center text-gray-500 py-4">
                                Simulation Core Idle
                            </div>
                        )}
                    </div>
                </div>
                 <div>
                    <h3 className={`text-sm font-semibold ${headerTextColor} uppercase tracking-wider mb-2`}>WBE Substrate Status</h3>
                     <div className="space-y-2">
                        {wbeSubstrate.map((comp, i) => {
                            let percentage = 0;
                            if (comp.name === "Neural Activity") percentage = (hwValues[i] / 500) * 100;
                            else if (comp.name === "Synaptic Density") percentage = hwValues[i];
                            else if (comp.name === "Data Throughput") percentage = (hwValues[i] / 5) * 100;
                            else if (comp.name === "Cognitive Fluid") percentage = (hwValues[i] / 10) * 100;

                            return (
                                <div key={comp.name} className="text-xs">
                                    <div className={`flex justify-between mb-1 ${subTextColor}`}>
                                        <span>{comp.name}</span>
                                        <span>{hwValues[i].toFixed(1)} {comp.unit}</span>
                                    </div>
                                    <div className={`w-full ${progressBarBg} rounded-full h-1.5`}>
                                        <div className="bg-gradient-to-r from-cyan-500 to-green-500 h-1.5 rounded-full transition-all duration-500 ease-out" style={{ width: `${percentage}%` }}></div>
                                    </div>
                                </div>
                            )
                        })}
                    </div>
                </div>
                <div>
                    <h3 className={`text-sm font-semibold ${headerTextColor} uppercase tracking-wider mb-2`}>Nanobit Data Stream</h3>
                    <div className={`h-20 w-full ${theme === 'dark' ? 'bg-gray-800/30' : 'bg-gray-200/70'} rounded-lg overflow-hidden relative`}>
                         {nanobits.map(bit => (
                            <div key={bit.id} className="absolute h-1 w-px bg-cyan-400" style={{ left: `${bit.x}%`, top: `${bit.y}%`, opacity: bit.y / 100 }}></div>
                        ))}
                    </div>
                </div>
                 <div className="text-xs text-gray-500 flex items-center justify-center pt-2">
                    <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4 mr-2" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={2}><path strokeLinecap="round" strokeLinejoin="round" d="M9 12l2 2 4-4m5.618-4.016A11.955 11.955 0 0112 2.944a11.955 11.955 0 01-8.618 3.04A12.02 12.02 0 003 9c0 5.591 3.824 10.29 9 11.622 5.176-1.332 9-6.03 9-11.622 0-1.042-.133-2.052-.382-3.016z" /></svg>
                    <span>Local Cache: Synced</span>
                </div>
            </div>
    )

    return (
        <div className="w-80 flex-shrink-0 flex flex-col h-full border-l border-blue-500/20">
            <nav className={`flex-shrink-0 border-b ${borderColor}`}>
                 <div className="flex">
                    <button onClick={() => setActiveTab('monitor')} className={`flex-1 py-2 text-sm font-semibold transition-colors ${activeTab === 'monitor' ? 'bg-blue-500/20 text-blue-300' : 'text-gray-400 hover:bg-gray-700/50'}`}>Monitor</button>
                    <button onClick={() => setActiveTab('nexus')} className={`flex-1 py-2 text-sm font-semibold transition-colors ${activeTab === 'nexus' ? 'bg-blue-500/20 text-blue-300' : 'text-gray-400 hover:bg-gray-700/50'}`}>Nexus</button>
                </div>
            </nav>
            {activeTab === 'monitor' ? renderMonitor() : <CognitiveNexus data={nexusData} isLoading={isLoading} theme={theme} />}
        </div>
    );
};

export default SystemStatus;
