
import React from 'react';

const VideoEditingHubPage: React.FC = () => {

    return (
        <div className="w-full h-full flex flex-col bg-gray-900 text-gray-300 overflow-hidden">
            {/* Toolbar */}
            <div className="flex-shrink-0 p-2 border-b border-blue-500/20 flex items-center space-x-2 bg-gray-800/50">
                <button className="p-2 hover:bg-gray-700 rounded-md"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path d="M10 12.5a.5.5 0 01-.5-.5v-5a.5.5 0 011 0v5a.5.5 0 01-.5.5z" /><path d="M10 2a8 8 0 100 16 8 8 0 000-16zM2 10a8 8 0 1116 0 8 8 0 01-16 0z" /></svg></button>
                <button className="p-2 hover:bg-gray-700 rounded-md"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path d="M8 3a1 1 0 011-1h2a1 1 0 110 2H9a1 1 0 01-1-1z" /><path d="M6 3a2 2 0 00-2 2v11a2 2 0 002 2h8a2 2 0 002-2V5a2 2 0 00-2-2 3 3 0 01-3 3H9a3 3 0 01-3-3z" /></svg></button>
                <div className="h-6 w-px bg-gray-700"></div>
                <button className="p-2 hover:bg-gray-700 rounded-md"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.707 7.293a1 1 0 00-1.414 1.414L8.586 10l-1.293 1.293a1 1 0 101.414 1.414L10 11.414l1.293 1.293a1 1 0 001.414-1.414L11.414 10l1.293-1.293a1 1 0 00-1.414-1.414L10 8.586 8.707 7.293z" clipRule="evenodd" /></svg></button>
            </div>
            
            <div className="flex-1 flex overflow-hidden">
                {/* Left Panel (Media, Effects) */}
                <aside className="w-64 p-4 border-r border-blue-500/20 flex flex-col space-y-4">
                     <h3 className="font-semibold text-blue-300">Media Bin</h3>
                     <div className="grid grid-cols-2 gap-2">
                        <div className="h-20 bg-gray-800 rounded-md flex items-center justify-center text-xs text-gray-500">Video_01.mp4</div>
                        <div className="h-20 bg-gray-800 rounded-md flex items-center justify-center text-xs text-gray-500">Video_02.mp4</div>
                        <div className="h-20 bg-gray-800 rounded-md flex items-center justify-center text-xs text-gray-500">Audio_Track.wav</div>
                     </div>
                     <h3 className="font-semibold text-blue-300">Effects</h3>
                     <div className="space-y-2 text-sm">
                        <div className="p-2 bg-gray-800/50 rounded-md">Fade In/Out</div>
                        <div className="p-2 bg-gray-800/50 rounded-md">Color Grade</div>
                     </div>
                </aside>

                {/* Main Area (Preview & Timeline) */}
                <main className="flex-1 flex flex-col overflow-hidden">
                    {/* Preview Player */}
                    <div className="flex-1 p-4 flex items-center justify-center bg-black">
                        <div className="w-full max-w-2xl aspect-video bg-gray-700 flex items-center justify-center text-gray-500">
                           <svg xmlns="http://www.w3.org/2000/svg" className="h-16 w-16" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z" /><path strokeLinecap="round" strokeLinejoin="round" d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
                        </div>
                    </div>

                    {/* Timeline */}
                    <div className="h-48 p-2 border-t border-blue-500/20 bg-gray-800/50 flex-shrink-0 overflow-x-auto">
                        <div className="w-[1000px] h-full space-y-1">
                            {/* Time ruler */}
                            <div className="h-6 flex text-xs text-gray-500">
                                {[...Array(10)].map((_,i) => <div key={i} className="w-[100px] border-l border-gray-700 pl-1">00:0{i}</div>)}
                            </div>
                            {/* Video Track */}
                            <div className="h-16 bg-gray-700/50 rounded-md flex items-center p-2">
                                <div className="h-full w-40 bg-blue-500/50 rounded-md border border-blue-400"></div>
                                <div className="h-full w-56 bg-blue-500/50 rounded-md border border-blue-400 ml-8"></div>
                            </div>
                            {/* Audio Track */}
                            <div className="h-16 bg-gray-700/50 rounded-md flex items-center p-2">
                                <div className="h-full w-80 bg-green-500/50 rounded-md border border-green-400"></div>
                            </div>
                        </div>
                    </div>
                </main>
            </div>
        </div>
    );
};

export default VideoEditingHubPage;
