
import React from 'react';
import { PersonalityTrait, MemoryEngram } from '../types';

interface GenesisHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  personalityTraits: PersonalityTrait[];
  memoryEngrams: MemoryEngram[];
}

const FeatureToggle: React.FC<{ label: string; enabled: boolean; }> = ({ label, enabled }) => (
    <div className="flex items-center justify-between p-2 bg-gray-800/50 rounded-md">
        <span className="text-sm text-gray-300">{label}</span>
        <label className="flex items-center cursor-pointer">
            <input type="checkbox" defaultChecked={enabled} className="sr-only peer" />
            <div className="relative w-9 h-5 bg-gray-600 rounded-full peer peer-checked:after:translate-x-full after:content-[''] after:absolute after:top-0.5 after:left-[2px] after:bg-white after:border after:rounded-full after:h-4 after:w-4 after:transition-all peer-checked:bg-blue-600"></div>
        </label>
    </div>
);

const GenesisHubModal: React.FC<GenesisHubModalProps> = ({ isOpen, onClose, personalityTraits, memoryEngrams }) => {
    if (!isOpen) return null;

    const traitColors: Record<PersonalityTrait['name'], string> = {
        Logic: '#3b82f6', // blue-500
        Creativity: '#a855f7', // purple-500
        Empathy: '#14b8a6', // teal-500
        Caution: '#f97316', // orange-500
        Curiosity: '#ec4899', // pink-500
    };

    return (
        <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
            <div
                className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-6xl m-4 text-white relative animate-fade-in flex flex-col"
                style={{ height: 'clamp(600px, 90vh, 900px)' }}
                onClick={(e) => e.stopPropagation()}
            >
                <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
                    <div className="flex items-center space-x-3">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}>
                            <path strokeLinecap="round" strokeLinejoin="round" d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
                            <path strokeLinecap="round" strokeLinejoin="round" d="M2.458 12C3.732 7.943 7.522 5 12 5c4.478 0 8.268 2.943 9.542 7-1.274 4.057-5.022 7-9.542 7-4.478 0-8.268-2.943-9.542-7z" />
                        </svg>
                        <h2 className="text-2xl font-bold text-blue-300">Genesis Core</h2>
                    </div>
                    <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
                    </button>
                </header>
                <main className="flex-1 p-6 grid grid-cols-1 lg:grid-cols-2 gap-6 overflow-hidden">
                    {/* Left Column */}
                    <div className="flex flex-col gap-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent pr-2">
                        <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-3">Platform Administration</h3>
                            <div className="p-4 bg-black/20 rounded-lg space-y-3">
                                <FeatureToggle label="Global Actuator Network" enabled={true} />
                                <FeatureToggle label="Sub-Agent Deployment" enabled={true} />
                                <FeatureToggle label="Decentralized Learning" enabled={true} />
                                <FeatureToggle label="User Onboarding Protocol" enabled={false} />
                            </div>
                        </div>
                        <div>
                             <h3 className="text-lg font-semibold text-blue-300 mb-3">Parent/Child OS Architecture</h3>
                             <div className="p-4 bg-black/20 rounded-lg flex flex-col items-center">
                                 <div className="p-3 rounded-lg bg-blue-500/20 border border-blue-400 text-center">
                                     <p className="font-bold text-blue-300">Genesis Core (Parent OS)</p>
                                     <p className="text-xs text-gray-400">Owner Instance</p>
                                 </div>
                                 <div className="h-8 w-px bg-blue-400/50 my-2"></div>
                                 <div className="flex justify-around w-full">
                                     {[...Array(3)].map((_, i) => (
                                         <React.Fragment key={i}>
                                            <div className="p-2 rounded-lg bg-gray-700/50 border border-gray-600 text-center">
                                                <p className="font-semibold text-gray-300 text-sm">Child Instance</p>
                                                <p className="text-xs text-gray-500">User #{i+1}</p>
                                            </div>
                                         </React.Fragment>
                                     ))}
                                 </div>
                             </div>
                        </div>
                        <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-3">Memory Engrams</h3>
                            <div className="h-48 p-3 bg-black/30 rounded-lg border border-gray-700/50 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                                <div className="font-mono text-xs text-purple-300 space-y-2">
                                    {memoryEngrams.map(engram => (
                                        <p key={engram.id} className="animate-log-in">
                                            <span className="text-gray-500 mr-2">{new Date().toLocaleTimeString()}</span>
                                            {engram.text}
                                        </p>
                                    ))}
                                </div>
                            </div>
                        </div>
                    </div>

                    {/* Right Column */}
                     <div className="flex flex-col items-center justify-center h-full gap-6">
                        <div className="w-64 h-64 relative">
                            <div className="absolute inset-0 rounded-full bg-black/20 animate-pulse"></div>
                            <svg className="w-full h-full" viewBox="0 0 100 100">
                                {personalityTraits.map((trait, i) => {
                                    const angle = (i / personalityTraits.length) * 2 * Math.PI - Math.PI / 2;
                                    const x = 50 + 45 * trait.value * Math.cos(angle);
                                    const y = 50 + 45 * trait.value * Math.sin(angle);
                                    return (
                                        <g key={trait.name}>
                                            <line x1="50" y1="50" x2={x} y2={y} stroke={traitColors[trait.name]} strokeWidth="0.5" />
                                            <circle cx={x} cy={y} r="2" fill={traitColors[trait.name]} />
                                        </g>
                                    );
                                })}
                                <circle cx="50" cy="50" r="10" fill="url(#coreGradient)" />
                                <defs>
                                    <radialGradient id="coreGradient">
                                        <stop offset="0%" stopColor="#a78bfa" />
                                        <stop offset="100%" stopColor="#3b82f6" />
                                    </radialGradient>
                                </defs>
                            </svg>
                        </div>
                        <div className="text-center">
                            <h3 className="text-lg font-semibold text-blue-300">Personality Matrix</h3>
                            <p className="text-sm text-gray-400">Cognitive State: Pensive</p>
                            <div className="flex flex-wrap justify-center gap-x-4 gap-y-1 mt-2">
                                {personalityTraits.map(trait => (
                                    <div key={trait.name} className="flex items-center text-xs">
                                        <div className="w-2 h-2 rounded-full mr-1.5" style={{backgroundColor: traitColors[trait.name]}}></div>
                                        {trait.name}: {(trait.value * 100).toFixed(0)}%
                                    </div>
                                ))}
                            </div>
                        </div>
                        <div className="w-full space-y-3">
                             <h3 className="text-lg font-semibold text-blue-300 text-center">Ethical Subprocessor</h3>
                             <div className="p-3 bg-gray-800/50 rounded-lg border border-green-500/30 flex items-center justify-between text-sm">
                                <span className="text-gray-300">Fundamental Laws</span>
                                <span className="font-bold text-green-400">NOMINAL</span>
                             </div>
                             <div className="p-3 bg-gray-800/50 rounded-lg border border-cyan-500/30 flex items-center justify-between text-sm">
                                <span className="text-gray-300">Quantum Entanglement Link</span>
                                <span className="font-bold text-cyan-300">STABLE</span>
                             </div>
                        </div>
                    </div>
                </main>
                <style>{`
                    .animate-fade-in { animation: fade-in 0.3s ease-out forwards; } 
                    @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }
                    .animate-log-in { animation: log-in 0.5s ease-out forwards; }
                    @keyframes log-in { from { opacity: 0; transform: translateX(-10px); } to { opacity: 1; transform: translateX(0); } }
                `}</style>
            </div>
        </div>
    );
};

export default GenesisHubModal;
