
import React from 'react';
import { User, ThirdPartyModel } from '../types';

interface PlatformIntegrationsPageProps {
  user: User | null;
  onStartVerification: () => void;
  modelStatus: Record<ThirdPartyModel, { connected: boolean }>;
  onToggleModel: (model: ThirdPartyModel) => void;
}

const ModelCard: React.FC<{ name: string; description: string; modelId: ThirdPartyModel; isConnected: boolean; onToggle: () => void; isCore?: boolean; }> = ({ name, description, modelId, isConnected, onToggle, isCore = false }) => {
    const getLogo = (name: string) => {
        if (name.includes('Quantum')) return 'Q';
        if (name.includes('OpenAI')) return 'O';
        if (name.includes('Claude')) return 'C';
        if (name.includes('Cohere')) return 'Co';
        if (name.includes('Firebase')) return 'F';
        return name.charAt(0);
    }
    const logo = getLogo(name);

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
                {!isCore && (
                    <button onClick={onToggle} className={`px-4 py-2 w-28 text-sm font-semibold rounded-md transition-colors ${isConnected ? 'bg-red-600/20 text-red-300 hover:bg-red-600/30' : 'bg-blue-600 text-white hover:bg-blue-500'}`}>
                        {isConnected ? 'Disconnect' : 'Connect'}
                    </button>
                )}
            </div>
        </div>
    );
};


const PlatformIntegrationsPage: React.FC<PlatformIntegrationsPageProps> = ({ user, onStartVerification, modelStatus, onToggleModel }) => {
  const isVerified = user?.kycStatus === 'verified';

  return (
    <main className="w-full h-full p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent relative">
        <p className="text-gray-400 mb-6 max-w-4xl">Connect and disconnect external AI models and data connectors to expand Quantum AI's capabilities. The AI will intelligently route tasks to the best model for the job.</p>

        {!isVerified && (
        <div className="absolute inset-x-6 inset-y-0 bg-gray-900/80 backdrop-blur-sm z-10 flex flex-col items-center justify-center text-center p-8 rounded-lg">
            <p className="font-semibold text-yellow-200 text-lg">KYC Verification Required</p>
            <p className="text-sm text-yellow-300/80 mt-2 max-w-sm">To prevent API abuse and ensure secure operation, connecting to third-party models requires identity verification.</p>
            <button onClick={onStartVerification} className="mt-6 px-4 py-2 bg-yellow-500 text-black text-sm rounded-md hover:bg-yellow-400 transition-colors font-semibold">
            Verify Identity Now
            </button>
        </div>
        )}

        <div className={`space-y-6 ${!isVerified ? 'opacity-40 pointer-events-none' : ''}`}>
             <div>
                <h3 className="text-lg font-semibold text-blue-300 mb-3">Core Model</h3>
                <ModelCard name="Quantum Core" description="The primary, in-house WBE model. Cannot be disconnected." modelId="quantumCore" isConnected={modelStatus.quantumCore.connected} onToggle={() => {}} isCore={true} />
            </div>
            <div>
                <h3 className="text-lg font-semibold text-blue-300 mb-3">Third-Party Language Models</h3>
                <div className="space-y-4">
                    <ModelCard name="OpenAI ChatGPT" description="General-purpose conversational and creative text generation." modelId="openai" isConnected={modelStatus.openai.connected} onToggle={() => onToggleModel('openai')} />
                    <ModelCard name="Anthropic Claude" description="Specialized in safety, detailed analysis, and long-context tasks." modelId="claude" isConnected={modelStatus.claude.connected} onToggle={() => onToggleModel('claude')} />
                    <ModelCard name="Cohere" description="Enterprise-focused models for search, summarization, and copywriting." modelId="cohere" isConnected={modelStatus.cohere.connected} onToggle={() => onToggleModel('cohere')} />
                </div>
            </div>
             <div>
                <h3 className="text-lg font-semibold text-blue-300 mb-3">Data Connectors</h3>
                <div className="space-y-4">
                    <ModelCard name="Firebase" description="Connect to Firebase projects for real-time data synchronization." modelId="firebase" isConnected={modelStatus.firebase.connected} onToggle={() => onToggleModel('firebase')} />
                </div>
            </div>
        </div>
    </main>
  );
};

export default PlatformIntegrationsPage;
