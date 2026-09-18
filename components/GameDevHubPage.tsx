
import React from 'react';

const GameDevHubPage: React.FC = () => {

    return (
        <div className="w-full h-full flex flex-col bg-gray-900 text-gray-300 overflow-hidden">
             {/* Toolbar */}
            <div className="flex-shrink-0 p-2 border-b border-blue-500/20 flex items-center space-x-2 bg-gray-800/50">
                <button className="p-2 hover:bg-gray-700 rounded-md" title="Play"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM9.555 7.168A1 1 0 008 8v4a1 1 0 001.555.832l3-2a1 1 0 000-1.664l-3-2z" clipRule="evenodd" /></svg></button>
                <button className="p-2 hover:bg-gray-700 rounded-md" title="Pause"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM9 6a1 1 0 00-1 1v6a1 1 0 102 0V7a1 1 0 00-1-1zm2 1a1 1 0 011-1h.008a1 1 0 110 2H12a1 1 0 01-1-1z" clipRule="evenodd" /></svg></button>
            </div>

            <div className="flex-1 flex overflow-hidden">
                {/* Hierarchy Panel */}
                <aside className="w-64 p-4 border-r border-blue-500/20 flex flex-col space-y-2 text-sm">
                    <h3 className="font-semibold text-blue-300 mb-2">Scene</h3>
                    <div><span className="font-bold">▶ World</span></div>
                    <div className="pl-4"><span className="text-gray-400">▷ Lighting</span></div>
                    <div className="pl-4"><span className="text-gray-400">▼ Player</span>
                        <div className="pl-8"><span className="text-gray-500">- Player_Mesh</span></div>
                        <div className="pl-8"><span className="text-gray-500">- Main_Camera</span></div>
                    </div>
                     <div className="pl-4"><span className="text-gray-400">▷ Terrain</span></div>
                </aside>

                 {/* Viewport */}
                <main className="flex-1 flex flex-col items-center justify-center bg-black relative">
                    <div className="absolute inset-0 z-0 opacity-20" style={{ backgroundImage: 'linear-gradient(to right, #1f2937 1px, transparent 1px), linear-gradient(to bottom, #1f2937 1px, transparent 1px)', backgroundSize: '40px 40px'}}></div>
                    <p className="text-gray-600">3D Scene Viewport</p>
                </main>

                {/* Inspector Panel */}
                 <aside className="w-80 p-4 border-l border-blue-500/20 flex flex-col space-y-4 text-sm">
                     <h3 className="font-semibold text-blue-300 mb-2">Inspector: Player</h3>
                     <div className="p-2 bg-gray-800/50 rounded-md">
                        <p className="font-semibold">Transform</p>
                        <div className="grid grid-cols-4 gap-2 mt-2 text-xs">
                           <span></span><span>X</span><span>Y</span><span>Z</span>
                           <span className="text-gray-400">Position</span><span>0</span><span>1</span><span>0</span>
                           <span className="text-gray-400">Rotation</span><span>0</span><span>90</span><span>0</span>
                           <span className="text-gray-400">Scale</span><span>1</span><span>1</span><span>1</span>
                        </div>
                     </div>
                     <div className="p-2 bg-gray-800/50 rounded-md">
                        <p className="font-semibold">Player Controller (Script)</p>
                     </div>
                 </aside>
            </div>
        </div>
    );
};

export default GameDevHubPage;
