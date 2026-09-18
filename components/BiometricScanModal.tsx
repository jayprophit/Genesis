import React, { useState, useEffect, useRef } from 'react';

type ScanMethod = 'iris' | 'face' | 'fingerprint';
type View = 'selection' | 'scanning' | 'error';

const scanSteps: Record<ScanMethod, { text: string; delay: number }[]> = {
    iris: [
        { text: "Initializing iris scanner...", delay: 1500 },
        { text: "Calibrating focal depth...", delay: 1000 },
        { text: "Verifying retinal patterns...", delay: 2000 },
        { text: "Authentication Complete. Access Granted.", delay: 1000 },
    ],
    face: [
        { text: "Initializing facial recognition...", delay: 1500 },
        { text: "Analyzing facial geometry...", delay: 2000 },
        { text: "Performing liveness check...", delay: 2000 },
        { text: "Authentication Complete. Access Granted.", delay: 1000 },
    ],
    fingerprint: [
        { text: "Activating fingerprint sensor...", delay: 1500 },
        { text: "Please place your finger on the sensor.", delay: 2000 },
        { text: "Analyzing ridge patterns...", delay: 2000 },
        { text: "Authentication Complete. Access Granted.", delay: 1000 },
    ],
};

interface BiometricScanModalProps {
  isOpen: boolean;
  onClose: () => void;
  onSuccess: () => void;
}

