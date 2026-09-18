
import React, { useState, useEffect } from 'react';
import { User, AvatarConfig } from '../types';
import { presetAvatars } from '../avatar.config';

interface AvatarForgeModalProps {
  isOpen: boolean;
  onClose: () => void;
  user: User | null;
  onSave: (config: AvatarConfig) => void;
  onCapture: (imageDataUrl: string) => void;
}

const faceShapes = 6;
const hairstyles = 8;
const outfits = 4;

const defaultAvatarConfig: AvatarConfig = {
    base: 'androgynous',
    height: 180,
    build: 50,
    skinColor: '#a1887f',
    faceShape: 0,
    eyeColor: '#4fc3f7',
    hairstyle: 0,
    outfit: 0,
};

const AvatarForgeModal: React.FC<AvatarForgeModalProps> = ({ isOpen, onClose, user, onSave, onCapture }) => {
    const [config, setConfig] = useState<AvatarConfig>(user?.avatarConfig || defaultAvatarConfig);
    const [activeTab, setActiveTab] = useState('presets');

    useEffect(() => {
        if(user?.avatarConfig) {
            setConfig(user.avatarConfig);
        } else {
            setConfig(defaultAvatarConfig);
        }
    }, [user]);

    if (!isOpen) return null;

    const handleSave = () => {
        onSave(config);
        onClose();
    };
    
    const handleRandomize = () => {
        setConfig({
            base: ['masculine', 'feminine', 'androgynous'][Math.floor(Math.random() * 3)] as 'masculine' | 'feminine' | 'androgynous',
            height: Math.floor(Math.random() * (210 - 150 + 1) + 150),
            build: Math.floor(Math.random() * 101),
            skinColor: `#${Math.floor(Math.random()*16777215).toString(16).padStart(6, '0')}`,
            faceShape: Math.floor(Math.random() * faceShapes),
            eyeColor: `#${Math.floor(Math.random()*16777215).toString(16).padStart(6, '0')}`,
            hairstyle: Math.floor(Math.random() * hairstyles),
            outfit: Math.floor(Math.random() * outfits),
        });
    }

    const tabs = ['presets', 'identity', 'body', 'head', 'attire'];

    const renderTabContent = () => {
        switch(activeTab) {
            case 'presets':
                return (
                    <div>
                        <h4 className="font-semibold text-gray-300 mb-3">Select a Preset</h4>
                        <div className="grid grid-cols-2 gap-4">
                            {presetAvatars.map(preset => (
                                <button key={preset.name} onClick={() => setConfig(preset.config)} className="p-3 bg-gray-800/50 rounded-lg border border-gray-700 text-left hover:bg-gray-800 hover:border-blue-500/50 transition-all duration-200">
                                    <div className="w-full h-24 bg-gray-700 rounded-md mb-2 flex items-center justify-center">
                                        {/* Simplified preview */}
                                        <div className="w-12 h-12 rounded-full" style={{backgroundColor: preset.config.skinColor}}></div>
                                    </div>
                                    <h5 className="font-semibold text-white">{preset.name}</h5>
                                </button>
                            ))}
                        </div>
                    </div>
                );
            case 'identity':
                return (
                    <div className="space-y-4">
                        <h4 className="font-semibold text-gray-300">Base Model</h4>
                        <div className="grid grid-cols-3 gap-2">
                             {['Masculine', 'Feminine', 'Androgynous'].map(base => {
                                const baseValue = base.toLowerCase() as AvatarConfig['base'];
                                return <button key={base} onClick={() => setConfig(c => ({...c, base: baseValue}))} className={`p-3 text-sm font-semibold rounded-md transition-colors ${config.base === baseValue ? 'bg-blue-600 text-white' : 'bg-gray-700 hover:bg-gray-600'}`}>{base}</button>
                             })}
                        </div>
                    </div>
                );
            case 'body':
                return (
                    <div className="space-y-6">
                        <div>
                            <label className="flex justify-between text-sm font-medium text-gray-300 mb-2"><span>Height</span> <span className="text-blue-300">{config.height} cm</span></label>
                            <input type="range" min="150" max="210" value={config.height} onChange={e => setConfig(c => ({...c, height: Number(e.target.value)}))} className="w-full h-2 bg-gray-700 rounded-lg appearance-none cursor-pointer range-slider"/>
                        </div>
                         <div>
                            <label className="flex justify-between text-sm font-medium text-gray-300 mb-2"><span>Build</span> <span className="text-blue-300">{config.build}% Muscular</span></label>
                            <input type="range" min="0" max="100" value={config.build} onChange={e => setConfig(c => ({...c, build: Number(e.target.value)}))} className="w-full h-2 bg-gray-700 rounded-lg appearance-none cursor-pointer range-slider"/>
                        </div>
                        <div>
                            <label className="flex justify-between items-center text-sm font-medium text-gray-300 mb-2"><span>Skin Tone</span></label>
                            <div className="relative">
                                <input type="color" value={config.skinColor} onChange={e => setConfig(c => ({...c, skinColor: e.target.value}))} className="w-full h-10 p-1 bg-gray-700 rounded-md cursor-pointer color-input"/>
                            </div>
                        </div>
                    </div>
                );
            case 'head':
                 return (
                    <div className="space-y-6">
                        <div>
                            <label className="text-sm font-medium text-gray-300 mb-2 block">Face Shape</label>
                            <div className="grid grid-cols-4 gap-2">
                                {[...Array(faceShapes)].map((_, i) => <button key={i} onClick={() => setConfig(c => ({...c, faceShape: i}))} className={`h-16 rounded-md transition-colors border-2 ${config.faceShape === i ? 'border-blue-500 bg-blue-500/20' : 'border-gray-700 bg-gray-800 hover:border-gray-600'}`}></button>)}
                            </div>
                        </div>
                         <div>
                            <label className="text-sm font-medium text-gray-300 mb-2 block">Hairstyle</label>
                            <div className="grid grid-cols-4 gap-2">
                                {[...Array(hairstyles)].map((_, i) => <button key={i} onClick={() => setConfig(c => ({...c, hairstyle: i}))} className={`h-16 rounded-md transition-colors border-2 ${config.hairstyle === i ? 'border-blue-500 bg-blue-500/20' : 'border-gray-700 bg-gray-800 hover:border-gray-600'}`}></button>)}
                            </div>
                        </div>
                        <div>
                            <label className="flex justify-between items-center text-sm font-medium text-gray-300 mb-2"><span>Eye Color</span></label>
                            <div className="relative">
                                <input type="color" value={config.eyeColor} onChange={e => setConfig(c => ({...c, eyeColor: e.target.value}))} className="w-full h-10 p-1 bg-gray-700 rounded-md cursor-pointer color-input"/>
                            </div>
                        </div>
                    </div>
                );
            case 'attire':
                return (
                     <div>
                        <label className="text-sm font-medium text-gray-300 mb-2 block">Outfit</label>
                        <div className="grid grid-cols-2 gap-2">
                            {[...Array(outfits)].map((_, i) => <button key={i} onClick={() => setConfig(c => ({...c, outfit: i}))} className={`h-24 rounded-md transition-colors border-2 ${config.outfit === i ? 'border-blue-500 bg-blue-500/20' : 'border-gray-700 bg-gray-800 hover:border-gray-600'}`}></button>)}
                        </div>
                    </div>
                );
            default: return null;
        }
    }

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-6xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(600px, 90vh, 900px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <div className="flex items-center space-x-3">
             <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M16 7a4 4 0 11-8 0 4 4 0 018 0zM12 14a7 7 0 00-7 7h14a7 7 0 00-7-7z" /></svg>
            <h2 className="text-2xl font-bold text-blue-300">Avatar Forge</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        <main className="flex-1 flex flex-col md:flex-row overflow-hidden">
            {/* Viewport */}
            <div className="flex-1 p-4 bg-black/20 flex items-center justify-center relative overflow-hidden">
                <div className="absolute inset-0 z-0 opacity-20" style={{ backgroundImage: 'linear-gradient(to right, rgba(23, 53, 99, 0.5) 1px, transparent 1px), linear-gradient(to bottom, rgba(23, 53, 99, 0.5) 1px, transparent 1px)', backgroundSize: '20px 20px'}}></div>
                <img src="https://i.ibb.co/L5w2Wzy/avatar-placeholder.png" alt="Avatar Preview" className="max-h-full max-w-full object-contain z-10" style={{filter: 'grayscale(0.5)'}} />
            </div>

            {/* Controls */}
            <aside className="w-full md:w-96 p-4 border-t md:border-t-0 md:border-l border-blue-500/20 flex flex-col overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                <nav className="flex-shrink-0 mb-4">
                    <div className="grid grid-cols-5 gap-1 p-1 bg-gray-800 rounded-lg">
                        {tabs.map(tab => (
                            <button key={tab} onClick={() => setActiveTab(tab)} className={`px-2 py-2 text-xs sm:text-sm font-semibold rounded-md transition-colors capitalize ${activeTab === tab ? 'bg-blue-600 text-white' : 'text-gray-400 hover:bg-gray-700'}`}>{tab}</button>
                        ))}
                    </div>
                </nav>
                <div className="flex-1">
                    {renderTabContent()}
                </div>
                <div className="flex-shrink-0 mt-4 pt-4 border-t border-blue-500/20 grid grid-cols-2 gap-2">
                    <button onClick={handleRandomize} className="w-full p-3 bg-gray-700 hover:bg-gray-600 rounded-md font-semibold transition-colors">Randomize</button>
                    <button onClick={handleSave} className="w-full p-3 bg-blue-600 hover:bg-blue-500 rounded-md font-semibold transition-colors">Save & Close</button>
                </div>
            </aside>
        </main>
        <style>{`
            .animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }
            .range-slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 16px; height: 16px; background: #3b82f6; border-radius: 50%; cursor: pointer; }
            .range-slider::-moz-range-thumb { width: 16px; height: 16px; background: #3b82f6; border-radius: 50%; cursor: pointer; border: none; }
            .color-input::-webkit-color-swatch-wrapper { padding: 0; }
            .color-input::-webkit-color-swatch { border: none; border-radius: 0.25rem; }
            .color-input::-moz-color-swatch { border: none; border-radius: 0.25rem; }
        `}</style>
      </div>
    </div>
  );
};

export default AvatarForgeModal;
