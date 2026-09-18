import React, { useState } from 'react';
import { LearningPath, LearningStep } from '../types';
import { generateLearningPath } from '../services/geminiService';

interface ELearningHubModalProps {
  isOpen: boolean;
  onClose: () => void;
}

const ELearningHubModal: React.FC<ELearningHubModalProps> = ({ isOpen, onClose }) => {
    const [goal, setGoal] = useState('');
    const [learningPath, setLearningPath] = useState<LearningPath | null>(null);
    const [isLoading, setIsLoading] = useState(false);

    const handleGeneratePath = async (e: React.FormEvent) => {
        e.preventDefault();
        if (!goal.trim()) return;
        
        setIsLoading(true);
        setLearningPath(null);
        try {
            const path = await generateLearningPath(goal);
            setLearningPath(path);
        } catch (error) {
            console.error("Failed to generate learning path:", error);
            // You can set an error state here to show in the UI
        } finally {
            setIsLoading(false);
        }
    };
    
    const handleToggleStep = (stepIndex: number) => {
        if (!learningPath) return;

        const newSteps = learningPath.steps.map((step, index) => {
            if (index === stepIndex) {
                return { ...step, completed: !step.completed };
            }
            return step;
        });
        
        // If a previous step is unchecked, uncheck all subsequent steps
        if(!newSteps[stepIndex].completed) {
            for(let i = stepIndex + 1; i < newSteps.length; i++) {
                newSteps[i].completed = false;
            }
        }
        
        // If a step is checked, ensure all previous steps are also checked
        if(newSteps[stepIndex].completed) {
            for(let i = 0; i < stepIndex; i++) {
                newSteps[i].completed = true;
            }
        }

        setLearningPath({ ...learningPath, steps: newSteps });
    };

    if (!isOpen) return null;

    const progress = learningPath ? (learningPath.steps.filter(s => s.completed).length / learningPath.steps.length) * 100 : 0;

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-3xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(500px, 90vh, 700px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <div className="flex items-center space-x-3">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path d="M12 14l9-5-9-5-9 5 9 5z" /><path d="M12 14l6.16-3.422a12.083 12.083 0 01.665 6.479A11.952 11.952 0 0012 20.055a11.952 11.952 0 00-6.824-2.998 12.078 12.078 0 01.665-6.479L12 14z" /><path strokeLinecap="round" strokeLinejoin="round" d="M12 14l9-5-9-5-9 5 9 5zm0 0l6.16-3.422a12.083 12.083 0 01.665 6.479A11.952 11.952 0 0012 20.055a11.952 11.952 0 00-6.824-2.998 12.078 12.078 0 01.665-6.479L12 14zm-4 6v-7.5l4-2.222" /></svg>
            <h2 className="text-2xl font-bold text-blue-300">E-Learning Hub</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            <form onSubmit={handleGeneratePath} className="flex gap-2 mb-6">
                <input 
                    type="text"
                    value={goal}
                    onChange={(e) => setGoal(e.target.value)}
                    placeholder="What do you want to learn? e.g., 'Master Quantum Physics'"
                    className="w-full bg-gray-800 border-gray-700 rounded-lg px-4 py-3"
                    disabled={isLoading}
                />
                <button type="submit" className="bg-blue-600 hover:bg-blue-500 text-white font-bold px-6 py-3 rounded-lg disabled:opacity-50" disabled={isLoading}>
                    {isLoading ? '...' : 'Generate'}
                </button>
            </form>

            {isLoading && (
                <div className="flex flex-col items-center justify-center text-center p-8">
                    <div className="w-12 h-12 border-4 border-dashed border-blue-500/50 rounded-full animate-spin-slow"></div>
                    <p className="mt-4 text-gray-400">Generating personalized learning path...</p>
                </div>
            )}
            
            {learningPath && (
                <div className="animate-fade-in">
                    <h3 className="text-xl font-bold text-blue-300 mb-2">{learningPath.goal}</h3>
                    <div className="mb-4">
                        <div className="w-full bg-gray-700/50 rounded-full h-2.5">
                            <div className="bg-gradient-to-r from-blue-500 to-cyan-400 h-2.5 rounded-full transition-all duration-500" style={{width: `${progress}%`}}></div>
                        </div>
                        <p className="text-right text-sm text-gray-400 mt-1">{Math.round(progress)}% Complete</p>
                    </div>

                    <div className="space-y-3">
                        {learningPath.steps.map((step, index) => (
                            <div key={index} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 flex items-start gap-4">
                                <input 
                                    type="checkbox" 
                                    checked={step.completed}
                                    onChange={() => handleToggleStep(index)}
                                    className="mt-1 h-5 w-5 rounded bg-gray-700 border-gray-600 text-blue-500 focus:ring-blue-600"
                                />
                                <div>
                                    <h4 className={`font-semibold ${step.completed ? 'text-gray-500 line-through' : 'text-white'}`}>{step.title}</h4>
                                    <p className={`text-sm mt-1 ${step.completed ? 'text-gray-600' : 'text-gray-400'}`}>{step.description}</p>
                                </div>
                            </div>
                        ))}
                    </div>
                </div>
            )}
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default ELearningHubModal;
