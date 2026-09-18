import React from 'react';
import { AgentId } from '../types';
import Avatar from './Avatar';

interface AgentIconProps {
  agentId: AgentId;
  avatarUrl?: string;
  className?: string;
}

const AgentIcon: React.FC<AgentIconProps> = ({ agentId, avatarUrl, className = 'w-6 h-6' }) => {
  
  const FallbackIcon = () => {
    switch (agentId) {
        case 'quantum-ai':
        return (
            <svg xmlns="http://www.w3.org/2000/svg" className={className} fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor">
            <path strokeLinecap="round" strokeLinejoin="round" d="M9.53 16.122a3 3 0 00-5.78 1.128 2.25 2.25 0 01-2.4 2.245 4.5 4.5 0 008.4-2.245c0-.399-.078-.78-.22-1.128zm0 0a15.998 15.998 0 003.388-1.62m-5.043-.025a15.994 15.994 0 011.622-3.385m5.043.025a15.994 15.994 0 001.622 3.385m0 0a3 3 0 01-5.78-1.128 2.25 2.25 0 00-2.4-2.245 4.5 4.5 0 018.4 2.245c0 .399-.078-.78-.22 1.128zm0 0c-.234.462-.443.922-.68 1.385m-5.043-.025a15.994 15.994 0 011.622-3.385m5.043.025a15.994 15.994 0 001.622 3.385" />
            </svg>
        );
        default:
        return null;
    }
  };

  return (
    <Avatar 
      src={avatarUrl}
      alt="AI Avatar"
      className={`${className} rounded-full object-cover`}
      fallback={<FallbackIcon />}
    />
  );
};

export default AgentIcon;
