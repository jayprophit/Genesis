

import React, { useState, useMemo } from 'react';
import { aiToolCategories } from '../aiTools.config';
import { AITool } from '../types';
import { generateOneOffResponse } from '../services/geminiService';

interface AIToolsHubPageProps {
  onToolSubmit: (prompt: string, title: string) => void;
}

const AIToolsHubPage: React.FC<AIToolsHubPageProps> = ({ onToolSubmit }) => {
    const [selectedCategory, setSelectedCategory] = useState<string | null>(null);
    const [selectedTool, setSelectedTool] = useState<AITool | null>(null);
    const [userInput, setUserInput] = useState('');
    const [isLoading, setIsLoading] = useState(false);
    const [result, setResult] = useState<string | null>(null);

    const handleSelectCategory = (categoryId: string) => {
        setSelectedCategory(prev => prev === categoryId ? null : categoryId);
        setSelectedTool(null);
        setResult(null);
    };

    const handleSelectTool = (tool: AITool) => {
        setSelectedTool(tool);
        setUserInput('');
        setResult(null);
        setSelectedCategory(aiToolCategories.find(c => c.tools.some(t => t.id === tool.id))?.id || null);
    }

    const handleGenerate = async () => {
        if (!selectedTool || !userInput.trim()) return;
        setIsLoading(true);
        setResult(null);
        try {
            const fullPrompt = selectedTool.prompt.replace('...', userInput);
            const response = await generateOneOffResponse(fullPrompt);
            setResult(response.text);
        } catch (error) {
            console.error("Error generating AI tool response:", error);
            setResult("Sorry, an error occurred while processing your request.");
        } finally {
            setIsLoading(false);
        }
    };
    
    const allTools = useMemo(() => aiToolCategories.flatMap(c => c.tools.map(t => ({ ...t, categoryId: c.id }))), []);

    return (
        <div className="w-full h-full flex flex-col overflow-hidden">
            <main className="flex-1 flex flex-col md:flex-row overflow-hidden">
                <div className="flex-1 p-6 relative flex items-center justify-center overflow-hidden">
                    {/* Tool Galaxy */}
                    <div className="absolute inset-0 animate-spin-slow" style={{ animationDuration: '120s' }}>
                        {allTools.map((tool, i) => {
                            const angle = (i / allTools.length) * 2 * Math.PI;
                            const radius = 150 + Math.floor(i / 8) * 60 + (i % 2) * 20;
                            const x = 50 + (radius / 3.5) * Math.cos(angle);
                            const y = 50 + (radius / 3.5) * Math.sin(angle);
                            const isSelected = selectedTool?.id === tool.id || (selectedCategory && tool.categoryId === selectedCategory);
                            return (
                                <button 
                                    key={tool.id} 
                                    onClick={() => handleSelectTool(tool)}
                                    className={`absolute w-12 h-12 rounded-full bg-gray-800/80 border-2 transition-all duration-300 flex items-center justify-center text-xs text-center p-1 font-semibold ${isSelected ? 'border-blue-400 scale-110 shadow-lg shadow-blue-500/50' : 'border-gray-700 hover:border-blue-500'}`}
                                    style={{ left: `${x}%`, top: `${y}%`, transform: 'translate(-50%, -50%)' }}
                                    title={tool.name}
                                >
                                    <span className={isSelected ? 'text-blue-300' : 'text-gray-400'}>{tool.name.split(' ').map(w => w[0]).join('')}</span>
                                </button>
                            );
                        })}
                    </div>
                    
                    {/* Central Wheel */}
                    <div className="relative w-72 h-72">
                         {aiToolCategories.map((cat, i) => {
                            const angle = 360 / aiToolCategories.length;
                            return (
                                <div 
                                    key={cat.id} 
                                    className="absolute w-1/2 h-1/2 origin-bottom-right group cursor-pointer"
                                    style={{ transform: `rotate(${i * angle}deg)`, clipPath: `polygon(0 0, 100% 0, 100% 100%, 0 0)` }}
                                    onClick={() => handleSelectCategory(cat.id)}
                                >
                                     <div 
                                        className="absolute inset-[-100%] bg-gray-800/50 border border-blue-500/10 transition-all duration-300 group-hover:bg-blue-900/50"
                                        style={{ 
                                            transform: `skewX(${90 - angle}deg)`,
                                            backgroundColor: selectedCategory === cat.id ? 'rgba(59, 130, 246, 0.4)' : ''
                                        }}
                                    >
                                        <div className="absolute w-full h-full text-center text-sm font-semibold text-gray-300 transition-colors group-hover:text-white" style={{transform: `skewX(-${90 - angle}deg) rotate(${angle / 2}deg) translate(80px) rotate(-${i * angle + angle/2}deg)`}}>
                                            {cat.name}
                                        </div>
                                    </div>
                                </div>
                            )
                         })}
                         <div className="absolute inset-1/4 rounded-full bg-gray-900 border-2 border-blue-500/20 flex items-center justify-center p-2 text-center text-sm text-gray-400">
                            Select a Category
                         </div>
                    </div>

                </div>
                <aside className="w-full md:w-[450px] bg-gray-900/50 border-t md:border-t-0 md:border-l border-blue-500/20 flex flex-col overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                    {selectedTool ? (
                        <div className="flex flex-col h-full">
                            <div className="p-6">
                                <h3 className="text-xl font-bold text-blue-300">{selectedTool.name}</h3>
                                <p className="text-gray-400 mt-2 mb-4">{selectedTool.description}</p>
                                <div className="p-4 bg-gray-800/50 rounded-lg">
                                    <p className="text-sm font-semibold text-gray-300 mb-2">Example Usage:</p>
                                    <p className="font-mono text-sm text-cyan-300 bg-black/30 p-2 rounded-md">"{selectedTool.prompt}"</p>
                                </div>
                            </div>
                            <div className="flex-1 p-6 pt-0 flex flex-col">
                                <label className="block text-sm font-medium text-gray-300 mb-2">Your Prompt</label>
                                <textarea 
                                    value={userInput}
                                    onChange={(e) => setUserInput(e.target.value)}
                                    placeholder={selectedTool.prompt.split('...')[1] || "Enter your details here..."}
                                    className="w-full flex-1 bg-gray-800 border-gray-700 rounded-lg p-3 resize-none"
                                />
                                <button onClick={handleGenerate} disabled={isLoading || !userInput.trim()} className="w-full mt-4 p-3 bg-blue-600 hover:bg-blue-500 rounded-md font-semibold transition-colors disabled:opacity-50">
                                    {isLoading ? 'Generating...' : 'Generate'}
                                </button>
                            </div>
                            {result && (
                                <div className="p-6 border-t border-blue-500/20 bg-black/20">
                                    <h4 className="font-semibold text-green-300 mb-2">Result</h4>
                                    <div className="max-h-48 overflow-y-auto p-3 bg-gray-800 rounded-md text-sm whitespace-pre-wrap">{result}</div>
                                    <button onClick={() => onToolSubmit(selectedTool.prompt.replace('...', userInput), selectedTool.name)} className="w-full mt-4 p-2 text-sm bg-gray-700 hover:bg-gray-600 rounded-md font-semibold transition-colors">Continue in Chat</button>
                                </div>
                            )}
                        </div>
                    ) : (
                        <div className="text-center text-gray-500 flex flex-col items-center justify-center h-full p-6">
                            <svg xmlns="http://www.w3.org/2000/svg" className="h-12 w-12 mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1}><path strokeLinecap="round" strokeLinejoin="round" d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" /></svg>
                            <h3 className="font-semibold text-gray-400">Select a Tool or Category</h3>
                            <p className="text-sm mt-1">Choose a category from the central wheel or click on a tool in the galaxy to see its details.</p>
                        </div>
                    )}
                </aside>
            </main>
        </div>
    );
};

export default AIToolsHubPage;
