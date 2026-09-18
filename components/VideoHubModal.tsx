import React, { useState } from 'react';
import { ActiveVideoGeneration, VideoGenerationParameters } from '../types';

interface VideoHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  activeGeneration: ActiveVideoGeneration | null;
  onStartGeneration: (params: VideoGenerationParameters) => void;
  onClearGeneration: () => void;
}

const loadingMessages = [
    "Aligning quantum bits...",
    "Rendering photonic streams...",
    "Compiling space-time coordinates...",
    "Simulating light refraction...",
    "Entangling pixel matrices...",
    "Finalizing temporal sequence...",
];

const VideoHubModal: React.FC<VideoHubModalProps> = ({ isOpen, onClose, activeGeneration, onStartGeneration, onClearGeneration }) => {
  const [params, setParams] = useState<VideoGenerationParameters>({
    prompt: '',
    resolution: '720p',
    aspectRatio: '16:9'
  });

  if (!isOpen) return null;

  const handleStart = () => {
    if (params.prompt.trim()) {
      onStartGeneration(params);
    }
  };

  const renderConfiguration = () => (
    <>
      <div className="p-6 flex-shrink-0">
        <p className="text-gray-400 mb-4">Describe the video you want to create. The AI will generate a short video based on your prompt using the Veo model.</p>
        <div className="space-y-4">
          <div>
            <label className="block text-sm font-medium text-gray-300 mb-1">Prompt</label>
            <textarea value={params.prompt} onChange={e => setParams(p => ({...p, prompt: e.target.value}))} rows={4} placeholder="e.g., A majestic whale breaching the ocean at sunset..." className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500 resize-none" />
          </div>
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
              <label className="block text-sm font-medium text-gray-300 mb-1">Resolution</label>
              <select value={params.resolution} onChange={e => setParams(p => ({...p, resolution: e.target.value as '720p' | '1080p'}))} className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500">
                <option>720p</option><option>1080p</option>
              </select>
            </div>
            <div>
              <label className="block text-sm font-medium text-gray-300 mb-1">Aspect Ratio</label>
              <select value={params.aspectRatio} onChange={e => setParams(p => ({...p, aspectRatio: e.target.value as '16:9' | '9:16'}))} className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500">
                <option>16:9 (Landscape)</option><option>9:16 (Portrait)</option>
              </select>
            </div>
          </div>
        </div>
      </div>
      <footer className="p-6 flex-shrink-0 border-t border-blue-500/20 text-right">
        <button onClick={handleStart} disabled={!params.prompt.trim()} className="bg-blue-600 hover:bg-blue-500 text-white font-bold px-6 py-2 rounded-lg transition-colors disabled:opacity-50 disabled:cursor-not-allowed">
          Generate Video
        </button>
      </footer>
    </>
  );

  const renderMonitoring = () => {
    if (!activeGeneration) return null;
    const isFinished = activeGeneration.status === 'Complete' || activeGeneration.status === 'Error';
    return (
      <div className="p-6 flex-1 flex flex-col min-h-0 items-center justify-center">
        {activeGeneration.status === 'Complete' && activeGeneration.resultUrl ? (
            <div className="w-full">
                <h3 className="text-xl font-semibold text-center text-green-300 mb-4">Generation Complete</h3>
                <video src={activeGeneration.resultUrl} controls autoPlay className="w-full max-w-lg mx-auto rounded-lg shadow-2xl" />
            </div>
        ) : activeGeneration.status === 'Error' ? (
            <div className="text-center">
                 <svg xmlns="http://www.w3.org/2000/svg" className="h-16 w-16 mx-auto text-red-400 mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 8v4m0 4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
                <h3 className="text-xl font-semibold text-red-300">Generation Failed</h3>
                <p className="text-gray-400 mt-2">{activeGeneration.progressMessage}</p>
            </div>
        ) : (
            <div className="text-center">
                <div className="w-20 h-20 border-4 border-dashed border-blue-500/50 rounded-full animate-spin-slow mx-auto"></div>
                <h3 className="text-xl font-semibold text-blue-300 mt-6">Generating Video...</h3>
                <p className="text-gray-400 mt-2">{activeGeneration.progressMessage}</p>
                <p className="text-sm text-gray-500 mt-4">(This can take several minutes. You can close this window; the process will continue.)</p>
            </div>
        )}

         {isFinished && (
            <div className="mt-8 text-center flex-shrink-0">
                <button onClick={onClearGeneration} className="bg-blue-600 hover:bg-blue-500 text-white font-bold px-6 py-2 rounded-lg transition-colors">
                    Create Another Video
                </button>
            </div>
         )}
      </div>
    );
  };

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-2xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(500px, 80vh, 700px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <h2 className="text-2xl font-bold text-blue-300">Video Hub</h2>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        
        {activeGeneration ? renderMonitoring() : renderConfiguration()}

        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default VideoHubModal;