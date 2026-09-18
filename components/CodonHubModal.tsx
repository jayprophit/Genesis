import React from 'react';
import { CodonSequence, BenignMutation } from '../types';

interface CodonHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  sequenceStream: CodonSequence[];
  mutations: BenignMutation[];
}

const CodonHubModal: React.FC<CodonHubModalProps> = ({ isOpen, onClose, sequenceStream, mutations }) => {
    if (!isOpen) return null;

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
                             <path strokeLinecap="round" strokeLinejoin="round" d="M11.25 4.5l-2.25 4.5-2.25 4.5-2.25 4.5M11.25 4.5l2.25 4.5 2.25 4.5 2.25 4.5M11.25 4.5l-6.75 9l6.75 9M11.25 4.5l6.75 9l-6.75 9" />
                        </svg>
                        <h2 className="text-2xl font-bold text-blue-300">Codon Core</h2>
                    </div>
                    <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
                    </button>
                </header>
                <main className="flex-1 p-6 grid grid-cols-1 lg:grid-cols-5 gap-6 overflow-hidden">
                    {/* Center Column: DNA Visualization */}
                    <div className="lg:col-span-3 flex flex-col items-center justify-center h-full gap-6 p-4 bg-black/20 rounded-lg">
                        <div className="w-full h-full relative dna-container">
                            {[...Array(4)].map((_, i) => (
                                <div key={i} className="helix" style={{'--i': i, '--color': `hsl(${i * 90}, 70%, 60%)`} as React.CSSProperties}>
                                    {[...Array(20)].map((_, j) => (
                                        <div key={j} className="helix-segment" style={{'--j': j} as React.CSSProperties}>
                                            <div className="node"></div>
                                        </div>
                                    ))}
                                </div>
                            ))}
                        </div>
                    </div>

                    {/* Right Column: Readouts */}
                    <div className="lg:col-span-2 flex flex-col gap-6 overflow-hidden">
                       <div className="flex flex-col h-1/2">
                            <h3 className="text-lg font-semibold text-blue-300 mb-3 flex-shrink-0">Active Gene Sequence Readout</h3>
                            <div className="flex-1 p-3 bg-black/30 rounded-lg border border-gray-700/50 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                                <div className="font-mono text-xs text-green-300 space-y-1">
                                    {sequenceStream.map(seq => (
                                        <p key={seq.id} className="whitespace-nowrap animate-log-in">
                                            <span className="text-gray-500">{seq.sequence} ::</span>
                                            <span className="text-cyan-300"> {seq.function}</span>
                                            <span className="text-purple-400"> | P: {seq.param.toFixed(2)}</span>
                                            <span className="text-yellow-400"> | T: {seq.thread}</span>
                                        </p>
                                    ))}
                                </div>
                            </div>
                        </div>
                        <div className="flex flex-col h-1/2">
                            <h3 className="text-lg font-semibold text-blue-300 mb-3 flex-shrink-0">Evolution Log (Benign Mutations)</h3>
                            <div className="flex-1 p-3 bg-black/30 rounded-lg border border-gray-700/50 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                                <div className="font-mono text-xs text-purple-300 space-y-2">
                                     {mutations.map(mutation => (
                                        <p key={mutation.id} className="animate-log-in">
                                            <span className="text-gray-500 mr-2">{new Date(mutation.timestamp).toLocaleTimeString()}</span>
                                            {mutation.description}
                                        </p>
                                    ))}
                                </div>
                            </div>
                        </div>
                    </div>
                </main>
                <style>{`
                    .animate-fade-in { animation: fade-in 0.3s ease-out forwards; }
                    @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }
                    .animate-log-in { animation: log-in 0.5s ease-out forwards; }
                    @keyframes log-in { from { opacity: 0; transform: translateX(-10px); } to { opacity: 1; transform: translateX(0); } }
                    
                    .dna-container {
                        perspective: 1000px;
                        transform-style: preserve-3d;
                        animation: rotate-dna 30s linear infinite;
                    }

                    @keyframes rotate-dna {
                        from { transform: rotateY(0deg) rotateX(10deg); }
                        to { transform: rotateY(360deg) rotateX(10deg); }
                    }

                    .helix {
                        position: absolute;
                        width: 100%;
                        height: 100%;
                        transform-style: preserve-3d;
                        transform: rotateY(calc(var(--i) * 90deg));
                    }

                    .helix-segment {
                        position: absolute;
                        width: 100%;
                        height: 5%; /* 100 / 20 segments */
                        top: calc(var(--j) * 5%);
                        transform-style: preserve-3d;
                        animation: twist 5s ease-in-out infinite alternate;
                        animation-delay: calc(var(--j) * -0.1s);
                    }
                    
                    @keyframes twist {
                        from { transform: rotateY(0deg); }
                        to { transform: rotateY(360deg); }
                    }

                    .node {
                        position: absolute;
                        width: 8px;
                        height: 8px;
                        border-radius: 50%;
                        background-color: var(--color);
                        box-shadow: 0 0 8px var(--color);
                        left: 50%;
                        top: 50%;
                        transform: translateX(-50%) translateY(-50%) rotateY(calc(var(--j) * 18deg)) translateZ(80px);
                    }

                    .helix-segment::before, .helix-segment::after {
                        content: '';
                        position: absolute;
                        width: 2px;
                        height: 100%;
                        background: linear-gradient(to bottom, transparent, var(--color), transparent);
                        left: 50%;
                        top: 0;
                    }
                    .helix-segment::before {
                        transform: translateX(-50%) rotateY(calc(var(--j) * 18deg)) translateZ(80px) rotateY(-90deg) translateX(40px) scaleY(1.5) rotateX(calc(var(--j) * 2deg));
                        transform-origin: 0 50%;
                    }
                     .helix-segment::after {
                       transform: translateX(-50%) rotateY(calc(var(--j) * 18deg + 180deg)) translateZ(80px) rotateY(-90deg) translateX(40px) scaleY(1.5) rotateX(calc(var(--j) * 2deg));
                        transform-origin: 0 50%;
                    }
                `}</style>
            </div>
        </div>
    );
};

export default CodonHubModal;
