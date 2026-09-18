import React, { useState } from 'react';

// A simple library to generate QR codes
const QRCodeStyling = ({ data }: { data: string }) => {
    const size = 200;
    const quietZone = 10;
    const matrix: boolean[][] = [];
    // Super simplified QR-like pattern generator for visual effect
    for (let y = 0; y < 21; y++) {
        matrix[y] = [];
        for (let x = 0; x < 21; x++) {
            matrix[y][x] = (x * y + (x ^ y)) % 3 === 0 || (x > 6 && x < 14 && y > 6 && y < 14) || x % 7 === 0 || y % 7 === 0;
        }
    }
    
    return (
        <svg width={size} height={size} viewBox={`-${quietZone} -${quietZone} ${size} ${size}`} className="bg-white p-2 rounded-lg">
            {matrix.map((row, y) => 
                row.map((cell, x) => 
                    cell && <rect key={`${y}-${x}`} x={x * 8} y={y * 8} width={8} height={8} fill="#000" />
                )
            )}
        </svg>
    )
};


interface ViewKeyModalProps {
  isOpen: boolean;
  onClose: () => void;
  privateKey: string;
}

const ViewKeyModal: React.FC<ViewKeyModalProps> = ({ isOpen, onClose, privateKey }) => {
  const [isKeyCopied, setIsKeyCopied] = useState(false);

  if (!isOpen) return null;

  const handleCopyKey = () => {
    navigator.clipboard.writeText(privateKey);
    setIsKeyCopied(true);
    setTimeout(() => setIsKeyCopied(false), 2000);
  };

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-md m-4 p-8 text-white relative animate-fade-in"
        onClick={(e) => e.stopPropagation()}
      >
        <header className="text-center mb-6">
            <h2 className="text-2xl font-bold text-blue-300">Your Sovereign Private Key</h2>
            <p className="text-sm text-gray-400 mt-2">This key grants access to your decentralized profile. Keep it secret, keep it safe.</p>
        </header>
        
        <div className="flex justify-center mb-6">
            <QRCodeStyling data={privateKey} />
        </div>

        <div className="relative">
            <input 
                type="password"
                readOnly
                value={privateKey}
                className="w-full bg-gray-800 border border-gray-700 rounded-lg pl-4 pr-12 py-3 text-gray-200 font-mono text-sm"
            />
            <button onClick={handleCopyKey} className="absolute right-2 top-1/2 -translate-y-1/2 p-2 text-gray-400 hover:text-white transition-colors">
                {isKeyCopied ? (
                    <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-green-400" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 13l4 4L19 7" /></svg>
                ) : (
                    <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M8 16H6a2 2 0 01-2-2V6a2 2 0 012-2h8a2 2 0 012 2v2m-6 12h8a2 2 0 002-2v-8a2 2 0 00-2-2h-8a2 2 0 00-2 2v8a2 2 0 002 2z" /></svg>
                )}
            </button>
        </div>
        
        <div className="mt-6 text-center">
            <button onClick={onClose} className="px-6 py-2 bg-blue-600 text-white rounded-md hover:bg-blue-500 transition-colors font-semibold">Done</button>
        </div>
         <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default ViewKeyModal;
