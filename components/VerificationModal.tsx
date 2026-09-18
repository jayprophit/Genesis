import React, { useState, useEffect } from 'react';

type VerificationStep = 'idle' | 'documentIntro' | 'documentScanning' | 'livenessIntro' | 'livenessScanning' | 'verifying' | 'success';

const stepConfig = {
    idle: {
        title: "AI Identity Verification",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="w-full h-full text-cyan-400" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M10 21h7a2 2 0 002-2V9.414a1 1 0 00-.293-.707l-5.414-5.414A1 1 0 0012.586 3H7a2 2 0 00-2 2v11m0 5l4.879-4.879m0 0a3 3 0 104.243-4.242 3 3 0 00-4.243 4.242z" /></svg>,
        text: "To access advanced features, I must verify your identity. This is a secure, one-time process.",
        buttonText: "Begin Verification",
    },
    documentIntro: {
        title: "Step 1: Document Scan",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="w-full h-full text-cyan-400" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M10 21h7a2 2 0 002-2V9.414a1 1 0 00-.293-.707l-5.414-5.414A1 1 0 0012.586 3H7a2 2 0 00-2 2v11m0 5l4.879-4.879m0 0a3 3 0 104.243-4.242 3 3 0 00-4.243 4.242z" /></svg>,
        text: "Please present your government-issued identity document to your device's primary optical sensor.",
        buttonText: "Simulate Document Scan",
    },
    documentScanning: {
        title: "Analyzing Document...",
        icon: <div className="w-full h-full border-4 border-dashed border-blue-500/50 rounded-full animate-spin-slow"></div>,
        text: "Analyzing cryptographic signatures and biometric markers. Please hold.",
        buttonText: "",
    },
    livenessIntro: {
        title: "Step 2: Liveness Check",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="w-full h-full text-cyan-400" viewBox="0 0 20 20" fill="currentColor"><path d="M10 12a2 2 0 100-4 2 2 0 000 4z" /><path fillRule="evenodd" d="M.458 10C1.732 5.943 5.522 3 10 3s8.268 2.943 9.542 7c-1.274 4.057-5.022 7-9.542 7S1.732 14.057.458 10zM14 10a4 4 0 11-8 0 4 4 0 018 0z" clipRule="evenodd" /></svg>,
        text: "I will now perform a liveness check. Please position your face within the frame.",
        buttonText: "Simulate Liveness Check",
    },
    livenessScanning: {
        title: "Analyzing Biometrics...",
        icon: <div className="w-full h-full border-4 border-dashed border-blue-500/50 rounded-full animate-spin-slow"></div>,
        text: "Verifying 3D geometry and micro-expressions to confirm presence. Please wait.",
        buttonText: "",
    },
    verifying: {
        title: "Finalizing Verification...",
        icon: <div className="w-full h-full border-4 border-dashed border-blue-500/50 rounded-full animate-spin-slow"></div>,
        text: "Cross-referencing quantum signature with decentralized identity hubs. This is the final step.",
        buttonText: "",
    },
    success: {
        title: "Verification Complete",
        icon: <svg xmlns="http://www.w3.org/2000/svg" className="w-full h-full text-green-400" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>,
        text: "Your identity has been successfully verified. All advanced features are now unlocked.",
        buttonText: "Done",
    },
};


interface VerificationModalProps {
  isOpen: boolean;
  onClose: () => void;
  onSuccess: () => void;
}

const VerificationModal: React.FC<VerificationModalProps> = ({ isOpen, onClose, onSuccess }) => {
    const [step, setStep] = useState<VerificationStep>('idle');

    useEffect(() => {
        if (!isOpen) {
            setTimeout(() => setStep('idle'), 300);
        }
    }, [isOpen]);

    const handleNextStep = () => {
        switch (step) {
            case 'idle':
                setStep('documentIntro');
                break;
            case 'documentIntro':
                setStep('documentScanning');
                setTimeout(() => setStep('livenessIntro'), 2500);
                break;
            case 'livenessIntro':
                setStep('livenessScanning');
                setTimeout(() => setStep('verifying'), 2500);
                break;
            case 'verifying':
                 // This state is timed, moves to success automatically
                 setTimeout(() => setStep('success'), 3000);
                break;
            case 'success':
                onSuccess();
                break;
        }
    };
    
    // Automatically trigger next step for scanning states
    useEffect(() => {
        if (step === 'verifying') {
            const timer = setTimeout(() => setStep('success'), 3000);
            return () => clearTimeout(timer);
        }
    }, [step]);


    if (!isOpen) return null;

    const currentStepConfig = stepConfig[step];
    
    return (
        <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
            <div 
                className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-md m-4 p-8 text-white relative animate-fade-in flex flex-col items-center text-center"
                onClick={(e) => e.stopPropagation()}
            >
                <div className="w-20 h-20 mb-6">
                    {currentStepConfig.icon}
                </div>

                <h2 className="text-2xl font-bold text-blue-300 mb-2">{currentStepConfig.title}</h2>
                <p className="text-gray-400 mb-8 min-h-[40px]">{currentStepConfig.text}</p>
                
                {currentStepConfig.buttonText && (
                     <button 
                        onClick={handleNextStep} 
                        className={`w-full p-3 rounded-md transition-colors font-semibold ${step === 'success' ? 'bg-green-600 hover:bg-green-500' : 'bg-blue-600 hover:bg-blue-500'}`}
                    >
                        {currentStepConfig.buttonText}
                    </button>
                )}
                 <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
            </div>
        </div>
    );
};

export default VerificationModal;