const BiometricScanModal: React.FC<BiometricScanModalProps> = ({ isOpen, onClose, onSuccess }) => {
    const videoRef = useRef<HTMLVideoElement>(null);
    const canvasRef = useRef<HTMLCanvasElement>(null);
    const [view, setView] = useState<View>('selection');
    const [scanMethod, setScanMethod] = useState<ScanMethod | null>(null);
    const [scanStep, setScanStep] = useState(0);
    const [error, setError] = useState<string | null>(null);

    const isMobile = typeof window !== 'undefined' && /Mobi/i.test(window.navigator.userAgent);

    useEffect(() => {
        if (!isOpen) {
            // Reset state when closing
            setTimeout(() => {
                setView('selection');
                setScanMethod(null);
                setScanStep(0);
                setError(null);
            }, 300);
            return;
        }

        if (view !== 'scanning' || !scanMethod || ['fingerprint'].includes(scanMethod)) return;

        let stream: MediaStream | null = null;
        let animationFrameId: number;

        const startCamera = async () => {
            try {
                if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
                    throw new Error("Camera access is not supported by your browser.");
                }
                stream = await navigator.mediaDevices.getUserMedia({ video: { facingMode: "user" } });
                if (videoRef.current) {
                    videoRef.current.srcObject = stream;
                    videoRef.current.play();
                }
                setError(null);
                drawLoop();
            } catch (err) {
                console.error("Error accessing camera:", err);
                setError("Camera access denied. Please enable camera permissions.");
                setView('error');
            }
        };

        const drawLoop = () => {
            const canvas = canvasRef.current;
            const video = videoRef.current;
            if (canvas && video) {
                const ctx = canvas.getContext('2d');
                if (ctx) {
                    ctx.clearRect(0, 0, canvas.width, canvas.height);
                    
                    const scanY = (Date.now() / 20) % canvas.height;
                    ctx.fillStyle = 'rgba(0, 255, 255, 0.5)';
                    ctx.fillRect(0, scanY, canvas.width, 3);
                    
                    const centerX = canvas.width / 2;
                    const centerY = canvas.height / 2;
                    ctx.strokeStyle = 'rgba(0, 255, 255, 0.7)';
                    ctx.lineWidth = 2;
                    ctx.beginPath();
                    ctx.arc(centerX, centerY, 80, 0, 2 * Math.PI);
                    ctx.stroke();
                }
            }
            animationFrameId = requestAnimationFrame(drawLoop);
        };

        startCamera();

        return () => {
            if (stream) {
                stream.getTracks().forEach(track => track.stop());
            }
            cancelAnimationFrame(animationFrameId);
        };
    }, [isOpen, view, scanMethod]);

    useEffect(() => {
        if (view !== 'scanning' || !scanMethod) return;

        const currentSteps = scanSteps[scanMethod];

        if (scanStep < currentSteps.length) {
            const timer = setTimeout(() => {
                setScanStep(prev => prev + 1);
            }, currentSteps[scanStep].delay);
            return () => clearTimeout(timer);
        } else {
            const finalTimer = setTimeout(onSuccess, 500);
            return () => clearTimeout(finalTimer);
        }
    }, [scanStep, view, scanMethod, onSuccess]);

    if (!isOpen) return null;
    
    const handleSelectMethod = (method: ScanMethod) => {
        setScanMethod(method);
        setView('scanning');
    }

    const currentSteps = scanMethod ? scanSteps[scanMethod] : [];
    const currentStepText = currentSteps[scanStep]?.text || "...";

    return (
        <div className="fixed inset-0 bg-black/80 z-[60] flex items-center justify-center" onClick={onClose}>
            <div className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-lg m-4 p-8 text-white relative animate-fade-in flex flex-col items-center" onClick={e => e.stopPropagation()}>
                <h2 className="text-2xl font-bold text-blue-300 mb-4">Biometric Authentication</h2>
                
                {view === 'selection' && (
                    <div className="w-full space-y-4 text-center">
                        <p className="text-gray-400 mb-4">Please select your preferred biometric method.</p>
                        <button onClick={() => handleSelectMethod('face')} className="w-full p-4 rounded-lg bg-cyan-500/10 border border-cyan-500/50 hover:bg-cyan-500/20 font-semibold transition-colors">Facial Recognition</button>
                        <button onClick={() => handleSelectMethod('iris')} className="w-full p-4 rounded-lg bg-cyan-500/10 border border-cyan-500/50 hover:bg-cyan-500/20 font-semibold transition-colors">Iris Scan</button>
                        {isMobile && <button onClick={() => handleSelectMethod('fingerprint')} className="w-full p-4 rounded-lg bg-cyan-500/10 border border-cyan-500/50 hover:bg-cyan-500/20 font-semibold transition-colors">Fingerprint Scan</button>}
                    </div>
                )}
                
                {view === 'scanning' && scanMethod && (
                    <>
                        <div className="w-64 h-48 bg-black rounded-lg overflow-hidden relative border-2 border-blue-500/30">
                            {scanMethod === 'fingerprint' ? (
                                 <div className="w-full h-full flex items-center justify-center">
                                    <svg xmlns="http://www.w3.org/2000/svg" className="w-24 h-24 text-cyan-400/50" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={0.5}><path strokeLinecap="round" strokeLinejoin="round" d="M12 11c.828 0 1.5.672 1.5 1.5s-.672 1.5-1.5 1.5-1.5-.672-1.5-1.5.672-1.5 1.5-1.5z" /><path strokeLinecap="round" strokeLinejoin="round" d="M12 21c-4.97 0-9-4.03-9-9s4.03-9 9-9 9 4.03 9 9-4.03 9-9 9z" /><path strokeLinecap="round" strokeLinejoin="round" d="M12 3v1M12 21v-1M4 12H3M21 12h-1M5.636 5.636l-.707-.707M18.364 18.364l.707.707M18.364 5.636l.707-.707M5.636 18.364l-.707.707" /></svg>
                                </div>
                            ) : (
                                <>
                                    <video ref={videoRef} className="w-full h-full object-cover" muted playsInline />
                                    <canvas ref={canvasRef} width="256" height="192" className="absolute inset-0" />
                                </>
                            )}
                        </div>
                        <div className="mt-6 text-center h-12">
                            <p className="text-cyan-300 font-mono transition-opacity duration-500">
                                {currentStepText}
                                {scanStep < currentSteps.length -1 && <span className="inline-block w-2 h-4 bg-cyan-300 ml-2 animate-pulse"></span>}
                            </p>
                        </div>
                    </>
                )}

                {view === 'error' && (
                     <div className="w-full space-y-4 text-center">
                        <p className="text-red-400">{error}</p>
                        <button onClick={onClose} className="px-6 py-2 bg-gray-600 text-white rounded-md hover:bg-gray-500 transition-colors font-semibold">Close</button>
                     </div>
                )}
            </div>
            <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
        </div>
    );
};

export default BiometricScanModal;