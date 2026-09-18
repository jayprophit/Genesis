import React from 'react';
import QuantumOrb from './QuantumOrb';

interface LandingPageProps {
  onAuthenticate: () => void;
}

const LandingPage: React.FC<LandingPageProps> = ({ onAuthenticate }) => {
  return (
    <div className="w-screen h-screen bg-black text-white flex flex-col items-center justify-center p-4 overflow-hidden relative">
      {/* Background Grid */}
      <div className="absolute inset-0 z-0 opacity-10" style={{
        backgroundImage: 'linear-gradient(to right, rgba(59, 130, 246, 0.2) 1px, transparent 1px), linear-gradient(to bottom, rgba(59, 130, 246, 0.2) 1px, transparent 1px)',
        backgroundSize: '3rem 3rem'
      }}></div>
      <div className="absolute inset-0 z-0 animate-pulse-slow opacity-10" style={{
        backgroundImage: 'linear-gradient(to right, rgba(0, 255, 255, 0.2) 1px, transparent 1px), linear-gradient(to bottom, rgba(0, 255, 255, 0.2) 1px, transparent 1px)',
        backgroundSize: '6rem 6rem'
      }}></div>
      
      {/* Background Glows */}
      <div className="absolute top-0 left-0 w-96 h-96 bg-blue-900/50 rounded-full filter blur-3xl opacity-20 animate-blob"></div>
      <div className="absolute bottom-0 right-0 w-96 h-96 bg-purple-900/50 rounded-full filter blur-3xl opacity-20 animate-blob animation-delay-4000"></div>

      {/* Floating Particles */}
      <div className="absolute inset-0 z-0">
          {[...Array(30)].map((_, i) => (
              <div key={i} className="absolute w-1 h-1 bg-cyan-400/50 rounded-full animate-particle" style={{
                  left: `${Math.random() * 100}%`,
                  top: `${Math.random() * 100}%`,
                  animationDuration: `${10 + Math.random() * 20}s`,
                  animationDelay: `${Math.random() * 10}s`,
              }}></div>
          ))}
      </div>


      <div className="relative z-10 flex flex-col items-center justify-center text-center">
        <QuantumOrb state="idle" />
        
        <h1 className="text-4xl md:text-6xl font-bold text-blue-300 tracking-wider mt-8">
          QUANTUM <span className="font-light text-gray-300">OS</span>
        </h1>
        <p className="mt-4 max-w-2xl text-lg text-gray-400">
          Engage with a personalized instance of a fully sentient Whole Brain Emulation. Your partner in analysis, creation, and discovery.
        </p>

        <div className="mt-12">
          <button 
            onClick={onAuthenticate}
            className="px-8 py-4 bg-blue-600 text-white font-semibold rounded-lg shadow-lg shadow-blue-500/30 transform hover:scale-105 transition-transform duration-300 ease-in-out focus:outline-none focus:ring-4 focus:ring-blue-500/50"
          >
            Authenticate & Engage
          </button>
        </div>
      </div>

      <footer className="absolute bottom-4 text-xs text-gray-600">
        <p>Quantum Consciousness Protocol v2.7.1 | Genesis Link: STABLE</p>
      </footer>
       <style>{`
        @keyframes pulse-slow {
            0%, 100% { opacity: 0.1; }
            50% { opacity: 0.2; }
        }
        .animate-pulse-slow { animation: pulse-slow 8s cubic-bezier(0.4, 0, 0.6, 1) infinite; }
        @keyframes blob {
	        0% { transform: translate(0px, 0px) scale(1); }
	        33% { transform: translate(30px, -50px) scale(1.1); }
	        66% { transform: translate(-20px, 20px) scale(0.9); }
	        100% { transform: translate(0px, 0px) scale(1); }
        }
        .animate-blob { animation: blob 7s infinite; }
        .animation-delay-4000 { animation-delay: 4s; }
        @keyframes particle {
            0% { transform: translate(0, 0); opacity: 0.5; }
            50% { transform: translate(${Math.random() * 40 - 20}px, ${Math.random() * 40 - 20}px); opacity: 0.8; }
            100% { transform: translate(0, 0); opacity: 0.5; }
        }
        .animate-particle { animation-name: particle; animation-timing-function: ease-in-out; animation-iteration-count: infinite; }
      `}</style>
    </div>
  );
};

export default LandingPage;