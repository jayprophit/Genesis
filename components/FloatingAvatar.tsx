
import React from 'react';
import { Agent } from '../types';
import QuantumOrb from './QuantumOrb';

interface FloatingAvatarProps {
    orbState: 'idle' | 'thinking' | 'speaking' | 'listening';
    agent: Agent;
}

const FloatingAvatar: React.FC<FloatingAvatarProps> = ({ orbState, agent }) => {
    // Only show the floating avatar on larger screens where it won't obstruct content
    return (
        <div className="hidden xl:block fixed bottom-4 right-4 z-50 pointer-events-none">
             <div className="w-40 h-40 scale-75">
                <QuantumOrb state={orbState} />
             </div>
        </div>
    );
};

export default FloatingAvatar;
