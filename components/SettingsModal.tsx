
import React, { useState } from 'react';
import { AppSettings, SyncService, AIBehaviorSettings, AITone, AIVerbosity, AICreativity, PrebuiltVoice } from '../types';

interface SettingsModalProps {
  isOpen: boolean;
  onClose: () => void;
  settings: AppSettings;
  onSettingsChange: (settings: AppSettings) => void;
  onResetOnboarding: () => void;
  isTtsEnabled: boolean;
  onToggleTts: () => void;
}

const complianceItems = {
  "Proof-of-Mechanisms": ["Proof of Work", "Proof of Stake", "Delegated Proof of Stake", "Proof of Activity", "Proof of Location", "Proof of Importance", "Proof of Elapsed Time", "Proof of Authority", "Proof of Burn", "Proof of Capacity", "Proof of Space", "Proof of Time Stake", "Proof of Brain", "Proof of History", "Proof of Contribution", "Proof of Impact", "Proof of Reputation", "Proof of Liquidity", "Proof of Affinity", "Proof of Chaos", "Proof of Discovery", "Proof of Engagement", "Proof of Identity", "Proof of Trust"],
  "Privacy & Data Regulations": ["GDPR", "CCPA", "PIPL", "Colorado Privacy Act", "PDPA", "Utah Consumers Privacy Act", "Australian Privacy Act", "HIPAA"],
  "Security Standards": ["ISO 27001", "PCI DSS", "GLBA", "SOC 2", "NIST Framework"]
};

const smartContracts = [
    { name: "Data Sovereignty Contract", description: "Ensures all user data is owned and controlled by the user via their private key.", status: "Active" },
    { name: "Cognitive Integrity Contract", description: "Governs the communication link between the Child Instance and the Genesis Core.", status: "Active" },
    { name: "Learning Path Agreement", description: "Defines the terms for generating and tracking personalized learning objectives.", status: "Active" },
    { name: "Interaction History Cache", description: "Manages the temporary local cache of conversations for seamless experience.", status: "Active" },
];


const ServiceCard: React.FC<{ name: string; description: string; icon: React.ReactNode; isConnected: boolean; onToggle: () => void; }> = ({ name, description, icon, isConnected, onToggle }) => (
    <div className="flex items-center justify-between p-4 bg-gray-800/50 rounded-lg border border-gray-700">
        <div className="flex items-center space-x-4">
            <div className="text-gray-400">{icon}</div>
            <div><h4 className="font-semibold text-white">{name}</h4><p className="text-sm text-gray-400">{description}</p></div>
        </div>
        <button onClick={onToggle} className={`px-4 py-2 text-sm font-semibold rounded-md transition-colors ${isConnected ? 'bg-red-600/20 text-red-300 hover:bg-red-600/30' : 'bg-blue-600 text-white hover:bg-blue-500'}`}>
            {isConnected ? 'Disconnect' : 'Connect'}
        </button>
    </div>
);

const DeviceCard: React.FC<{ name: string; id: string; icon: React.ReactNode; }> = ({ name, id, icon }) => (
    <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
        <div className="flex items-center justify-between">
            <div className="flex items-center space-x-4">
                <div className="text-gray-400">{icon}</div>
                <div>
                    <h4 className="font-semibold text-white">{name}</h4>
                    <p className="text-xs text-gray-500 font-mono">{id}</p>
                </div>
            </div>
             <div className="flex items-center space-x-2">
                <span className="text-xs text-green-400 flex items-center"><div className="w-2 h-2 bg-green-400 rounded-full mr-1.5 animate-pulse"></div>Authorized</span>
                <button className={`px-3 py-1.5 text-xs font-semibold rounded-md transition-colors bg-gray-700 hover:bg-gray-600 text-gray-300`}>
                    Configure
                </button>
            </div>
        </div>
    </div>
);

