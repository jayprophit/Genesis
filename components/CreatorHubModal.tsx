import React, { useState } from 'react';

interface CreatorHubModalProps {
  isOpen: boolean;
  onClose: () => void;
}

const tools = [
    {
        name: "Write 1st Draft",
        description: "Generate a first draft for any document, from essays to reports.",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z" /></svg>,
    },
    {
        name: "Generate a Script",
        description: "Create a screenplay, video script, or podcast outline.",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M7 4v16M17 4v16M3 8h4m10 0h4M3 12h18M3 16h4m10 0h4" /></svg>,
    },
    {
        name: "AI Trip Planner",
        description: "Plan your next vacation with a detailed, personalized itinerary.",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 20l-5.447-2.724A1 1 0 013 16.382V8.618a1 1 0 01.553-.894L9 4m0 16v-5h6v5m6-5v-5a2 2 0 00-2-2h-1.586a1 1 0 01-.707-.293l-1.121-1.121A2 2 0 0013.172 6H10.828a2 2 0 00-1.414.586L8.293 7.707A1 1 0 017.586 8H6a2 2 0 00-2 2v5" /></svg>,
    },
    {
        name: "Design Slides",
        description: "Automatically generate a slide deck for your presentation.",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 12H5m14 0l-4 4m4-4l-4-4" /></svg>,
    },
    {
        name: "Generate Meme",
        description: "Create a viral meme with a caption and image.",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M14.828 14.828a4 4 0 01-5.656 0M9 10h.01M15 10h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>,
    },
    {
        name: "Recipe Generator",
        description: "Generate creative recipes based on ingredients you have.",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M20 13V6a2 2 0 00-2-2H6a2 2 0 00-2 2v7m16 0v5a2 2 0 01-2 2H6a2 2 0 01-2-2v-5m16 0h-2.586a1 1 0 00-.707.293l-2.414 2.414a1 1 0 01-.707.293h-3.172a1 1 0 01-.707-.293l-2.414-2.414A1 1 0 006.586 13H4" /></svg>,
    }
];

const CreatorHubModal: React.FC<CreatorHubModalProps> = ({ isOpen, onClose }) => {
  const [activeTool, setActiveTool] = useState<string | null>(null);

  const selectedTool = tools.find(t => t.name === activeTool);
  
  const handleClose = () => {
    setActiveTool(null);
    onClose();
  };
  
  const renderToolInterface = () => {
    if (!selectedTool) return null;
    return (
        <div className="flex flex-col h-full">
            <header className="flex items-center space-x-3 p-4 border-b border-blue-500/10">
                <button onClick={() => setActiveTool(null)} className="p-2 rounded-full hover:bg-gray-700/50"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 19l-7-7 7-7" /></svg></button>
                <div className="text-blue-300">{selectedTool.icon}</div>
                <div>
                    <h3 className="font-semibold text-lg text-white">{selectedTool.name}</h3>
                    <p className="text-sm text-gray-400">{selectedTool.description}</p>
                </div>
            </header>
            <div className="flex-1 p-4 space-y-4">
                 <div>
                    <label className="block text-sm font-medium text-gray-300 mb-1">Prompt</label>
                    <textarea rows={8} placeholder={`Describe the ${selectedTool.name.toLowerCase()} you want to generate...`} className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500 resize-none" />
                </div>
            </div>
            <footer className="p-4 border-t border-blue-500/10 text-right">
                <button className="bg-blue-600 hover:bg-blue-500 text-white font-bold px-6 py-2 rounded-lg transition-colors">Generate</button>
            </footer>
        </div>
    );
  }

  const renderToolGrid = () => (
      <>
        <p className="text-gray-400 mb-6">Instruct the AI to build anything, from digital platforms to physical technologies. Provide a clear description and a deployment target.</p>
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
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
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={handleClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-4xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(500px, 90vh, 750px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <div className="flex items-center space-x-3">
             <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}>
                <path strokeLinecap="round" strokeLinejoin="round" d="M5 3v4M3 5h4M6 17v4m-2-2h4m5-16l2.286 6.857L21 12l-5.714 2.143L13 21l-2.286-6.857L5 12l5.714-2.143L13 3z" />
             </svg>
            <h2 className="text-2xl font-bold text-blue-300">Creator Core</h2>
          </div>
          <button onClick={handleClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            {activeTool ? renderToolInterface() : renderToolGrid()}
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default CreatorHubModal;