import React, { useState } from 'react';
import { SocialProvider } from '../types';

interface ConnectWalletModalProps {
  isOpen: boolean;
  onClose: () => void;
  onConnect: (privateKey: string) => void;
  onSocialLogin: (provider: SocialProvider) => void;
  onBiometricScan: () => void;
}

const ConnectWalletModal: React.FC<ConnectWalletModalProps> = ({ isOpen, onClose, onConnect, onSocialLogin, onBiometricScan }) => {
  const [privateKey, setPrivateKey] = useState('');
  const [error, setError] = useState('');
  const [showAdvanced, setShowAdvanced] = useState(false);

  const handleConnect = () => {
    if (privateKey.trim().length < 10) { 
        setError('Please enter a valid private key.');
        return;
    }
    setError('');
    onConnect(privateKey);
  };

  if (!isOpen) return null;

  return (
    <div 
      className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center"
      onClick={onClose}
      aria-modal="true"
      role="dialog"
    >
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-md m-4 p-8 text-white relative animate-fade-in"
        onClick={(e) => e.stopPropagation()}
      >
        <button onClick={onClose} className="absolute top-4 right-4 text-gray-500 hover:text-white transition-colors" aria-label="Close">
          <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
        </button>
        <header className="text-center">
            <h2 className="text-3xl font-bold text-blue-300 mb-2">Authenticate</h2>
            <p className="text-gray-400 mb-8">Access your personalized AI instance.</p>
        </header>
        
        <div className="space-y-4">
            <button onClick={onBiometricScan} className="w-full flex items-center justify-center p-4 rounded-lg bg-cyan-500/20 border border-cyan-500 hover:bg-cyan-500/30 text-lg font-semibold transition-colors duration-200 text-cyan-200 group">
                <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6 mr-3 transition-transform group-hover:scale-110" viewBox="0 0 20 20" fill="currentColor"><path d="M10 12a2 2 0 100-4 2 2 0 000 4z" /><path fillRule="evenodd" d="M.458 10C1.732 5.943 5.522 3 10 3s8.268 2.943 9.542 7c-1.274 4.057-5.022 7-9.542 7S1.732 14.057.458 10zM14 10a4 4 0 11-8 0 4 4 0 018 0z" clipRule="evenodd" /></svg>
                Biometric Scan
            </button>
            <div className="flex items-center text-xs text-gray-500">
                <div className="flex-1 border-t border-gray-700"></div>
                <span className="px-4">OR USE A TRUSTED PROVIDER</span>
                <div className="flex-1 border-t border-gray-700"></div>
            </div>
            <div className="grid grid-cols-2 gap-4">
                <button onClick={() => onSocialLogin('google')} className="w-full flex items-center justify-center p-3 rounded-lg bg-gray-700/50 hover:bg-gray-700 font-medium transition-colors duration-200">
                    <svg className="w-5 h-5 mr-2" viewBox="0 0 48 48"><path fill="#FFC107" d="M43.611 20.083H42V20H24v8h11.303c-1.649 4.657-6.08 8-11.303 8c-6.627 0-12-5.373-12-12s5.373-12 12-12c3.059 0 5.842 1.154 7.961 3.039l5.657-5.657C34.046 6.053 29.268 4 24 4C12.955 4 4 12.955 4 24s8.955 20 20 20s20-8.955 20-20c0-1.341-.138-2.65-.389-3.917z"></path><path fill="#FF3D00" d="M6.306 14.691l6.571 4.819C14.655 15.108 18.961 12 24 12c3.059 0 5.842 1.154 7.961 3.039l5.657-5.657C34.046 6.053 29.268 4 24 4C16.318 4 9.656 8.337 6.306 14.691z"></path><path fill="#4CAF50" d="M24 44c5.166 0 9.86-1.977 13.409-5.192l-6.19-5.238C29.211 35.091 26.715 36 24 36c-5.223 0-9.641-3.657-11.303-8.591l-6.571 4.819C9.656 39.663 16.318 44 24 44z"></path><path fill="#1976D2" d="M43.611 20.083H42V20H24v8h11.303c-.792 2.237-2.231 4.166-4.087 5.571l6.19 5.238C43.021 36.687 44 34.027 44 31c0-5.842-3.478-10.837-8.389-13.25z"></path></svg>
                    Google
                </button>
                 <button onClick={() => onSocialLogin('github')} className="w-full flex items-center justify-center p-3 rounded-lg bg-gray-700/50 hover:bg-gray-700 font-medium transition-colors duration-200">
                    <svg className="w-5 h-5 mr-2" viewBox="0 0 24 24" fill="currentColor"><path d="M12 0c-6.626 0-12 5.373-12 12 0 5.302 3.438 9.8 8.207 11.387.599.111.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23.957-.266 1.983-.399 3.003-.404 1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.91 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576 4.765-1.589 8.199-6.086 8.199-11.386 0-6.627-5.373-12-12-12z"/></svg>
                    GitHub
                </button>
                <button onClick={() => onSocialLogin('apple')} className="w-full flex items-center justify-center p-3 rounded-lg bg-gray-700/50 hover:bg-gray-700 font-medium transition-colors duration-200">
                    <svg className="w-5 h-5 mr-2" viewBox="0 0 24 24" fill="currentColor" xmlns="http://www.w3.org/2000/svg">
<path d="M19.221 9.471c.734-2.396-.757-4.322-2.153-5.266-1.376-.93-3.22-.63-4.288 1.144-.34.575-.62 1.157-.853 1.745-.96-.45-1.956-.69-2.964-.72-2.074-.06-3.923.95-5.068 2.576-2.57 3.65-1.02 8.35 1.58 11.23 1.25 1.38 2.87 2.2 4.6 2.22 1.63.02 3.22-.72 4.29-1.93.07-.08.15-.17.22-.25.13.08.26.16.4.24 1.25.7 2.76.99 4.26.54 2.65-.8 3.8-3.5 3.84-3.6a.43.43 0 00-.5-.47c-.07 0-1.6.3-3.3-1.06-1.4-.95-2.2-2.6-2.2-4.2 0-1.3.6-2.5 1.6-3.34zm-5.41-3.33c.8-1.4 2.2-2.06 3.4-1.5 1.2.57 1.8 2.1 1.2 3.2-.6 1.1-1.9 1.9-3.1 1.4-1.2-.5-1.9-2-1.5-3.1z"/>
</svg>
                    Apple
                </button>
                <button onClick={() => onSocialLogin('microsoft')} className="w-full flex items-center justify-center p-3 rounded-lg bg-gray-700/50 hover:bg-gray-700 font-medium transition-colors duration-200">
                    <svg className="w-5 h-5 mr-2" viewBox="0 0 24 24" fill="currentColor"><path d="M11.4 22.5h-9v-9h9v9zm11.1 0h-9v-9h9v9zm-11.1-11h-9v-9h9v9zm11.1 0h-9v-9h9v9z" /></svg>
                    Microsoft
                </button>
            </div>
            <div className="mt-8 text-center">
                <button onClick={() => setShowAdvanced(prev => !prev)} className="text-sm text-gray-500 hover:text-gray-300">
                    {showAdvanced ? 'Hide Advanced Options' : 'Show Advanced Options'}
                </button>
            </div>

            {showAdvanced && (
                <div className="mt-4 pt-4 border-t border-gray-700 animate-fade-in">
                    <h3 className="text-sm font-semibold text-gray-400 text-center mb-4">Or connect with a private key (advanced)</h3>
                    <div className="space-y-4">
                        <input
                            type="password"
                            value={privateKey}
                            onChange={(e) => setPrivateKey(e.target.value)}
                            placeholder="Enter your private key"
                            className="w-full bg-gray-800 border border-gray-700 rounded-lg px-4 py-3 text-white font-mono"
                        />
                        {error && <p className="text-red-400 text-sm text-center">{error}</p>}
                        <button onClick={handleConnect} className="w-full p-3 bg-yellow-600/80 text-black font-semibold rounded-lg hover:bg-yellow-600">
                            Connect with Key
                        </button>
                    </div>
                </div>
            )}
        </div>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default ConnectWalletModal;