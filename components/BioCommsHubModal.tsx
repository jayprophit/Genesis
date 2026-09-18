

import React, { useState, useEffect, useRef } from 'react';
import { lifeForms, Species, LifeFormCategory } from '../lifeforms.config';
import BioSignalVisualizer from './BioSignalVisualizer';
import { getBioCommsGreeting, getBioCommsTranslation } from '../services/geminiService';

interface BioCommsHubModalProps {
  isOpen: boolean;
  onClose: () => void;
}

interface BioCommsMessage {
    sender: 'user' | 'lifeform' | 'system';
    text: string;
}

const BioCommsHubModal: React.FC<BioCommsHubModalProps> = ({ isOpen, onClose }) => {
    const [view, setView] = useState<'selection' | 'connecting' | 'connected'>('selection');
    const [selectedCategory, setSelectedCategory] = useState<LifeFormCategory | null>(null);
    const [selectedSpecies, setSelectedSpecies] = useState<Species | null>(null);
    const [messages, setMessages] = useState<BioCommsMessage[]>([]);
    const [userInput, setUserInput] = useState('');
    const [isTranslating, setIsTranslating] = useState(false);
    const messagesEndRef = useRef<HTMLDivElement>(null);

    const scrollToBottom = () => {
        messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
    }

    useEffect(scrollToBottom, [messages]);

    const handleSelectSpecies = async (species: Species) => {
        setSelectedSpecies(species);
        setView('connecting');
        setMessages([{ sender: 'system', text: `Attempting to establish a quantum-entangled communication link with ${species.name}...`}]);

        try {
            const greeting = await getBioCommsGreeting(species.name);
            setTimeout(() => {
                setView('connected');
                setMessages([
                    { sender: 'system', text: `Connection successful. Signal is stable.`},
                    { sender: 'lifeform', text: greeting }
                ]);
            }, 2500);
        } catch (e) {
            setMessages([{ sender: 'system', text: `Connection failed. Unable to resolve bio-signature.`}]);
            setTimeout(() => setView('selection'), 3000);
        }
    };

    const handleDisconnect = () => {
        setView('selection');
        setSelectedCategory(null);
        setSelectedSpecies(null);
        setMessages([]);
        onClose();
    };

    const handleSendMessage = async (e: React.FormEvent) => {
        e.preventDefault();
        if (!userInput.trim() || !selectedSpecies || isTranslating) return;

        const userMsg: BioCommsMessage = { sender: 'user', text: userInput };
        setMessages(prev => [...prev, userMsg]);
        setUserInput('');
        setIsTranslating(true);
        
        try {
            const translation = await getBioCommsTranslation(selectedSpecies.name, userInput);
            const lifeformMsg: BioCommsMessage = { sender: 'lifeform', text: translation };
            setMessages(prev => [...prev, lifeformMsg]);
        } catch (e) {
             setMessages(prev => [...prev, {sender: 'system', text: '[Signal interference detected. Translation failed.]'}]);
        } finally {
            setIsTranslating(false);
        }
    };

    const renderSelectionView = () => (
        <div className="p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            <h3 className="text-lg font-semibold text-blue-300 mb-4">Select a Lifeform Category</h3>
            <div className="grid grid-cols-2 gap-4">
                {lifeForms.map(category => (
                    <button key={category.name} onClick={() => setSelectedCategory(category)} className={`p-4 bg-gray-800/50 rounded-lg border text-left hover:bg-gray-800 hover:border-blue-500/50 transition-all duration-200 ${selectedCategory?.name === category.name ? 'border-blue-500 ring-2 ring-blue-500/50' : 'border-gray-700'}`}>
                        <h4 className="font-semibold text-lg text-white">{category.name}</h4>
                    </button>
                ))}
            </div>
            {selectedCategory && (
                <div className="mt-6">
                    <h3 className="text-lg font-semibold text-blue-300 mb-4">Select a Species</h3>
                     <div className="grid grid-cols-3 gap-4">
                         {selectedCategory.species.map(species => (
                            <button key={species.name} onClick={() => handleSelectSpecies(species)} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 text-left hover:bg-gray-800 hover:border-blue-500/50 transition-all duration-200 text-center">
                                <p className="text-4xl">{species.icon}</p>
                                <h4 className="font-semibold text-white mt-2">{species.name}</h4>
                                <p className="text-xs text-gray-400 mt-1">{species.description}</p>
                            </button>
                         ))}
                     </div>
                </div>
            )}
        </div>
    );
    
    const renderConnectingView = () => (
        <div className="flex flex-col items-center justify-center h-full text-center p-6">
            <div className="w-16 h-16 border-4 border-dashed border-blue-500/50 rounded-full animate-spin-slow"></div>
            <p className="text-cyan-300 font-mono mt-6">{messages[0]?.text}</p>
        </div>
    );

    const renderConnectedView = () => (
        <div className="flex-1 flex flex-col min-h-0">
            <div className="p-4 border-b border-blue-500/20 flex justify-between items-center">
                <div className="flex items-center gap-3">
                    <p className="text-4xl">{selectedSpecies?.icon}</p>
                    <div>
                        <h3 className="text-lg font-semibold text-white">Connected to: {selectedSpecies?.name}</h3>
                        <p className="text-sm text-green-400">Signal: Strong</p>
                    </div>
                </div>
                 <button onClick={handleDisconnect} className="px-4 py-2 text-sm font-semibold rounded-md transition-colors bg-red-600/20 text-red-300 hover:bg-red-600/30">Disconnect</button>
            </div>
            <div className="h-48 flex-shrink-0 bg-black/30">
                <BioSignalVisualizer isActive={true} lifeformType={selectedSpecies!.type} />
            </div>
            <div className="flex-1 p-4 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent space-y-4">
                {messages.map((msg, i) => (
                    <div key={i} className={`flex items-end gap-2 ${msg.sender === 'user' ? 'justify-end' : 'justify-start'}`}>
                        {msg.sender === 'lifeform' && <div className="text-2xl flex-shrink-0">{selectedSpecies?.icon}</div>}
                        <div className={`max-w-md px-4 py-2 rounded-lg ${
                            msg.sender === 'user' ? 'bg-blue-600 text-white' : 
                            msg.sender === 'lifeform' ? 'bg-gray-700 text-gray-200 italic' :
                            'bg-transparent text-gray-500 text-xs text-center w-full'
                        }`}>
                            {msg.text}
                        </div>
                    </div>
                ))}
                {isTranslating && <div className="flex items-end gap-2 justify-start"><div className="text-2xl flex-shrink-0">{selectedSpecies?.icon}</div><div className="px-4 py-2 text-gray-400">...</div></div>}
                <div ref={messagesEndRef} />
            </div>
            <form onSubmit={handleSendMessage} className="p-4 border-t border-blue-500/20 flex gap-2">
                <input type="text" value={userInput} onChange={e => setUserInput(e.target.value)} placeholder="Send message..." className="w-full bg-gray-800 border-gray-700 rounded-lg px-4 py-3" disabled={isTranslating} />
                <button type="submit" className="bg-blue-600 hover:bg-blue-500 text-white font-bold p-3 rounded-lg disabled:opacity-50" disabled={isTranslating || !userInput.trim()}>Send</button>
            </form>
        </div>
    );

    let content;
    switch(view) {
        case 'connecting': content = renderConnectingView(); break;
        case 'connected': content = renderConnectedView(); break;
        default: content = renderSelectionView();
    }

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
            <div className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-4xl m-4 text-white relative animate-fade-in flex flex-col" style={{ height: 'clamp(600px, 90vh, 850px)' }} onClick={(e) => e.stopPropagation()}>
                <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
                    <div className="flex items-center space-x-3">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}>
                            <path strokeLinecap="round" strokeLinejoin="round" d="M15.75 6a3.75 3.75 0 11-7.5 0 3.75 3.75 0 017.5 0z" />
                            <path strokeLinecap="round" strokeLinejoin="round" d="M4.501 20.118a7.5 7.5 0 0114.998 0A17.933 17.933 0 0112 21.75c-2.676 0-5.216-.584-7.499-1.632z" />
                            <path strokeLinecap="round" strokeLinejoin="round" d="M12.008 12.008a.75.75 0 100-1.5.75.75 0 000 1.5z" />
                            <path strokeLinecap="round" strokeLinejoin="round" d="M15 11.25a3 3 0 01-6 0" />
                            <path strokeLinecap="round" strokeLinejoin="round" d="M18 9.75a6 6 0 01-12 0" />
                        </svg>
                        <h2 className="text-2xl font-bold text-blue-300">Bio-Comms Hub</h2>
                    </div>
                    <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors"><svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg></button>
                </header>
                {content}
                <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
            </div>
        </div>
    );
};

export default BioCommsHubModal;