import React from 'react';
import { User } from '../types';

interface CommerceHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  user: User | null;
  onStartVerification: () => void;
}

const CommerceHubModal: React.FC<CommerceHubModalProps> = ({ isOpen, onClose, user, onStartVerification }) => {
  if (!isOpen) return null;

  const isVerified = user?.kycStatus === 'verified';

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-2xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(400px, 80vh, 700px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <div className="flex items-center space-x-3">
             <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}>
                <path strokeLinecap="round" strokeLinejoin="round" d="M13 7h8m0 0v8m0-8l-8 8-4-4-6 6" />
             </svg>
            <h2 className="text-2xl font-bold text-blue-300">Commerce Core</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            <p className="text-gray-400 mb-6">Access global financial markets, e-commerce platforms, and educational systems. Analyze stocks, execute trades, and more.</p>
            
            {!isVerified && (
                 <div className="p-4 mb-6 bg-yellow-600/20 border border-yellow-500/50 rounded-lg text-center">
                    <p className="font-semibold text-yellow-200">KYC Verification Required</p>
                    <p className="text-sm text-yellow-300/80 mt-1">Trading and financial analysis features require identity verification.</p>
                    <button onClick={() => { onStartVerification(); onClose(); }} className="mt-3 px-4 py-2 bg-yellow-500 text-black text-sm rounded-md hover:bg-yellow-400 transition-colors font-semibold">
                        Verify Identity Now
                    </button>
                </div>
            )}

            <div className={`space-y-4 ${!isVerified ? 'opacity-40 pointer-events-none' : ''}`}>
                <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                    <h3 className="font-semibold text-lg text-blue-300 mb-2">Analyze Stock Symbol</h3>
                    <p className="text-sm text-gray-400">e.g., "analyze stock symbol GOOGL".</p>
                </div>
                <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                    <h3 className="font-semibold text-lg text-blue-300 mb-2">Execute Trade</h3>
                    <p className="text-sm text-gray-400">e.g., "buy 10 shares of TSLA".</p>
                </div>
                 <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                    <h3 className="font-semibold text-lg text-blue-300 mb-2">Find Product</h3>
                    <p className="text-sm text-gray-400">e.g., "find a 4K 120Hz monitor".</p>
                </div>
                 <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                    <h3 className="font-semibold text-lg text-blue-300 mb-2">Create Course Outline</h3>
                    <p className="text-sm text-gray-400">e.g., "create an advanced course outline for machine learning".</p>
                </div>
            </div>
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default CommerceHubModal;