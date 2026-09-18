import React, { useState } from 'react';
import { Device, ContextEntry, ProactiveDirective } from '../types';

interface StrategicHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  devices: Device[];
  contextStream: ContextEntry[];
  directives: ProactiveDirective[];
}

const getDeviceIcon = (type: Device['type']) => {
    switch (type) {
        case 'Desktop': return <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9.75 17L9 20l-1 1h8l-1-1-.75-3M3 13h18M5 17h14a2 2 0 002-2V5a2 2 0 00-2-2H5a2 2 0 00-2 2v10a2 2 0 002 2z" /></svg>;
        case 'Phone': return <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 18h.01M8 21h8a2 2 0 002-2V5a2 2 0 00-2-2H8a2 2 0 00-2 2v14a2 2 0 002 2z" /></svg>;
        case 'Smart Glasses': return <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 20l-5.447-2.724A1 1 0 013 16.382V8.618a1 1 0 01.553-.894L9 4m0 16v-5h6v5m0 0l5.447-2.724A1 1 0 0021 16.382V8.618a1 1 0 00-.553-.894L15 4m-6 16h6" /></svg>;
        case 'Smart Watch': return <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M9 4.5v15m6-15v15" /><path strokeLinecap="round" strokeLinejoin="round" d="M5.25 6.75h13.5" /><path strokeLinecap="round" strokeLinejoin="round" d="M5.25 17.25h13.5" /><rect x="4.5" y="2.25" width="15" height="19.5" rx="4" /></svg>;
        case 'Smart Ring': return <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z" /><path strokeLinecap="round" strokeLinejoin="round" d="M12 15a3 3 0 110-6 3 3 0 010 6z" /></svg>;
        case 'Vehicle': return <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4" /></svg>;
        case 'Home Hub': return <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6" /></svg>;
    }
}

const StrategicHubModal: React.FC<StrategicHubModalProps> = ({ isOpen, onClose, devices, contextStream, directives }) => {
    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
            <div
                className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-6xl m-4 text-white relative animate-fade-in flex flex-col"
                style={{ height: 'clamp(600px, 90vh, 900px)' }}
                onClick={(e) => e.stopPropagation()}
            >
                <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
                    <div className="flex items-center space-x-3">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M3.75 6A2.25 2.25 0 016 3.75h2.25A2.25 2.25 0 0110.5 6v2.25a2.25 2.25 0 01-2.25 2.25H6a2.25 2.25 0 01-2.25-2.25V6zM3.75 15.75A2.25 2.25 0 016 13.5h2.25a2.25 2.25 0 012.25 2.25V18a2.25 2.25 0 01-2.25 2.25H6A2.25 2.25 0 013.75 18v-2.25zM13.5 6a2.25 2.25 0 012.25-2.25H18A2.25 2.25 0 0120.25 6v2.25A2.25 2.25 0 0118 10.5h-2.25a2.25 2.25 0 01-2.25-2.25V6zM13.5 15.75a2.25 2.25 0 012.25-2.25H18a2.25 2.25 0 012.25 2.25V18A2.25 2.25 0 0118 20.25h-2.25a2.25 2.25 0 01-2.25-2.25v-2.25z" /></svg>
                        <h2 className="text-2xl font-bold text-blue-300">Strategic Core</h2>
                    </div>
                    <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
                    </button>
                </header>
                <main className="flex-1 p-6 grid grid-cols-1 lg:grid-cols-3 gap-6 overflow-hidden">
                    {/* Left Column */}
                    <div className="lg:col-span-1 flex flex-col gap-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent pr-2">
                        <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-3">Device Mesh</h3>
                            <div className="space-y-3">
                                {devices.map(device => {
                                    const statusClasses = device.status === 'Connected' ? 'bg-green-500/20 text-green-300 border-green-500/30' : device.status === 'Standby' ? 'bg-yellow-500/20 text-yellow-300 border-yellow-500/30' : 'bg-red-500/20 text-red-300 border-red-500/30';
                                    return (
                                        <div key={device.id} className={`p-3 bg-gray-800/50 rounded-lg border flex items-center justify-between ${statusClasses}`}>
                                            <div className="flex items-center gap-3">
                                                {getDeviceIcon(device.type)}
                                                <div>
                                                    <p className="font-semibold text-white">{device.name}</p>
                                                    <p className="text-xs text-gray-400">{device.type}</p>
                                                </div>
                                            </div>
                                            <span className="text-xs font-bold">{device.status}</span>
                                        </div>
                                    );
                                })}
                            </div>
                        </div>
                         <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-3">Live Context Stream</h3>
                             <div className="h-64 p-3 bg-black/30 rounded-lg border border-gray-700/50 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                                <div className="font-mono text-xs text-cyan-300 space-y-2">
                                    {contextStream.map(entry => (
                                        <p key={entry.timestamp}>
                                            <span className="text-gray-500 mr-2">{new Date(entry.timestamp).toLocaleTimeString()}</span>
                                            {entry.text}
                                        </p>
                                    ))}
                                </div>
                            </div>
                        </div>
                    </div>
                     {/* Right Column */}
                    <div className="lg:col-span-2 flex flex-col gap-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent pr-2">
                        <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-3">Proactive Directives</h3>
                             <div className="space-y-3">
                                {directives.map(dir => (
                                     <div key={dir.id} className="p-3 bg-gray-800/50 rounded-lg border border-gray-700">
                                        <div className="flex justify-between items-start">
                                            <div>
                                                <p className="font-semibold text-gray-300">IF: <span className="text-white font-normal">{dir.trigger}</span></p>
                                                <p className="font-semibold text-cyan-300 mt-1">THEN: <span className="text-white font-normal">{dir.action}</span></p>
                                            </div>
                                             <label className="flex items-center cursor-pointer"><input type="checkbox" defaultChecked={dir.enabled} className="sr-only peer" /><div className="relative w-11 h-6 bg-gray-600 rounded-full peer peer-focus:ring-4 peer-focus:ring-blue-800 peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-0.5 after:start-[2px] after:bg-white after:border after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-blue-600"></div></label>
                                        </div>
                                    </div>
                                ))}
                                <button className="w-full mt-2 p-2 text-sm text-center bg-gray-700/50 hover:bg-gray-700 rounded-lg transition-colors">Add New Directive</button>
                            </div>
                        </div>
                        <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-3">AR/Holographic Interface Designer</h3>
                            <div className="h-64 p-4 bg-black/30 rounded-lg border border-dashed border-gray-700/50 flex items-center justify-center text-center text-gray-500">
                               <div className="space-y-2">
                                    <svg xmlns="http://www.w3.org/2000/svg" className="h-12 w-12 mx-auto" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1} d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" /><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1} d="M2.458 12C3.732 7.943 7.522 5 12 5c4.478 0 8.268 2.943 9.542 7-1.274 4.057-5.022 7-9.542 7-4.478 0-8.268-2.943-9.542-7z" /></svg>
                                    <p className="font-semibold">Simulated HUD</p>
                                    <p className="text-xs max-w-sm">Use natural language to design your interface. For example: "Show my vitals at the bottom left, comms log top right, and a minimap in the center."</p>
                               </div>
                            </div>
                        </div>
                    </div>
                </main>
                <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
            </div>
        </div>
    );
};

export default StrategicHubModal;