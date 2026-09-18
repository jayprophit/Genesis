
import React from 'react';

interface SummaryModalProps {
  isOpen: boolean;
  onClose: () => void;
  summary: string;
  isLoading: boolean;
  theme: 'light' | 'dark';
}

const renderMarkdown = (text: string) => {
    // A simple markdown renderer
    const html = text
        .replace(/\*\*(.*?)\*\*/g, '<strong>$1</strong>')
        .replace(/\*(.*?)\*/g, '<em>$1</em>')
        .replace(/\n/g, '<br />');
    return { __html: html };
};

const SummaryModal: React.FC<SummaryModalProps> = ({ isOpen, onClose, summary, isLoading, theme }) => {
  if (!isOpen) return null;

  const modalBg = theme === 'dark' ? 'bg-gray-900/80 backdrop-blur-lg border border-blue-500/20' : 'bg-white/80 backdrop-blur-lg border border-gray-300';
  const headerTextColor = theme === 'dark' ? 'text-blue-300' : 'text-blue-700';
  const textColor = theme === 'dark' ? 'text-gray-300' : 'text-gray-800';
  const closeButtonColor = theme === 'dark' ? 'text-gray-500 hover:text-white' : 'text-gray-500 hover:text-black';

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className={`${modalBg} rounded-2xl shadow-2xl w-full max-w-2xl m-4 text-white relative animate-fade-in flex flex-col`}
        style={{ height: 'clamp(300px, 60vh, 500px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <div className="flex items-center space-x-3">
            <svg xmlns="http://www.w3.org/2000/svg" className={`h-7 w-7 ${headerTextColor}`} fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth="1.5">
                <path strokeLinecap="round" strokeLinejoin="round" d="M3.75 6.75h16.5M3.75 12h16.5m-16.5 5.25H12" />
            </svg>
            <h2 className={`text-2xl font-bold ${headerTextColor}`}>Session Summary</h2>
          </div>
          <button onClick={onClose} className={`${closeButtonColor} transition-colors`} aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
          {isLoading ? (
            <div className="flex flex-col items-center justify-center h-full text-center">
                <div className="w-12 h-12 border-4 border-dashed border-blue-500/50 rounded-full animate-spin-slow"></div>
                <p className={`mt-4 ${textColor}`}>Generating summary...</p>
            </div>
          ) : (
            <div className={`prose prose-sm prose-invert max-w-none ${textColor}`} dangerouslySetInnerHTML={renderMarkdown(summary)}></div>
          )}
        </main>
         <footer className="p-4 flex-shrink-0 border-t border-blue-500/20 text-right">
            <button onClick={onClose} className="bg-blue-600 hover:bg-blue-500 text-white font-semibold px-6 py-2 rounded-lg transition-colors">
                Close
            </button>
        </footer>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default SummaryModal;
