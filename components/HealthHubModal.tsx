
import React, { useState, useEffect, useRef } from 'react';
import { ActiveBioScan, BioScanParameters, HealthFinding } from '../types';
import FrequencyVisualizer from './FrequencyVisualizer';
import InteractiveHealingWeb from './InteractiveHealingWeb';

interface HealthHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  activeBioScan: ActiveBioScan | null;
  onStartBioScan: (params: BioScanParameters) => void;
  onClearBioScan: () => void;
}

interface FrequencyProgram {
    name: string;
    description: string;
    frequency: number;
    duration: number; // in minutes
}

const frequencyPrograms: FrequencyProgram[] = [
    { name: 'Cellular Repair', description: 'Simulates frequencies aimed at promoting cellular regeneration.', frequency: 727, duration: 15 },
    { name: 'Detoxification', description: 'Targets frequencies associated with eliminating bodily toxins.', frequency: 625, duration: 20 },
    { name: 'Pain Relief', description: 'Focuses on frequencies thought to reduce inflammation and pain signals.', frequency: 304, duration: 25 },
    { name: 'Energy Boost', description: 'Uses frequencies intended to vitalize and energize the body.', frequency: 465, duration: 10 },
    { name: 'Sleep Aid', description: 'Simulates calming frequencies to aid in relaxation and deep sleep.', frequency: 3.5, duration: 30 },
];

const BioMetricVisualizer: React.FC = () => (
    <div className="w-full h-48 bg-gray-900/50 rounded-lg relative overflow-hidden border border-blue-500/20 flex items-center justify-center">
        <svg className="absolute w-40 h-40 text-cyan-400/30" fill="none" viewBox="0 0 100 100">
            <path d="M 50,50 m -48,0 a 48,48 0 1,1 96,0 a 48,48 0 1,1 -96,0" stroke="currentColor" strokeWidth="2" />
            <path d="M 50,50 m -38,0 a 38,38 0 1,1 76,0 a 38,38 0 1,1 -76,0" stroke="currentColor" strokeWidth="1" strokeDasharray="4 2" />
            <path d="M 50,50 m -28,0 a 28,28 0 1,1 56,0 a 28,28 0 1,1 -56,0" stroke="currentColor" strokeWidth="1" />
            <path d="M 20 50 L 80 50 M 50 20 L 50 80" stroke="currentColor" strokeWidth="0.5" />
        </svg>
        <div className="absolute w-full h-1 bg-cyan-400/70 shadow-2xl shadow-cyan-400 animate-scan"></div>
        <style>{`
          @keyframes scan {
            0% { transform: translateY(-96px); }
            50% { transform: translateY(96px); }
            100% { transform: translateY(-96px); }
          }
        `}</style>
    </div>
);

