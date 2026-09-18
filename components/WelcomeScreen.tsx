
import React from 'react';
import { Agent } from '../types';
import QuantumOrb from './QuantumOrb';

interface WelcomeScreenProps {
    agent: Agent;
    theme: 'light' | 'dark';
}

const WelcomeScreen: React.FC<WelcomeScreenProps> = ({ agent, theme }) => (
    <div className="h-full flex flex-col items-center justify-center text-center p-8">
        <QuantumOrb state="idle" />
        <h2 className={`text-2xl md:text-3xl font-bold ${theme === 'dark' ? 'text-gray-200' : 'text-gray-800'} mt-4`}>{agent.name}</h2>
        <p className={`${theme === 'dark' ? 'text-gray-400' : 'text-gray-600'} mt-2 max-w-md`}>{agent.welcomeMessage.replace('{userName}', 'User')}</p>
    </div>
);

export default WelcomeScreen;
