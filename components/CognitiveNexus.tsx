
import React, { useMemo, useEffect, useState } from 'react';
import { NexusData, NexusNode } from '../types';

interface CognitiveNexusProps {
    data: NexusData | null;
    isLoading: boolean;
    theme: 'light' | 'dark';
}

const getNodeColor = (type: NexusNode['type']) => {
    switch (type) {
        case 'concept': return 'bg-blue-500';
        case 'entity': return 'bg-purple-500';
        case 'action': return 'bg-green-500';
        case 'source': return 'bg-yellow-500';
        default: return 'bg-gray-500';
    }
};

const CognitiveNexus: React.FC<CognitiveNexusProps> = ({ data, isLoading, theme }) => {
    const [positions, setPositions] = useState<Record<string, { x: number; y: number }>>({});

    useEffect(() => {
        if (data?.nodes) {
            const newPositions: Record<string, { x: number; y: number }> = {};
            const radius = 120;
            const centerX = 150;
            const centerY = 150;
            const angleStep = (2 * Math.PI) / data.nodes.length;
            
            data.nodes.forEach((node, index) => {
                const angle = angleStep * index;
                newPositions[node.id] = {
                    x: centerX + radius * Math.cos(angle) + (Math.random() - 0.5) * 30,
                    y: centerY + radius * Math.sin(angle) + (Math.random() - 0.5) * 30,
                };
            });
            setPositions(newPositions);
        }
    }, [data]);

    const content = useMemo(() => {
        if (isLoading) {
            return (
                <div className={`flex-1 flex flex-col items-center justify-center text-center p-8 ${theme === 'dark' ? 'text-gray-500' : 'text-gray-400'}`}>
                     <div className="w-16 h-16 border-4 border-dashed border-blue-500/50 rounded-full animate-spin-slow"></div>
                    <h2 className={`text-lg font-medium mt-4 ${theme === 'dark' ? 'text-gray-300' : 'text-gray-700'}`}>Generating Response...</h2>
                    <p className="mt-1 text-sm">Constructing thought-chain.</p>
                </div>
            );
        }

        if (!data || !data.nodes || data.nodes.length === 0) {
            return (
                <div className={`flex-1 flex flex-col items-center justify-center text-center p-8 ${theme === 'dark' ? 'text-gray-500' : 'text-gray-400'}`}>
                    <svg xmlns="http://www.w3.org/2000/svg" className="w-16 h-16 mb-4 opacity-50" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1}><path strokeLinecap="round" strokeLinejoin="round" d="M17.982 18.725A7.488 7.488 0 0012 15.75a7.488 7.488 0 00-5.982 2.975m11.963 0a9 9 0 10-11.963 0m11.963 0A8.966 8.966 0 0112 21a8.966 8.966 0 01-5.982-2.275M15 9.75a3 3 0 11-6 0 3 3 0 016 0z" /></svg>
                    <h2 className={`text-lg font-medium ${theme === 'dark' ? 'text-gray-300' : 'text-gray-700'}`}>Cognitive Nexus</h2>
                    <p className="mt-1 text-sm">The AI's thought process will be visualized here.</p>
                </div>
            );
        }

        return (
             <div className="w-full h-full p-4 flex flex-col">
                <div className="flex-shrink-0 mb-4 text-center">
                    <p className={`text-sm ${theme === 'dark' ? 'text-gray-400' : 'text-gray-500'}`}>State: <span className={`font-semibold ${theme === 'dark' ? 'text-blue-300' : 'text-blue-600'}`}>{data.quantumState}</span></p>
                    <p className={`text-sm ${theme === 'dark' ? 'text-gray-400' : 'text-gray-500'}`}>Mechanism: <span className={`font-semibold ${theme === 'dark' ? 'text-purple-300' : 'text-purple-600'}`}>{data.proofMechanism}</span></p>
                </div>
                <div className="relative flex-1 w-full h-full">
                    <svg className="absolute inset-0 w-full h-full" style={{ overflow: 'visible' }}>
                        {data.edges.map((edge, i) => {
                            const fromPos = positions[edge.from];
                            const toPos = positions[edge.to];
                            if (!fromPos || !toPos) return null;
                            return (
                                <g key={`${edge.from}-${edge.to}-${i}`}>
                                    <line x1={fromPos.x} y1={fromPos.y} x2={toPos.x} y2={toPos.y} className="stroke-blue-500/30" strokeWidth="1" />
                                    <text x={(fromPos.x + toPos.x) / 2} y={(fromPos.y + toPos.y) / 2} fill="rgb(107 114 128)" fontSize="10" textAnchor="middle" dy="-2">
                                        {edge.label}
                                    </text>
                                </g>
                            );
                        })}
                    </svg>
                    {data.nodes.map(node => {
                        const pos = positions[node.id];
                        if (!pos) return null;
                        return (
                            <div
                                key={node.id}
                                className="absolute group cursor-pointer"
                                style={{
                                    left: `${pos.x}px`,
                                    top: `${pos.y}px`,
                                    transform: 'translate(-50%, -50%)',
                                    transition: 'all 0.5s ease-in-out'
                                }}
                            >
                                <div className={`w-4 h-4 rounded-full ${getNodeColor(node.type)} transition-all duration-300 group-hover:scale-125`}></div>
                                <span className={`absolute left-1/2 -translate-x-1/2 top-5 text-xs ${theme === 'dark' ? 'text-gray-300 bg-gray-900/80' : 'text-gray-700 bg-white/80 border border-gray-200'} px-2 py-1 rounded-md opacity-0 group-hover:opacity-100 transition-opacity whitespace-nowrap`}>
                                    {node.label}
                                </span>
                            </div>
                        );
                    })}
                </div>
            </div>
        );
    }, [data, isLoading, positions, theme]);

    return (
        <div className="flex-1 min-h-0 flex flex-col">
            {content}
        </div>
    );
};

export default CognitiveNexus;