const BehaviorRadioGroup: React.FC<{ label: string; options: string[]; selected: string; onChange: (value: string) => void; }> = ({ label, options, selected, onChange }) => (
    <div>
        <h4 className="font-semibold text-gray-300 mb-2">{label}</h4>
        <div className="flex flex-wrap gap-2">
            {options.map(option => (
                <button
                    key={option}
                    onClick={() => onChange(option)}
                    className={`px-3 py-1.5 text-sm font-medium rounded-full transition-colors capitalize ${selected === option ? 'bg-blue-600 text-white' : 'bg-gray-700 hover:bg-gray-600 text-gray-300'}`}
                >
                    {option}
                </button>
            ))}
        </div>
    </div>
);

const SettingsModal: React.FC<SettingsModalProps> = ({ isOpen, onClose, settings, onSettingsChange, onResetOnboarding, isTtsEnabled, onToggleTts }) => {
  const [activeTab, setActiveTab] = useState('general');

  if (!isOpen) return null;

  const handleSyncToggle = (service: SyncService) => {
    const newSyncSettings = { ...settings.sync, [service]: !settings.sync[service] };
    onSettingsChange({ ...settings, sync: newSyncSettings });
  };
  
  const handleTimeoutChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    const newTimeout = parseInt(e.target.value, 10);
    onSettingsChange({ ...settings, sessionTimeout: newTimeout });
  };

  const handleBehaviorChange = (field: keyof AIBehaviorSettings, value: string) => {
    onSettingsChange({
      ...settings,
      aiBehavior: {
        ...settings.aiBehavior,
        [field]: value,
      }
    });
  };
  
  const maleVoices: PrebuiltVoice[] = ['Puck', 'Charon', 'Fenrir'];
  const femaleVoices: PrebuiltVoice[] = ['Kore', 'Zephyr'];

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-3xl m-4 text-white relative animate-fade-in flex flex-col" style={{ height: 'clamp(400px, 80vh, 700px)'}} onClick={(e) => e.stopPropagation()}>
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <h2 className="text-2xl font-bold text-blue-300">Settings</h2>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>

        <div className="flex-1 flex flex-col md:flex-row overflow-hidden">
            <nav className="w-full md:w-56 flex-shrink-0 border-b md:border-b-0 md:border-r border-blue-500/20 p-4">
                <ul className="space-y-1">
                    <li><button onClick={() => setActiveTab('general')} className={`w-full text-left px-3 py-2 rounded-md text-sm font-medium transition-colors ${activeTab === 'general' ? 'bg-blue-500/20 text-blue-300' : 'text-gray-400 hover:bg-gray-700/50'}`}>General</button></li>
                    <li><button onClick={() => setActiveTab('behavior')} className={`w-full text-left px-3 py-2 rounded-md text-sm font-medium transition-colors ${activeTab === 'behavior' ? 'bg-blue-500/20 text-blue-300' : 'text-gray-400 hover:bg-gray-700/50'}`}>AI Behavior</button></li>
                    <li><button onClick={() => setActiveTab('voice')} className={`w-full text-left px-3 py-2 rounded-md text-sm font-medium transition-colors ${activeTab === 'voice' ? 'bg-blue-500/20 text-blue-300' : 'text-gray-400 hover:bg-gray-700/50'}`}>Voice & Speech</button></li>
                    <li><button onClick={() => setActiveTab('sync')} className={`w-full text-left px-3 py-2 rounded-md text-sm font-medium transition-colors ${activeTab === 'sync' ? 'bg-blue-500/20 text-blue-300' : 'text-gray-400 hover:bg-gray-700/50'}`}>Cloud Sync</button></li>
                    <li><button onClick={() => setActiveTab('session')} className={`w-full text-left px-3 py-2 rounded-md text-sm font-medium transition-colors ${activeTab === 'session' ? 'bg-blue-500/20 text-blue-300' : 'text-gray-400 hover:bg-gray-700/50'}`}>Session & Security</button></li>
                    <li><button onClick={() => setActiveTab('iot')} className={`w-full text-left px-3 py-2 rounded-md text-sm font-medium transition-colors ${activeTab === 'iot' ? 'bg-blue-500/20 text-blue-300' : 'text-gray-400 hover:bg-gray-700/50'}`}>Automations & IoT</button></li>
                    <li><button onClick={() => setActiveTab('contracts')} className={`w-full text-left px-3 py-2 rounded-md text-sm font-medium transition-colors ${activeTab === 'contracts' ? 'bg-blue-500/20 text-blue-300' : 'text-gray-400 hover:bg-gray-700/50'}`}>Smart Contracts</button></li>
                    <li><button onClick={() => setActiveTab('trust')} className={`w-full text-left px-3 py-2 rounded-md text-sm font-medium transition-colors ${activeTab === 'trust' ? 'bg-blue-500/20 text-blue-300' : 'text-gray-400 hover:bg-gray-700/50'}`}>Trust & Compliance</button></li>
                </ul>
            </nav>
            <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                {activeTab === 'general' && (
                    <div className='space-y-6'>
                         <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-1">Onboarding Guide</h3>
                            <p className="text-sm text-gray-400 mb-4">Redo the initial tutorial to get a refresher on the key features.</p>
                            <button 
                                onClick={onResetOnboarding}
                                className="px-4 py-2 text-sm font-semibold rounded-md transition-colors bg-gray-700 hover:bg-gray-600 text-gray-200"
                            >
                                Reset Onboarding Guide
                            </button>
                        </div>
                        <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-1">Auto-Save Chats</h3>
                            <p className="text-sm text-gray-400 mb-4">Automatically save conversations to your browser's local storage as you chat.</p>
                            <label className="flex items-center cursor-pointer">
                                <input 
                                    type="checkbox" 
                                    checked={settings.autoSaveChats} 
                                    onChange={() => onSettingsChange({ ...settings, autoSaveChats: !settings.autoSaveChats })} 
                                    className="sr-only peer" 
                                />
                                <div className="relative w-11 h-6 bg-gray-600 rounded-full peer peer-focus:ring-4 peer-focus:ring-blue-800 peer-checked:after:translate-x-full after:content-[''] after:absolute after:top-0.5 after:start-[2px] after:bg-white after:border after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-blue-600"></div>
                                <span className="ms-3 text-sm font-medium text-gray-300">Enable Auto-Save</span>
                            </label>
                        </div>
                    </div>
                )}
                {activeTab === 'behavior' && (
                    <div className='space-y-6'>
                         <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-1">AI Personality</h3>
                            <p className="text-sm text-gray-400 mb-4">Adjust the default behavior of the AI to better suit your preferences. These settings can be overridden temporarily via response options.</p>
                        </div>
                        <BehaviorRadioGroup
                            label="Tone"
                            options={['neutral', 'formal', 'casual', 'enthusiastic']}
                            selected={settings.aiBehavior.tone}
                            onChange={(value) => handleBehaviorChange('tone', value as AITone)}
                        />
                         <BehaviorRadioGroup
                            label="Verbosity"
                            options={['concise', 'default', 'detailed']}
                            selected={settings.aiBehavior.verbosity}
                            onChange={(value) => handleBehaviorChange('verbosity', value as AIVerbosity)}
                        />
                        <BehaviorRadioGroup
                            label="Creativity"
                            options={['factual', 'balanced', 'imaginative']}
                            selected={settings.aiBehavior.creativity}
                            onChange={(value) => handleBehaviorChange('creativity', value as AICreativity)}
                        />
                    </div>
                )}
                {activeTab === 'voice' && (
                    <div className='space-y-6'>
                        <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-1">Text-to-Speech (TTS)</h3>
                            <p className="text-sm text-gray-400 mb-4">Enable the AI to speak its responses aloud.</p>
                            <label className="flex items-center cursor-pointer">
                                <input type="checkbox" checked={isTtsEnabled} onChange={onToggleTts} className="sr-only peer" />
                                <div className="relative w-11 h-6 bg-gray-600 rounded-full peer peer-focus:ring-4 peer-focus:ring-blue-800 peer-checked:after:translate-x-full after:content-[''] after:absolute after:top-0.5 after:start-[2px] after:bg-white after:border after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-blue-600"></div>
                                <span className="ms-3 text-sm font-medium text-gray-300">Enable Voice Responses</span>
                            </label>
                        </div>
                        <div className={!isTtsEnabled ? 'opacity-50 pointer-events-none transition-opacity' : 'transition-opacity'}>
                            <h3 className="text-lg font-semibold text-blue-300 mb-1">AI Voice Selection</h3>
                            <p className="text-sm text-gray-400 mb-4">Choose the voice for the AI assistant.</p>
                            <div className="space-y-4">
                                <fieldset>
                                    <legend className="font-semibold text-gray-300 mb-2">Female Voices</legend>
                                    <div className="flex flex-wrap gap-2">
                                        {femaleVoices.map(voice => (
                                            <label key={voice} className="flex items-center space-x-2 cursor-pointer">
                                                <input type="radio" name="ai-voice" value={voice} checked={settings.aiBehavior.voice === voice} onChange={(e) => handleBehaviorChange('voice', e.target.value as PrebuiltVoice)} className="hidden peer" />
                                                <span className="px-3 py-1.5 text-sm font-medium rounded-full transition-colors capitalize bg-gray-700 text-gray-300 peer-hover:bg-gray-600 peer-checked:bg-blue-600 peer-checked:text-white">{voice}</span>
                                            </label>
                                        ))}
                                    </div>
                                </fieldset>
                                <fieldset>
                                    <legend className="font-semibold text-gray-300 mb-2">Male Voices</legend>
                                    <div className="flex flex-wrap gap-2">
                                        {maleVoices.map(voice => (
                                            <label key={voice} className="flex items-center space-x-2 cursor-pointer">
                                                <input type="radio" name="ai-voice" value={voice} checked={settings.aiBehavior.voice === voice} onChange={(e) => handleBehaviorChange('voice', e.target.value as PrebuiltVoice)} className="hidden peer" />
                                                <span className="px-3 py-1.5 text-sm font-medium rounded-full transition-colors capitalize bg-gray-700 text-gray-300 peer-hover:bg-gray-600 peer-checked:bg-blue-600 peer-checked:text-white">{voice}</span>
                                            </label>
                                        ))}
                                    </div>
                                </fieldset>
                            </div>
                        </div>
                    </div>
                )}
                {activeTab === 'sync' && (
                    <div className='space-y-4'>
                        <ServiceCard name="Google Drive" description="Sync chat history and knowledge base." isConnected={settings.sync.google} onToggle={() => handleSyncToggle('google')} icon={<svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" viewBox="0 0 48 48"><path fill="#4285F4" d="M34.4 24l-6.5 11.3-6.6-11.3H11l13 22.5L37 24z"/><path fill="#34A853" d="M11 24l6.5-11.3L24 24l-6.5 11.3z"/><path fill="#FBBC04" d="M24 24l6.5-11.3L37 24l-6.5 11.3z"/><path fill="#EA4335" d="M24 24h13L24 1.5 11 24z"/></svg>} />
                        <ServiceCard name="Microsoft OneDrive" description="Save and load conversations." isConnected={settings.sync.microsoft} onToggle={() => handleSyncToggle('microsoft')} icon={<svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" viewBox="0 0 24 24" fill="#0072C6"><path d="M5.31 4.25h6.49c1.23 0 2.33.19 3.29.57 1.39.54 2.44 1.5 3.16 2.87.52 1 .78 2.11.78 3.32 0 1.23-.27 2.37-.81 3.41-.69 1.34-1.74 2.31-3.16 2.91-1.04.44-2.22.66-3.54.66h-1.6v3.25H5.31V4.25zm5.17 9.17h1.02c.7 0 1.31-.15 1.83-.45.74-.42 1.11-1.12 1.11-2.12 0-.96-.39-1.67-1.17-2.12-.48-.28-1.06-.42-1.74-.42h-.92v5.11z"/></svg>} />
                        <ServiceCard name="GitHub" description="Sync agent configurations as gists." isConnected={settings.sync.github} onToggle={() => handleSyncToggle('github')} icon={<svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" viewBox="0 0 24 24" fill="currentColor"><path d="M12 0c-6.626 0-12 5.373-12 12 0 5.302 3.438 9.8 8.207 11.387.599.111.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23.957-.266 1.983-.399 3.003-.404 1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.91 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576 4.765-1.589 8.199-6.086 8.199-11.386 0-6.627-5.373-12-12-12z"/></svg>} />
                    </div>
                )}
                {activeTab === 'session' && (
                     <div className='space-y-6'>
                         <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-1">Automatic Logout</h3>
                            <p className="text-sm text-gray-400 mb-4">For your security, you can choose to be logged out automatically after a period of inactivity.</p>
                            <div className="max-w-xs">
                                <label htmlFor="session-timeout" className="block text-sm font-medium text-gray-300 mb-2">Logout after:</label>
                                <select 
                                    id="session-timeout"
                                    value={settings.sessionTimeout} 
                                    onChange={handleTimeoutChange} 
                                    className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500"
                                >
                                    <option value="15">15 minutes</option>
                                    <option value="30">30 minutes</option>
                                    <option value="60">1 hour</option>
                                    <option value="0">Never</option>
                                </select>
                            </div>
                        </div>
                    </div>
                )}
                 {activeTab === 'iot' && (
                    <div className='space-y-4'>
                         <h3 className="text-lg font-semibold text-blue-300 mb-1">Manage Connected Devices</h3>
                         <p className="text-sm text-gray-400 mb-4">Authorize and configure devices for the AI's Actuator Core to control.</p>
                         <DeviceCard name="Smart Lamp" id="lab_light_1" icon={<svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" d="M9.75 3.104v5.714a2.25 2.25 0 01-.659 1.591L5 14.5M9.75 3.104c-.251.023-.501.05-.75.082m.75-.082a24.301 24.301 0 014.5 0m0 0v5.714c0 .597.237 1.17.659 1.591L19 14.5M12 14.5v-5.714c0-.597-.237-1.17-.659-1.591L7 3.104M12 14.5c0 0-3.032 3.032-3.032 3.032S6 21 6 21h12s.968-3.468.968-3.468S12 14.5 12 14.5z" /></svg>} />
                         <DeviceCard name="Robotic Arm" id="dev_arm_01" icon={<svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" d="M15.59 14.37a6 6 0 01-5.84 7.38v-4.82m5.84-2.56a17.96 17.96 0 00-5.84-2.56m0 0A17.965 17.965 0 0112 2.25c-2.996 0-5.74-1.1-7.75-2.922m9.5 14.25a17.966 17.966 0 01-9.5-14.25" /></svg>} />
                         <button className="w-full mt-4 flex items-center justify-center p-3 rounded-lg text-left transition-colors duration-200 bg-gray-700/50 hover:bg-gray-700 text-gray-300">
                             <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-5 h-5 mr-2"><path strokeLinecap="round" strokeLinejoin="round" d="M12 4.5v15m7.5-7.5h-15" /></svg>
                             Authorize New Device
                         </button>
                    </div>
                )}
                 {activeTab === 'contracts' && (
                    <div className='space-y-4'>
                         <h3 className="text-lg font-semibold text-blue-300 mb-1">Active Smart Contracts</h3>
                         <p className="text-sm text-gray-400 mb-4">These contracts govern the core functionality and data handling of your personalized AI instance.</p>
                         {smartContracts.map(contract => (
                            <div key={contract.name} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                                <div className="flex items-center justify-between">
                                    <h4 className="font-semibold text-white">{contract.name}</h4>
                                    <span className="text-xs text-green-400 bg-green-500/10 px-2 py-1 rounded-full">{contract.status}</span>
                                </div>
                                <p className="text-sm text-gray-400 mt-2">{contract.description}</p>
                            </div>
                         ))}
                    </div>
                )}
                {activeTab === 'trust' && (
                    <div className="space-y-6">
                        {Object.entries(complianceItems).map(([category, items]) => (
                            <div key={category}>
                                <h3 className="text-lg font-semibold text-blue-300 mb-3">{category}</h3>
                                <div className="space-y-2">
                                    {items.map(item => (
                                        <div key={item} className="flex items-center p-3 bg-gray-800/50 rounded-md">
                                            <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-green-400 mr-3 flex-shrink-0" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" /></svg>
                                            <span className="text-sm text-gray-300">{item}</span>
                                        </div>
                                    ))}
                                </div>
                            </div>
                        ))}
                    </div>
                )}
            </main>
        </div>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default SettingsModal;
