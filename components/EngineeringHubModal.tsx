
import React, { useState } from 'react';

interface EngineeringHubModalProps {
  isOpen: boolean;
  onClose: () => void;
}

const tools = [
    {
        name: "Web App Scaffolder",
        description: "Generate boilerplate code for a web application with your chosen stack.",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M10 20l4-16m4 4l4 4-4 4M6 16l-4-4 4-4" /></svg>,
    },
    {
        name: "Business Model Canvas",
        description: "Generate a business model canvas based on your product idea.",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 17v-2m3 2v-4m3 4v-6m2 10H7a2 2 0 01-2-2V7a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z" /></svg>,
    },
    {
        name: "System Architecture",
        description: "Design a scalable system architecture for your application.",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M8 9l4-4 4 4m0 6l-4 4-4-4" /></svg>,
    },
    {
        name: "Deploy to GitHub",
        description: "Create and deploy a new repository to your GitHub account.",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" viewBox="0 0 24 24" fill="currentColor"><path d="M12 0c-6.626 0-12 5.373-12 12 0 5.302 3.438 9.8 8.207 11.387.599.111.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23.957-.266 1.983-.399 3.003-.404 1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.91 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576 4.765-1.589 8.199-6.086 8.199-11.386 0-6.627-5.373-12-12-12z"/></svg>,
    }
];

const EngineeringHubModal: React.FC<EngineeringHubModalProps> = ({ isOpen, onClose }) => {
  const [activeTool, setActiveTool] = useState<string | null>(null);
  
  const selectedTool = tools.find(t => t.name === activeTool);

  const renderToolInterface = () => {
    if (!selectedTool) return null;
    return (
      <div className="flex flex-col h-full">
        <header className="flex items-center space-x-3 p-4 border-b border-blue-500/10">
          <button onClick={() => setActiveTool(null)} className="p-2 rounded-full hover:bg-gray-700/50">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 19l-7-7 7-7" /></svg>
          </button>
          <div className="text-blue-300">{selectedTool.icon}</div>
          <div>
            <h3 className="font-semibold text-lg text-white">{selectedTool.name}</h3>
          </div>
        </header>
        <div className="flex-1 flex overflow-hidden">
          <div className="flex-1 p-4 flex flex-col">
            <div className="flex-1 bg-black/30 rounded-lg border border-gray-700/50 flex items-center justify-center text-gray-500 relative">
              <div className="absolute inset-0 z-0 opacity-20" style={{ backgroundImage: 'linear-gradient(to right, #1f2937 1px, transparent 1px), linear-gradient(to bottom, #1f2937 1px, transparent 1px)', backgroundSize: '20px 20px'}}></div>
              <span className="z-10">3D Viewport</span>
            </div>
            <form className="mt-4">
              <label className="block text-sm font-medium text-gray-300 mb-1">Prompt</label>
              <textarea rows={3} placeholder={`Describe the ${selectedTool.name.toLowerCase()} you want to generate...`} className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500 resize-none" />
            </form>
          </div>
          <aside className="w-48 p-4 border-l border-blue-500/10">
            <h4 className="font-semibold text-gray-300 mb-3">Tools</h4>
            <div className="space-y-2 text-sm">
                <button className="w-full text-left p-2 rounded-md hover:bg-gray-700/50">Extrude</button>
                <button className="w-full text-left p-2 rounded-md hover:bg-gray-700/50">Bevel</button>
                <button className="w-full text-left p-2 rounded-md hover:bg-gray-700/50">Assemble</button>
                 <button className="w-full text-left p-2 rounded-md hover:bg-gray-700/50">Analyze Stress</button>
            </div>
          </aside>
        </div>
        <footer className="p-4 border-t border-blue-500/10 text-right">
          <button className="bg-blue-600 hover:bg-blue-500 text-white font-bold px-6 py-2 rounded-lg transition-colors">Generate</button>
        </footer>
      </div>
    );
  }

  const renderToolGrid = () => (
    <>
        <p className="text-gray-400 mb-6">Interface with the AI's engineering capabilities. Generate CAD models, control robotics, and design complex systems.</p>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            {tools.map(tool => (
                <button key={tool.name} onClick={() => setActiveTool(tool.name)} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 text-left hover:bg-gray-800 hover:border-blue-500/50 transition-all duration-200">
                    <div className="text-blue-300 mb-2">{tool.icon}</div>
                    <h3 className="font-semibold text-lg text-white mb-1">{tool.name}</h3>
                    <p className="text-sm text-gray-400">{tool.description}</p>
                </button>
            ))}
        </div>
    </>
  );

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-4xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(500px, 80vh, 700px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        {!activeTool &&
            <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
                <div className="flex items-center space-x-3">
                    <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}>
                        <path strokeLinecap="round" strokeLinejoin="round" d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4" />
                    </svg>
                    <h2 className="text-2xl font-bold text-blue-300">CAD & Engineering Suite</h2>
                </div>
                <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
                    <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
                </button>
            </header>
        }
        <main className={`flex-1 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent ${!activeTool ? 'p-6' : ''}`}>
            {activeTool ? renderToolInterface() : renderToolGrid()}
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default EngineeringHubModal;
