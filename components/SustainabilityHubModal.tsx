import React, { useState, useCallback } from 'react';
import { getSustainabilityAnalysis } from '../services/geminiService';

interface SustainabilityHubModalProps {
  isOpen: boolean;
  onClose: () => void;
}

const sdgs = [
  { name: 'No Poverty', color: '#E5243B' },
  { name: 'Zero Hunger', color: '#DDA63A' },
  { name: 'Good Health and Well-Being', color: '#4C9F38' },
  { name: 'Quality Education', color: '#C5192D' },
  { name: 'Gender Equality', color: '#FF3A21' },
  { name: 'Clean Water and Sanitation', color: '#26BDE2' },
  { name: 'Affordable and Clean Energy', color: '#FCC30B' },
  { name: 'Decent Work and Economic Growth', color: '#A21942' },
  { name: 'Industry, Innovation and Infrastructure', color: '#FD6925' },
  { name: 'Reduced Inequalities', color: '#DD1367' },
  { name: 'Sustainable Cities and Communities', color: '#FD9D24' },
  { name: 'Responsible Consumption and Production', color: '#BF8B2E' },
  { name: 'Climate Action', color: '#3F7E44' },
  { name: 'Life Below Water', color: '#0A97D9' },
  { name: 'Life on Land', color: '#56C02B' },
  { name: 'Peace, Justice and Strong Institutions', color: '#00689D' },
  { name: 'Partnerships for the Goals', color: '#19486A' },
];

const renderMarkdown = (text: string) => {
    let html = text
        .replace(/\*\*(.*?)\*\*/g, '<strong>$1</strong>')
        .replace(/\*(.*?)\*/g, '<em>$1</em>')
        .replace(/(\d+\.\s.*)/g, '<p>$1</p>')
        .replace(/\n/g, '<br />');

    // Group paragraphs correctly
    html = html.split('<br /><br />').map(p => `<p>${p}</p>`).join('');

    return { __html: html };
};

const SustainabilityHubModal: React.FC<SustainabilityHubModalProps> = ({ isOpen, onClose }) => {
    const [selectedGoal, setSelectedGoal] = useState<{ index: number, name: string } | null>(null);
    const [analysis, setAnalysis] = useState<string | null>(null);
    const [isLoading, setIsLoading] = useState(false);

    const handleGoalClick = useCallback(async (index: number, name: string) => {
        setSelectedGoal({ index, name });
        setIsLoading(true);
        setAnalysis(null);
        try {
            const result = await getSustainabilityAnalysis(name);
            setAnalysis(result);
        } catch (error) {
            setAnalysis("Failed to retrieve analysis. Please try again.");
        } finally {
            setIsLoading(false);
        }
    }, []);

    if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-6xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(600px, 90vh, 900px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <div className="flex items-center space-x-3">
             <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M12 21a9.004 9.004 0 008.716-6.747M12 21a9.004 9.004 0 01-8.716-6.747M12 21c2.485 0 4.5-4.03 4.5-9S14.485 3 12 3m0 18c-2.485 0-4.5-4.03-4.5-9S9.515 3 12 3m0 0a8.997 8.997 0 017.843 4.582M12 3a8.997 8.997 0 00-7.843 4.582m15.686 0A11.953 11.953 0 0112 10.5c-2.998 0-5.74-1.1-7.843-2.918m15.686 0A8.959 8.959 0 0121 12c0 .778-.099 1.533-.284 2.253m0 0A17.919 17.919 0 0112 16.5c-3.162 0-6.133-.815-8.716-2.247m0 0A9.015 9.015 0 013 12c0-1.605.42-3.113 1.157-4.418" /></svg>
            <h2 className="text-2xl font-bold text-blue-300">Sustainability Hub</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        <main className="flex-1 flex flex-col md:flex-row overflow-hidden">
            <div className="w-full md:w-1/2 flex items-center justify-center p-6">
                <div className="relative w-[400px] h-[400px] rounded-full">
                    <div className="absolute inset-0 animate-spin-slow">
                        {sdgs.map((goal, i) => {
                            const angle = 360 / sdgs.length;
                            return (
                                <div 
                                    key={goal.name} 
                                    className="absolute w-1/2 h-1/2 origin-bottom-right group cursor-pointer"
                                    style={{ transform: `rotate(${i * angle}deg)`, clipPath: `polygon(0 0, 100% 0, 100% 100%, 0 0)` }}
                                    onClick={() => handleGoalClick(i, goal.name)}
                                >
                                    <div 
                                        className="absolute inset-[-100%] transition-transform duration-300 group-hover:scale-105"
                                        style={{ backgroundColor: goal.color, transform: `skewX(${90 - angle}deg)` }}
                                    ></div>
                                </div>
                            );
                        })}
                    </div>
                    <div className="absolute inset-[30%] bg-gray-800 rounded-full flex flex-col items-center justify-center text-center p-4">
                        <img src="https://i.ibb.co/68bDR2q/sdg-logo.png" alt="SDG Logo" className="w-24 h-24"/>
                        <p className="text-xs text-gray-400 mt-2">Click a goal to analyze</p>
                    </div>
                </div>
            </div>
            <div className="w-full md:w-1/2 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent border-t md:border-t-0 md:border-l border-blue-500/20">
                {selectedGoal ? (
                    <div>
                        <h3 className="text-2xl font-bold mb-4 flex items-center" style={{color: sdgs[selectedGoal.index].color}}>
                           Goal {selectedGoal.index + 1}: {selectedGoal.name}
                        </h3>
                        {isLoading ? (
                            <div className="flex items-center justify-center h-64">
                                <div className="w-12 h-12 border-4 border-dashed border-blue-500/50 rounded-full animate-spin-slow"></div>
                            </div>
                        ) : analysis ? (
                            <div className="prose prose-invert prose-sm" dangerouslySetInnerHTML={renderMarkdown(analysis)}></div>
                        ): null}
                    </div>
                ) : (
                    <div className="flex flex-col items-center justify-center h-full text-center text-gray-500">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-16 w-16 mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1} d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
                        <h3 className="text-lg font-semibold">Select a Sustainable Development Goal</h3>
                        <p className="mt-2 max-w-sm">Select one of the 17 goals from the wheel to receive an AI-powered analysis on its objectives, challenges, and how technology can contribute to its success.</p>
                    </div>
                )}
            </div>
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default SustainabilityHubModal;