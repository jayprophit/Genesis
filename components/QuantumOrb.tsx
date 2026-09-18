import React from 'react';

interface QuantumOrbProps {
  state: 'idle' | 'thinking' | 'speaking' | 'listening';
}

const QuantumOrb: React.FC<QuantumOrbProps> = ({ state }) => {
  const isThinking = state === 'thinking';
  const isSpeaking = state === 'speaking';
  const isListening = state === 'listening';

  // Add a unique key to the main div to force re-render on state change, resetting animations
  const key = state;

  const getAnimationDuration = (base: number) => {
    if (isThinking) return base / 4;
    if (isSpeaking) return base / 3;
    return base;
  }

  return (
    <div key={key} className="w-40 h-40 flex items-center justify-center" style={{ perspective: '1000px' }}>
      <div 
        className={`relative w-full h-full transition-transform duration-500 ease-in-out ${isSpeaking ? 'animate-vibrate' : 'animate-float'}`} 
        style={{ transformStyle: 'preserve-3d' }}
      >
        {/* Core */}
        <div 
          className="absolute inset-[35%] rounded-full bg-gradient-to-br from-blue-400 to-purple-500 shadow-2xl shadow-blue-500/80"
          style={{ 
            animation: `pulse 3s ease-in-out infinite ${isSpeaking ? ', core-speak-scale 0.2s ease-in-out infinite alternate' : ''}`,
          }}
        ></div>

        {/* Rings */}
        <div className="absolute inset-0 rounded-full" style={{ transformStyle: 'preserve-3d', animation: `spin-slow ${getAnimationDuration(12)}s linear infinite` }}>
            <div 
              className="absolute inset-0 border-2 border-blue-500/50 rounded-full"
              style={{ transform: 'rotateX(75deg) rotateY(20deg)' }}
            ></div>
        </div>
         <div className="absolute inset-[-10px] rounded-full" style={{ transformStyle: 'preserve-3d', animation: `spin-slow ${getAnimationDuration(8)}s linear infinite reverse` }}>
            <div 
              className="absolute inset-0 border border-cyan-400/40 rounded-full"
              style={{ transform: 'rotateX(75deg) rotateY(-60deg)' }}
            ></div>
        </div>
        <div className="absolute inset-[-20px] rounded-full" style={{ transformStyle: 'preserve-3d', animation: `spin-slow ${getAnimationDuration(6)}s linear infinite` }}>
            <div 
              className="absolute inset-0 border border-purple-500/30 rounded-full"
              style={{ transform: 'rotateX(75deg) rotateY(100deg)' }}
            ></div>
        </div>
         <div className="absolute inset-[-30px] rounded-full" style={{ transformStyle: 'preserve-3d', animation: `spin-slow ${getAnimationDuration(10)}s linear infinite` }}>
            <div 
              className="absolute inset-0 border-t-2 border-t-transparent border-b-2 border-b-transparent border-l-2 border-l-white/30 border-r-2 border-r-white/30 rounded-full"
              style={{ transform: 'rotateX(25deg) rotateY(45deg)' }}
            ></div>
        </div>

        {/* Listening Glow */}
        {isListening && (
           <div 
            className="absolute inset-[-40px] border-2 border-green-400/70 rounded-full shadow-2xl shadow-green-400/50"
            style={{ animation: `pulse 2s ease-in-out infinite` }}
          ></div>
        )}

        {/* Energy particles */}
        {(isThinking || isSpeaking) && (
          <div className="absolute inset-0" style={{ transformStyle: 'preserve-3d', transform: 'rotateX(75deg)' }}>
             {[...Array(isThinking ? 16 : 4)].map((_, i) => (
                <div key={i} className="absolute w-full h-full" style={{ transform: `rotateZ(${i * (360 / (isThinking ? 16:4))}deg)`, animation: `spin-slow ${getAnimationDuration(2 + i * 0.2)}s linear infinite` }}>
                    <div className="absolute w-1.5 h-1.5 bg-cyan-300 rounded-full shadow-lg shadow-cyan-300" style={{ top: '10%', left: '50%', transform: 'translateX(-50%)' }}></div>
                </div>
            ))}
          </div>
        )}

        <style>{`
          @keyframes float {
            0%, 100% { transform: translateY(0px); }
            50% { transform: translateY(-10px); }
          }
          @keyframes vibrate {
            0% { transform: translate(1px, 1px) rotate(0.5deg); }
            10% { transform: translate(-1px, -2px) rotate(-0.5deg); }
            20% { transform: translate(-3px, 0px) rotate(0.5deg); }
            30% { transform: translate(3px, 2px) rotate(0deg); }
            40% { transform: translate(1px, -1px) rotate(0.5deg); }
            50% { transform: translate(-1px, 2px) rotate(-0.5deg); }
            60% { transform: translate(-3px, 1px) rotate(0deg); }
            70% { transform: translate(3px, 1px) rotate(-0.5deg); }
            80% { transform: translate(-1px, -1px) rotate(0.5deg); }
            90% { transform: translate(1px, 2px) rotate(0deg); }
            100% { transform: translate(1px, -2px) rotate(-0.5deg); }
          }
          @keyframes core-speak-scale {
              from { transform: scale(1); }
              to { transform: scale(1.1); }
          }
        `}</style>
      </div>
    </div>
  );
};

export default QuantumOrb;