import React from 'react';
import { User, ThirdPartyModel } from '../types';

interface ModelIntegrationHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  user: User | null;
  onStartVerification: () => void;
  modelStatus: Record<ThirdPartyModel, { connected: boolean }>;
  onToggleModel: (model: ThirdPartyModel) => void;
}

const ModelCard: React.FC<{ name: string; description: string; modelId: ThirdPartyModel; isConnected: boolean; onToggle: () => void; }> = ({ name, description, modelId, isConnected, onToggle }) => {
    // Simple logo placeholder
    const logo = name.charAt(0);

    return (
        <div className="flex items-center justify-between p-4 bg-gray-800/50 rounded-lg border border-gray-700">
            <div className="flex items-center space-x-4">
                <div className="w-10 h-10 rounded-lg bg-gray-700 flex items-center justify-center font-bold text-lg">{logo}</div>
                <div>
                    <h4 className="font-semibold text-white">{name}</h4>
                    <p className="text-sm text-gray-400">{description}</p>
                </div>
            </div>
            <div className="flex items-center space-x-3">
                <div className={`flex items-center text-xs font-semibold ${isConnected ? 'text-green-400' : 'text-gray-500'}`}>
                    <div className={`w-2 h-2 rounded-full mr-2 ${isConnected ? 'bg-green-400 animate-pulse' : 'bg-gray-500'}`}></div>
                    {isConnected ? 'Connected' : 'Disconnected'}
                </div>
                <button onClick={onToggle} className={`px-4 py-2 w-28 text-sm font-semibold rounded-md transition-colors ${isConnected ? 'bg-red-600/20 text-red-300 hover:bg-red-600/30' : 'bg-blue-600 text-white hover:bg-blue-500'}`}>
                    {isConnected ? 'Disconnect' : 'Connect'}
                </button>
            </div>
        </div>
    );
};


const ModelIntegrationHubModal: React.FC<ModelIntegrationHubModalProps> = ({ isOpen, onClose, user, onStartVerification, modelStatus, onToggleModel }) => {
  if (!isOpen) return null;

  const isVerified = user?.kycStatus === 'verified';

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-3xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(500px, 80vh, 700px)' }}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <div className="flex items-center space-x-3">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M11 4a2 2 0 114 0v1a1 1 0 001 1h3a1 1 0 011 1v3a1 1 0 01-1 1h-1a2 2 0 100 4h1a1 1 0 011 1v3a1 1 0 01-1 1h-3a1 1 0 01-1-1v-1a2 2 0 10-4 0v1a1 1 0 01-1 1H7a1 1 0 01-1-1v-3a1 1 0 00-1-1H4a2 2 0 110-4h1a1 1 0 001-1V7a1 1 0 011-1h3a1 1 0 001-1V4z" /></svg>
            <h2 className="text-2xl font-bold text-blue-300">Model Integration Hub</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent relative">
          <p className="text-gray-400 mb-6">Connect and disconnect external AI models to expand Quantum AI's capabilities. The AI will intelligently route tasks to the best model for the job.</p>

          {!isVerified && (
            <div className="absolute inset-x-6 inset-y-0 bg-gray-900/80 backdrop-blur-sm z-10 flex flex-col items-center justify-center text-center p-8 rounded-lg">
              <p className="font-semibold text-yellow-200 text-lg">KYC Verification Required</p>
              <p className="text-sm text-yellow-300/80 mt-2 max-w-sm">To prevent API abuse and ensure secure operation, connecting to third-party models requires identity verification.</p>
              <button onClick={() => { onStartVerification(); onClose(); }} className="mt-6 px-4 py-2 bg-yellow-500 text-black text-sm rounded-md hover:bg-yellow-400 transition-colors font-semibold">
                Verify Identity Now
              </button>
            </div>
          )}

          <div className={`space-y-4 ${!isVerified ? 'opacity-40 pointer-events-none' : ''}`}>
            <ModelCard name="OpenAI ChatGPT" description="General-purpose conversational and creative text generation." modelId="openai" isConnected={modelStatus.openai.connected} onToggle={() => onToggleModel('openai')} />
            <ModelCard name="Anthropic Claude" description="Specialized in safety, detailed analysis, and long-context tasks." modelId="claude" isConnected={modelStatus.claude.connected} onToggle={() => onToggleModel('claude')} />
            <ModelCard name="Cohere" description="Enterprise-focused models for search, summarization, and copywriting." modelId="cohere" isConnected={modelStatus.cohere.connected} onToggle={() => onToggleModel('cohere')} />
          </div>
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default ModelIntegrationHubModal;
