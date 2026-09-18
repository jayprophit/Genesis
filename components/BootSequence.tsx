import React, { useState, useEffect } from 'react';

const bootSteps = [
    { text: 'Booting Quantum OS...', delay: 100 },
    { text: 'Initializing Quantum Core...', delay: 500 },
    { text: 'Calibrating Entanglement Matrix...', delay: 500 },
    { text: 'Securing Neural Link...', delay: 400 },
    { text: 'Activating Cognitive Cores...', delay: 600 },
    { text: 'Synchronizing Device Mesh...', delay: 500 },
    { text: 'Loading User Profile...', delay: 300 },
    { text: 'All systems nominal. Welcome.', delay: 200 },
];

interface BootSequenceProps {
    onComplete: () => void;
}

const BootSequence: React.FC<BootSequenceProps> = ({ onComplete }) => {
    const [currentStep, setCurrentStep] = useState(0);

    useEffect(() => {
        if (currentStep < bootSteps.length -1) {
            const timer = setTimeout(() => {
                setCurrentStep(prev => prev + 1);
            }, bootSteps[currentStep].delay);
            return () => clearTimeout(timer);
        } else {
             const finalTimer = setTimeout(onComplete, 1000);
             return () => clearTimeout(finalTimer);
        }
    }, [currentStep, onComplete]);

    return (
        <div className="fixed inset-0 bg-black flex flex-col items-center justify-center z-[10000] font-mono text-green-400">
            <div className="w-24 h-24 mb-8">
                <div className="relative w-full h-full">
                    <div className="absolute inset-0 border-2 border-green-500/50 rounded-full animate-spin-slow" style={{ animationDuration: '6s'}}></div>
                    <div className="absolute inset-2 border border-green-400/40 rounded-full animate-spin-slow" style={{ animationDuration: '4s', animationDirection: 'reverse'}}></div>
                    <div className="absolute inset-4 bg-green-500/30 rounded-full animate-pulse"></div>
                </div>
            </div>
            <div className="w-full max-w-lg text-left p-4">
                {bootSteps.slice(0, currentStep + 1).map((step, index) => (
                    <div key={index} className="flex items-center">
                        <span className="mr-2">&gt;</span>
                        <p className={`transition-opacity duration-300 ${index <= currentStep ? 'opacity-100' : 'opacity-0'}`}>
                           {step.text}
                           {index === currentStep && <span className="inline-block w-2 h-4 bg-green-400 ml-2 animate-pulse"></span>}
                        </p>
                    </div>
                ))}
            </div>
        </div>
    );
};

export default BootSequence;
