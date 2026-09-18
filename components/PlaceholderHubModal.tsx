import React from 'react';

interface PlaceholderHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  title: string;
}

const PlaceholderHubModal: React.FC<PlaceholderHubModalProps> = ({ isOpen, onClose, title }) => {
  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-lg m-4 text-white relative animate-fade-in flex flex-col"
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <h2 className="text-2xl font-bold text-blue-300">{title}</h2>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        <main className="flex-1 p-8 flex flex-col items-center justify-center text-center">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-16 w-16 text-gray-600 mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1}><path strokeLinecap="round" strokeLinejoin="round" d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4" /></svg>
            <h3 className="text-xl font-semibold text-gray-300">Feature Under Construction</h3>
            <p className="text-gray-500 mt-2">This hub is currently in development. Please check back later.</p>
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default PlaceholderHubModal;
