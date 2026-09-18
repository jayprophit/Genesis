import React from 'react';

const MiniAvatar: React.FC = () => (
    <div className="w-8 h-8 md:w-10 md:h-10 flex-shrink-0 rounded-full bg-gradient-to-tr from-blue-500 to-purple-600 flex items-center justify-center shadow-lg shadow-blue-500/30 relative overflow-hidden">
        <div className="absolute inset-0 bg-black/20"></div>
        <div className="w-3 h-3 rounded-full bg-cyan-300/80 shadow-inner"></div>
         <div className="absolute w-full h-1/2 top-0 left-0 bg-gradient-to-b from-white/20 to-transparent"></div>
    </div>
);

export default MiniAvatar;
