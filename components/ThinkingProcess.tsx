
import React from 'react';
import { Agent } from '../types';
import MiniAvatar from './MiniAvatar';
import Avatar from './Avatar';

interface ThinkingProcessProps {
  agent: Agent;
}

const ThinkingProcess: React.FC<ThinkingProcessProps> = ({ agent }) => {
  return (
    <div className="flex items-end gap-3 justify-start">
       <Avatar
        src={agent?.avatarUrl}
        alt="AI Avatar"
        className="w-8 h-8 md:w-10 md:h-10 rounded-full object-cover flex-shrink-0 shadow-lg"
        fallback={<MiniAvatar />}
      />
      <div className="max-w-xs md:max-w-md lg:max-w-2xl px-4 py-3 rounded-2xl shadow-md bg-gray-800 rounded-bl-none">
        <div className="flex items-center space-x-2">
            <div className="w-2 h-2 bg-blue-400 rounded-full animate-pulse" style={{animationDelay: '0s'}}></div>
            <div className="w-2 h-2 bg-blue-400 rounded-full animate-pulse" style={{animationDelay: '0.2s'}}></div>
            <div className="w-2 h-2 bg-blue-400 rounded-full animate-pulse" style={{animationDelay: '0.4s'}}></div>
        </div>
      </div>
    </div>
  );
};

export default ThinkingProcess;
