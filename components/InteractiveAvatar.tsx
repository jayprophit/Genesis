import React from 'react';

interface InteractiveAvatarProps {
  state: 'idle' | 'thinking';
}

const InteractiveAvatar: React.FC<InteractiveAvatarProps> = ({ state }) => {
  const isThinking = state === 'thinking';

  return (
    <div className="w-32 h-32 md:w-48 md:h-48 flex items-center justify-center" style={{ perspective: '1000px' }}>
      <div className="relative w-full h-full" style={{ transformStyle: 'preserve-3d', animation: 'float 6s ease-in-out infinite' }}>
        {/* Core */}
        <div 
          className="absolute inset-1/4 rounded-full bg-gradient-to-br from-blue-400 to-purple-500 shadow-2xl shadow-blue-500/80"
          style={{ animation: 'pulse 3s ease-in-out infinite' }}
        ></div>

        {/* Inner Ring */}
        <div 
          className="absolute inset-0 border-2 border-blue-500/50 rounded-full"
          style={{ 
            transform: 'rotateX(75deg) rotateY(20deg)', 
            animation: `spin-slow ${isThinking ? '2s' : '8s'} linear infinite`
          }}
        ></div>
        
        {/* Middle Ring */}
        <div 
          className="absolute inset-[-10px] border border-cyan-400/40 rounded-full"
          style={{ 
            transform: 'rotateX(75deg) rotateY(-60deg)', 
            animation: `spin-slow ${isThinking ? '1.5s' : '6s'} linear infinite reverse`
          }}
        ></div>
        
        {/* Outer Ring */}
        <div 
          className="absolute inset-[-20px] border border-purple-500/30 rounded-full"
          style={{ 
            transform: 'rotateX(75deg) rotateY(100deg)', 
            animation: `spin-slow ${isThinking ? '1s' : '4s'} linear infinite`
          }}
        ></div>

         {/* Energy particles for thinking state */}
        {isThinking && (
          <div className="absolute inset-0">
             {[...Array(3)].map((_, i) => (
                <div key={i} className="absolute w-2 h-2 bg-cyan-300 rounded-full shadow-lg shadow-cyan-300" style={{
                    top: '50%',
                    left: '50%',
                    animation: `spin-fast ${1.5 + i * 0.2}s linear infinite`,
                    transformOrigin: `${60 + i * 10}px`,
                }}></div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
};

export default InteractiveAvatar;