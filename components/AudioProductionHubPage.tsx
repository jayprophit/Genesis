
import React from 'react';

const AudioProductionHubPage: React.FC = () => {
    
    return (
        <div className="w-full h-full flex flex-col bg-gray-900 text-gray-300 overflow-hidden">
             {/* Transport Controls */}
            <div className="flex-shrink-0 p-2 border-b border-blue-500/20 flex items-center space-x-2 bg-gray-800/50 justify-center">
                <button className="p-2 hover:bg-gray-700 rounded-md"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path d="M8.445 14.832A1 1 0 0010 14.168V5.832a1 1 0 00-1.555-.832L4.5 8.168a1 1 0 000 1.664l3.945 3.168zM11 5v10a1 1 0 001.555.832l4.5-5a1 1 0 000-1.664l-4.5-5A1 1 0 0011 5z" /></svg></button>
                <button className="p-2 hover:bg-gray-700 rounded-md"><svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8 text-blue-400" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM9.555 7.168A1 1 0 008 8v4a1 1 0 001.555.832l3-2a1 1 0 000-1.664l-3-2z" clipRule="evenodd" /></svg></button>
                <button className="p-2 hover:bg-gray-700 rounded-md"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM8 7a1 1 0 00-1 1v4a1 1 0 102 0V8a1 1 0 00-1-1zm4 0a1 1 0 00-1 1v4a1 1 0 102 0V8a1 1 0 00-1-1z" clipRule="evenodd" /></svg></button>
                <button className="p-2 hover:bg-gray-700 rounded-md"><svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6 text-red-500" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16z" clipRule="evenodd" /></svg></button>
            </div>

            <div className="flex-1 flex flex-col overflow-hidden">
                {/* Timeline */}
                <div className="h-32 flex-shrink-0 border-b border-blue-500/20 overflow-x-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                     <div className="w-[1200px] h-full relative p-2">
                        {/* Ruler */}
                        <div className="h-6 flex text-xs text-gray-500">
                             {[...Array(12)].map((_,i) => <div key={i} className="w-[100px] border-l border-gray-700 pl-1">{i+1}</div>)}
                        </div>
                        {/* Tracks */}
                        <div className="space-y-1 mt-1">
                            <div className="h-10 bg-gray-800 rounded-md flex items-center"><div className="w-40 h-8 ml-24 bg-purple-500/50 rounded"></div></div>
                            <div className="h-10 bg-gray-800 rounded-md flex items-center"><div className="w-64 h-8 ml-48 bg-cyan-500/50 rounded"></div></div>
                        </div>
                     </div>
                </div>

                {/* Mixer */}
                <div className="flex-1 flex p-4 gap-4 bg-gray-800/30">
                    {['Drums', 'Bass', 'Synth', 'Vocals', 'Master'].map(track => (
                        <div key={track} className="w-24 p-2 bg-gray-800 rounded-lg flex flex-col items-center">
                            <div className="h-40 w-2 bg-gray-900 rounded-full flex items-end">
                                <div className="w-full bg-green-500 rounded-full" style={{height: `${Math.random() * 80 + 10}%`}}></div>
                            </div>
                            <p className="text-xs mt-2">{track}</p>
                        </div>
                    ))}
                </div>
            </div>
        </div>
    );
};

export default AudioProductionHubPage;
