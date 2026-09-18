
import React, { useState, useRef, useEffect, MouseEvent } from 'react';
import { HealingWebNode, HealingWebLink } from '../types';
import { getHealingWebExplanation } from '../services/geminiService';

const webData = {
    nodes: [
        { id: 'root', label: 'The Healing Web', group: 'core', x: 400, y: 300 },
        { id: 'symptoms', label: 'Treat the Symptoms', group: 'western', x: 200, y: 150 },
        { id: 'root_cause', label: 'Cure the Cause', group: 'holistic', x: 600, y: 150 },
        { id: 'big_pharma', label: 'Big Pharma', group: 'western', x: 100, y: 250 },
        { id: 'holistic', label: 'Holistic Healing', group: 'holistic', x: 700, y: 250 },
        { id: 'fast_food', label: 'Fast Food', group: 'western', x: 150, y: 450 },
        { id: 'natural_news', label: 'Natural News', group: 'holistic', x: 650, y: 450 },
        { id: 'gut_health', label: 'Gut Health', group: 'holistic', x: 550, y: 350 },
        { id: 'stress', label: 'Stress', group: 'western', x: 250, y: 350 },
    ],
    links: [
        { source: 'root', target: 'symptoms' },
        { source: 'root', target: 'root_cause' },
        { source: 'symptoms', target: 'big_pharma' },
        { source: 'root_cause', target: 'holistic' },
        { source: 'big_pharma', target: 'fast_food' },
        { source: 'holistic', target: 'natural_news' },
        { source: 'root_cause', target: 'gut_health' },
        { source: 'symptoms', target: 'stress' },
        { source: 'stress', target: 'gut_health' },
    ]
};

const nodeColors: Record<string, string> = {
    core: '#a855f7', // purple
    western: '#ef4444', // red
    holistic: '#22c55e', // green
};

const renderMarkdown = (text: string) => {
    let html = text
        .replace(/\*\*(.*?)\*\*/g, '<strong>$1</strong>')
        .replace(/\*(.*?)\*/g, '<em>$1</em>')
        .replace(/(\d+\.\s.*)/g, '<p>$1</p>')
        .replace(/\n/g, '<br />');
    html = html.split('<br /><br />').map(p => `<p class="mb-2">${p}</p>`).join('');
    return { __html: html };
};

const InteractiveHealingWeb: React.FC = () => {
    const [viewBox, setViewBox] = useState('0 0 800 600');
    const [selectedNode, setSelectedNode] = useState<HealingWebNode | null>(null);
    const [explanation, setExplanation] = useState<string | null>(null);
    const [isLoading, setIsLoading] = useState(false);
    const svgRef = useRef<SVGSVGElement>(null);
    const isDragging = useRef(false);
    const lastPos = useRef({ x: 0, y: 0 });

    const handleNodeClick = async (node: HealingWebNode) => {
        setSelectedNode(node);
        setIsLoading(true);
        setExplanation(null);
        try {
            const result = await getHealingWebExplanation(node.label);
            setExplanation(result);
        } catch (error) {
            setExplanation("Failed to retrieve explanation.");
        } finally {
            setIsLoading(false);
        }
    };

    const handleMouseDown = (e: MouseEvent<SVGSVGElement>) => {
        isDragging.current = true;
        lastPos.current = { x: e.clientX, y: e.clientY };
    };

    const handleMouseUp = () => {
        isDragging.current = false;
    };

    const handleMouseMove = (e: MouseEvent<SVGSVGElement>) => {
        if (!isDragging.current) return;
        const [vx, vy, vw, vh] = viewBox.split(' ').map(Number);
        const dx = e.clientX - lastPos.current.x;
        const dy = e.clientY - lastPos.current.y;
        setViewBox(`${vx - dx} ${vy - dy} ${vw} ${vh}`);
        lastPos.current = { x: e.clientX, y: e.clientY };
    };
    
    return (
        <div className="w-full h-full flex flex-col md:flex-row bg-black/30 rounded-lg overflow-hidden">
            <div className="flex-1 relative cursor-grab active:cursor-grabbing">
                <svg
                    ref={svgRef}
                    className="w-full h-full"
                    viewBox={viewBox}
                    onMouseDown={handleMouseDown}
                    onMouseUp={handleMouseUp}
                    onMouseLeave={handleMouseUp}
                    onMouseMove={handleMouseMove}
                >
                    {webData.links.map((link, i) => {
                        const sourceNode = webData.nodes.find(n => n.id === link.source);
                        const targetNode = webData.nodes.find(n => n.id === link.target);
                        if (!sourceNode || !targetNode) return null;
                        return (
                            <line
                                key={i}
                                x1={sourceNode.x} y1={sourceNode.y}
                                x2={targetNode.x} y2={targetNode.y}
                                stroke="rgba(59, 130, 246, 0.3)"
                                strokeWidth="1"
                            />
                        );
                    })}
                    {webData.nodes.map(node => (
                        <g key={node.id} transform={`translate(${node.x}, ${node.y})`} className="cursor-pointer group" onClick={() => handleNodeClick(node)}>
                            <circle r="10" fill={nodeColors[node.group]} className="transition-all duration-300 group-hover:r-12" />
                            <circle r="8" fill={nodeColors[node.group]} stroke="#111827" strokeWidth="2" />
                            <text
                                textAnchor="middle"
                                dy="4"
                                fill="white"
                                fontSize="8"
                                className="font-semibold pointer-events-none"
                            >
                                {node.label.split(' ').map(w => w[0]).join('')}
                            </text>
                            <text
                                textAnchor="middle"
                                dy="25"
                                fill="white"
                                fontSize="10"
                                className="font-bold pointer-events-none opacity-0 group-hover:opacity-100 transition-opacity"
                            >
                                {node.label}
                            </text>
                        </g>
                    ))}
                </svg>
            </div>
            <aside className="w-full md:w-96 p-4 border-t md:border-t-0 md:border-l border-blue-500/20 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                {selectedNode ? (
                    <div>
                        <h3 className="text-xl font-bold mb-4" style={{color: nodeColors[selectedNode.group]}}>
                            {selectedNode.label}
                        </h3>
                        {isLoading ? (
                             <div className="flex items-center justify-center h-48">
                                <div className="w-8 h-8 border-4 border-dashed border-blue-500/50 rounded-full animate-spin-slow"></div>
                            </div>
                        ) : (
                            <div className="prose prose-sm prose-invert" dangerouslySetInnerHTML={explanation ? renderMarkdown(explanation) : {__html: ''}}></div>
                        )}
                    </div>
                ) : (
                     <div className="text-center text-gray-500 h-full flex flex-col items-center justify-center">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-12 w-12 mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1} d="M15.59 14.37a6 6 0 01-5.84 7.38v-4.82m5.84-2.56a17.96 17.96 0 00-5.84-2.56m0 0A17.965 17.965 0 0112 2.25c-2.996 0-5.74-1.1-7.75-2.922m9.5 14.25a17.966 17.966 0 01-9.5-14.25" /></svg>
                        <h3 className="font-semibold text-gray-400">The Healing Web</h3>
                        <p className="text-sm mt-1">Click on a node in the web to learn more about it.</p>
                    </div>
                )}
            </aside>
        </div>
    );
};

export default InteractiveHealingWeb;
