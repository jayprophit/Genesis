import React from 'react';
import { capabilities, CapabilityCategory, Capability } from '../capabilities.config';

interface CapabilitiesHubModalProps {
  isOpen: boolean;
  onClose: () => void;
}

const CapabilityCard: React.FC<{ capability: Capability }> = ({ capability }) => (
    <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 hover:bg-gray-800 hover:border-blue-500/50 transition-all duration-200">
        <div className="flex items-center justify-between">
            <h4 className="font-semibold text-white">{capability.name}</h4>
        </div>
        <p className="text-sm text-gray-400 mt-2">{capability.description}</p>
    </div>
);

const CapabilitiesHubModal: React.FC<CapabilitiesHubModalProps> = ({ isOpen, onClose }) => {
  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-5xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(500px, 90vh, 800px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <div className="flex items-center space-x-3">
             <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}>
                <path strokeLinecap="round" strokeLinejoin="round" d="M3.75 3.75v4.5m0-4.5h4.5m-4.5 0L9 9M3.75 20.25v-4.5m0 4.5h4.5m-4.5 0L9 15M20.25 3.75h-4.5m4.5 0v4.5m0-4.5L15 9m5.25 11.25h-4.5m4.5 0v-4.5m0 4.5L15 15" />
             </svg>
            <h2 className="text-2xl font-bold text-blue-300">Capabilities Core</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            <div className="space-y-8">
                {capabilities.map(category => (
                    <div key={category.id}>
                        <h3 className="text-xl font-semibold text-blue-300 mb-4 pb-2 border-b-2 border-blue-500/20">{category.name}</h3>
                        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
                            {category.capabilities.map(capability => (
                                <CapabilityCard key={capability.id} capability={capability} />
                            ))}
                        </div>
                    </div>
                ))}
            </div>
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default CapabilitiesHubModal;