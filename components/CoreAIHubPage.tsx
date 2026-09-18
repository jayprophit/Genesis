
import React from 'react';
import { PersonalityTrait, MemoryEngram } from '../types';

interface CoreAIHubPageProps {
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

const CoreAIHubPage: React.FC<CoreAIHubPageProps> = ({ personalityTraits, memoryEngrams }) => {
    const traitColors: Record<PersonalityTrait['name'], string> = {
        Logic: '#3b82f6', // blue-500
        Creativity: '#a855f7', // purple-500
        Empathy: '#14b8a6', // teal-500
        Caution: '#f97316', // orange-500
        Curiosity: '#ec4899', // pink-500
    };

    return (
        <main className="w-full h-full p-6 grid grid-cols-1 lg:grid-cols-2 gap-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            {/* Left Column */}
            <div className="flex flex-col gap-6">
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
             <style>{`
                .animate-log-in { animation: log-in 0.5s ease-out forwards; }
                @keyframes log-in { from { opacity: 0; transform: translateX(-10px); } to { opacity: 1; transform: translateX(0); } }
            `}</style>
        </main>
    );
};

export default CoreAIHubPage;