const HealthFindingCard: React.FC<{ finding: HealthFinding }> = ({ finding }) => {
    const getDomainStyle = (domain: HealthFinding['domain']) => {
        switch(domain) {
            case 'Cellular': return { icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM7 9a1 1 0 000 2h6a1 1 0 100-2H7z" clipRule="evenodd" /></svg>, color: 'text-yellow-300', border: 'border-yellow-500/30' };
            case 'Neurological': return { icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path d="M10 3.5a1.5 1.5 0 013 0V4a1 1 0 001 1h3a1 1 0 011 1v3a1 1 0 01-1 1h-.5a1.5 1.5 0 000 3h.5a1 1 0 011 1v3a1 1 0 01-1 1h-3a1 1 0 00-1-1v-.5a1.5 1.5 0 01-3 0v.5a1 1 0 00-1 1H6a1 1 0 01-1-1v-3a1 1 0 011-1h.5a1.5 1.5 0 000-3H6a1 1 0 01-1-1V6a1 1 0 011-1h3a1 1 0 001-1v-.5z" /></svg>, color: 'text-purple-300', border: 'border-purple-500/30' };
            case 'Cardiovascular': return { icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M3.172 5.172a4 4 0 015.656 0L10 6.343l1.172-1.171a4 4 0 115.656 5.656L10 17.657l-6.828-6.829a4 4 0 010-5.656z" clipRule="evenodd" /></svg>, color: 'text-red-400', border: 'border-red-500/30' };
            case 'Metabolic': return { icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.707 7.293a1 1 0 00-1.414 1.414L8.586 10l-1.293 1.293a1 1 0 101.414 1.414L10 11.414l1.293 1.293a1 1 0 001.414-1.414L11.414 10l1.293-1.293a1 1 0 00-1.414-1.414L10 8.586 8.707 7.293z" clipRule="evenodd" /></svg>, color: 'text-cyan-300', border: 'border-cyan-500/30' };
            default: return { icon: <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7-4a1 1 0 11-2 0 1 1 0 012 0zM9 9a1 1 0 000 2v3a1 1 0 001 1h1a1 1 0 100-2v-3a1 1 0 00-1-1H9z" clipRule="evenodd" /></svg>, color: 'text-gray-300', border: 'border-gray-500/30' };
        }
    }
    const { icon, color, border } = getDomainStyle(finding.domain);
    return (
        <div className={`p-4 bg-gray-800/50 rounded-lg border ${border}`}>
            <div className={`flex items-center font-semibold text-lg ${color} mb-2`}>
                {icon}
                <span className="ml-2">{finding.domain} Finding</span>
            </div>
            <div className="space-y-3 text-sm">
                <div><h4 className="font-semibold text-gray-300">Finding:</h4><p className="text-gray-400">{finding.finding}</p></div>
                <div><h4 className="font-semibold text-gray-300">Implication:</h4><p className="text-gray-400">{finding.implication}</p></div>
                <div><h4 className="font-semibold text-gray-300">Recommendation:</h4><p className="text-gray-200 bg-gray-900/50 p-2 rounded-md border-l-2 border-blue-400">{finding.recommendation}</p></div>
            </div>
        </div>
    )
};


const HealthHubModal: React.FC<HealthHubModalProps> = ({ isOpen, onClose, activeBioScan, onStartBioScan, onClearBioScan }) => {
  const [activeTab, setActiveTab] = useState<'bioscan' | 'frequency' | 'web'>('bioscan');
  const [bioScanParams, setBioScanParams] = useState<BioScanParameters>({ scanType: 'Comprehensive' });

  // Frequency Therapy State
  const [activeProgram, setActiveProgram] = useState<FrequencyProgram | null>(null);
  const [sessionTime, setSessionTime] = useState(0);
  const [isRunning, setIsRunning] = useState(false);
  const timerRef = useRef<number | null>(null);
  const audioContextRef = useRef<AudioContext | null>(null);
  const oscillatorRef = useRef<OscillatorNode | null>(null);
  
  const stopAudio = () => {
    if (oscillatorRef.current) {
        oscillatorRef.current.stop();
        oscillatorRef.current.disconnect();
        oscillatorRef.current = null;
    }
    if (audioContextRef.current && audioContextRef.current.state !== 'closed') {
        audioContextRef.current.close().then(() => {
            audioContextRef.current = null;
        });
    }
  };

  const handleClose = () => {
    if (timerRef.current) clearInterval(timerRef.current);
    setIsRunning(false);
    setActiveProgram(null);
    stopAudio();
    onClose();
  }

  useEffect(() => {
    return () => {
        // Ensure audio stops on component unmount
        stopAudio();
    };
}, []);

  useEffect(() => {
    if (isRunning && activeProgram) {
      if (!audioContextRef.current || audioContextRef.current.state === 'closed') {
        audioContextRef.current = new (window.AudioContext || (window as any).webkitAudioContext)();
      }
      const audioCtx = audioContextRef.current;
      
      stopAudio(); // Stop any previous sound

      const oscillator = audioCtx.createOscillator();
      const gainNode = audioCtx.createGain();
      
      oscillator.type = 'sine';
      oscillator.frequency.setValueAtTime(activeProgram.frequency, audioCtx.currentTime);
      gainNode.gain.setValueAtTime(0.1, audioCtx.currentTime); // Low volume to be safe
      
      oscillator.connect(gainNode);
      gainNode.connect(audioCtx.destination);
      oscillator.start();
      
      oscillatorRef.current = oscillator;
    } else {
        stopAudio();
    }
  }, [isRunning, activeProgram]);


  useEffect(() => {
    if (isRunning && sessionTime > 0) {
      timerRef.current = window.setInterval(() => {
        setSessionTime(prev => prev - 1);
      }, 1000);
    } else if (sessionTime <= 0 && isRunning) {
      handleStopSession();
    }
    return () => {
      if (timerRef.current) clearInterval(timerRef.current);
    };
  }, [isRunning, sessionTime]);

  const handleStartSession = (program: FrequencyProgram) => {
    setActiveProgram(program);
    setSessionTime(program.duration * 60);
    setIsRunning(true);
  };
  
  const handleStopSession = () => {
    if (timerRef.current) clearInterval(timerRef.current);
    setIsRunning(false);
  };
  
  const formatTime = (seconds: number) => {
    const mins = Math.floor(seconds / 60).toString().padStart(2, '0');
    const secs = (seconds % 60).toString().padStart(2, '0');
    return `${mins}:${secs}`;
  }

  if (!isOpen) return null;
  
  const renderBioScan = () => {
    const renderConfiguration = () => (
      <>
        <div className="p-6 flex-shrink-0">
          <p className="text-gray-400 mb-4">Initiate a non-invasive bio-scan to analyze your physiological and neurological well-being. The AI will provide a comprehensive report with actionable insights.</p>
          <BioMetricVisualizer />
        </div>
        <footer className="p-6 flex-shrink-0 border-t border-blue-500/20 text-center">
          <button onClick={() => onStartBioScan(bioScanParams)} className="bg-blue-600 hover:bg-blue-500 text-white font-bold px-8 py-3 text-lg rounded-lg transition-colors">
            Initiate Comprehensive Bio-Scan
          </button>
        </footer>
      </>
    );

    const renderMonitoring = () => {
      if (!activeBioScan) return null;
      const isFinished = activeBioScan.status === 'Complete' || activeBioScan.status === 'Error';
      return (
        <div className="p-6 flex-1 flex flex-col min-h-0">
          <div className="flex-shrink-0">
            <BioMetricVisualizer />
            <div className="my-4 text-center"><h3 className="text-lg font-semibold text-blue-300">Bio-Scan in Progress</h3></div>
            <div className="my-2 space-y-2">
                <div className="flex justify-between text-sm text-gray-400">
                    <span>Status: <span className="font-semibold text-cyan-300">{activeBioScan.status}...</span></span>
                    <span className="font-semibold text-purple-300">{activeBioScan.currentStep}</span>
                </div>
                <div className="w-full bg-gray-700/50 rounded-full h-2.5"><div className="bg-gradient-to-r from-blue-500 to-purple-500 h-2.5 rounded-full transition-all duration-500" style={{width: `${activeBioScan.progress}%`}}></div></div>
            </div>
          </div>
          <div className="flex-1 mt-4 bg-black/30 rounded-lg p-3 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent min-h-0">
              {isFinished && activeBioScan.result ? (
                   <div className="space-y-6">
                      <div>
                          <h4 className="text-xl font-bold text-gray-200 mb-2">Health Summary</h4>
                          <div className="text-center p-4 bg-gray-800/50 rounded-lg">
                              <p className="text-gray-400 text-sm">Overall Wellness Score</p>
                              <p className="text-5xl font-bold text-green-400">{activeBioScan.result.overallScore.toFixed(1)}<span className="text-2xl text-gray-400">%</span></p>
                          </div>
                          <p className="text-gray-300 whitespace-pre-wrap mt-4">{activeBioScan.result.summary}</p>
                      </div>
                       <div>
                          <h4 className="text-xl font-bold text-gray-200 mb-3">Key Findings & Recommendations</h4>
                          <div className="space-y-4">{activeBioScan.result.findings.map((finding, i) => <HealthFindingCard key={i} finding={finding} />)}</div>
                      </div>
                   </div>
              ) : (<div className="font-mono text-xs text-green-400 h-full flex items-end"><p className="animate-pulse">{activeBioScan.currentStep}...</p></div>)}
          </div>
           {isFinished && (<div className="mt-4 text-center flex-shrink-0"><button onClick={onClearBioScan} className="bg-blue-600 hover:bg-blue-500 text-white font-bold px-6 py-2 rounded-lg transition-colors">Start New Scan</button></div>)}
        </div>
      );
    };

    return activeBioScan ? renderMonitoring() : renderConfiguration();
  };
  
  const renderFrequencyTherapy = () => (
    <div className="p-6 flex-1 flex flex-col min-h-0">
      {!activeProgram ? (
        <>
            <p className="text-gray-400 mb-4 flex-shrink-0">Select a simulated bio-healing frequency program. These are inspired by the theories of Dr. Royal Rife and are for conceptual purposes only.</p>
            <div className="flex-1 overflow-y-auto space-y-3 pr-2 scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                {frequencyPrograms.map(prog => (
                    <div key={prog.name} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 flex items-center justify-between">
                        <div>
                            <h4 className="font-semibold text-white">{prog.name}</h4>
                            <p className="text-sm text-gray-400">{prog.description}</p>
                        </div>
                        <button onClick={() => handleStartSession(prog)} className="px-4 py-2 text-sm font-semibold rounded-md transition-colors bg-blue-600 hover:bg-blue-500 text-white">
                            Start
                        </button>
                    </div>
                ))}
            </div>
        </>
      ) : (
        <div className="flex flex-col items-center justify-center h-full text-center">
            <h3 className="text-2xl font-bold text-blue-300">{activeProgram.name}</h3>
            <p className="text-gray-400">Session in progress</p>
            <div className="my-6 w-full max-w-sm h-24">
                <FrequencyVisualizer isRunning={isRunning} frequency={activeProgram.frequency} />
            </div>
            <p className="text-lg text-cyan-300 font-mono">Target Frequency: {activeProgram.frequency} Hz</p>
            <p className="text-5xl font-bold text-white my-4">{formatTime(sessionTime)}</p>
            {isRunning ? (
                <button onClick={handleStopSession} className="px-8 py-3 text-lg font-semibold rounded-lg transition-colors bg-red-600/80 hover:bg-red-600 text-white">Stop Session</button>
            ) : (
                <div className="space-y-2">
                    <p className="text-lg text-green-400 font-semibold">Session Complete</p>
                    <button onClick={() => setActiveProgram(null)} className="px-6 py-2 font-semibold rounded-lg transition-colors bg-blue-600 hover:bg-blue-500 text-white">Back to Programs</button>
                </div>
            )}
        </div>
      )}
    </div>
  );
  
  const renderHealingWeb = () => (
      <div className="p-0 flex-1 flex flex-col min-h-0 h-full">
         <InteractiveHealingWeb />
      </div>
  );

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={handleClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-4xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(600px, 90vh, 850px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <div className="flex items-center space-x-3">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M21 8.25c0-2.485-2.099-4.5-4.688-4.5-1.935 0-3.597 1.126-4.312 2.733-.715-1.607-2.377-2.733-4.313-2.733C5.1 3.75 3 5.765 3 8.25c0 7.22 9 12 9 12s9-4.78 9-12z" /></svg>
            <h2 className="text-2xl font-bold text-blue-300">Health Hub</h2>
          </div>
          <button onClick={handleClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        
         <nav className="flex-shrink-0 border-b border-blue-500/20 px-6">
            <div className="flex space-x-4">
                <button onClick={() => setActiveTab('bioscan')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'bioscan' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Bio-Scan</button>
                <button onClick={() => setActiveTab('frequency')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'frequency' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Frequency Therapy</button>
                <button onClick={() => setActiveTab('web')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'web' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Healing Web</button>
            </div>
        </nav>
        
        <div className="flex-1 flex flex-col min-h-0">
          {activeTab === 'bioscan' && renderBioScan()}
          {activeTab === 'frequency' && renderFrequencyTherapy()}
          {activeTab === 'web' && renderHealingWeb()}
        </div>

        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default HealthHubModal;