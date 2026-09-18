import React from 'react';

interface CommsHubModalProps {
  isOpen: boolean;
  onClose: () => void;
}

const CommsHubModal: React.FC<CommsHubModalProps> = ({ isOpen, onClose }) => {
  if (!isOpen) return null;

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
                <path strokeLinecap="round" strokeLinejoin="round" d="M8 12h.01M12 12h.01M16 12h.01M21 12c0 4.418-4.03 8-9 8a9.863 9.863 0 01-4.255-.949L3 20l1.395-3.72C3.512 15.042 3 13.574 3 12c0-4.418 4.03-8 9-8s9 3.582 9 8z" />
             </svg>
            <h2 className="text-2xl font-bold text-blue-300">Comms Core</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            <p className="text-gray-400 mb-6">Manage your communications through the AI. Draft and send emails, text messages, or initiate phone calls via natural language commands directly in the chat.</p>
            <div className="space-y-4">
                <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                    <h3 className="font-semibold text-lg text-blue-300 mb-2">Send Email</h3>
                    <p className="text-sm text-gray-400">Use the chat to compose and send emails.</p>
                    <p className="text-sm font-mono mt-2 bg-gray-900/50 p-2 rounded-md text-cyan-300">e.g., "send an email to jane@example.com about our project update"</p>
                </div>
                <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                    <h3 className="font-semibold text-lg text-blue-300 mb-2">Send Text Message</h3>
                     <p className="text-sm text-gray-400">Send SMS messages to your contacts.</p>
                    <p className="text-sm font-mono mt-2 bg-gray-900/50 p-2 rounded-md text-cyan-300">e.g., "send a text to John saying I'll be there in 5 minutes"</p>
                </div>
                 <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                    <h3 className="font-semibold text-lg text-blue-300 mb-2">Make Phone Call</h3>
                     <p className="text-sm text-gray-400">Initiate phone calls through a connected device.</p>
                    <p className="text-sm font-mono mt-2 bg-gray-900/50 p-2 rounded-md text-cyan-300">e.g., "call mom"</p>
                </div>
            </div>
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default CommsHubModal;