import React, { useState } from 'react';
import { MonitoredSite, DeliveryServiceResult } from '../types';

interface DistributionChainHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  monitoredSites: MonitoredSite[];
}

const formatCurrency = (value: number) => {
    return new Intl.NumberFormat('en-US', { style: 'currency', currency: 'USD' }).format(value);
};

const DistributionChainHubModal: React.FC<DistributionChainHubModalProps> = ({ isOpen, onClose, monitoredSites }) => {
    const [activeTab, setActiveTab] = useState('automation');
    const [deliveryQuery, setDeliveryQuery] = useState({ origin: '', destination: '', serviceType: 'Standard' });
    const [deliveryResults, setDeliveryResults] = useState<DeliveryServiceResult[] | null>(null);
    const [isSearching, setIsSearching] = useState(false);

    if (!isOpen) return null;

    const handleFindServices = (e: React.FormEvent) => {
        e.preventDefault();
        setIsSearching(true);
        setDeliveryResults(null);
        // Simulate API call
        setTimeout(() => {
            setDeliveryResults([
                { providerName: 'QuantumLeap Logistics', price: 150.75, eta: '2 hours' },
                { providerName: 'Veridia Express', price: 185.50, eta: '1.5 hours' },
                { providerName: 'SameDay Couriers', price: 220.00, eta: '45 minutes' },
            ]);
            setIsSearching(false);
        }, 1500);
    };
    
    const renderSupplyChainAutomation = () => (
        <div className="space-y-6">
            {monitoredSites.map(site => (
                <div key={site.id} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                    <h4 className="font-semibold text-lg text-white">{site.name}</h4>
                    <p className="text-sm text-gray-400 mb-4">{site.location}</p>
                    <div className="space-y-4">
                        {site.materials.map(material => {
                            const levelPercent = material.currentLevel;
                            const barColor = levelPercent > material.reorderThreshold + 10 ? 'bg-green-500' : levelPercent > material.reorderThreshold ? 'bg-yellow-500' : 'bg-red-500';
                            let statusColor = 'text-gray-400';
                            if (material.status === 'Ordering...') statusColor = 'text-blue-400 animate-pulse';
                            else if (levelPercent <= material.reorderThreshold) statusColor = 'text-red-400';

                            return (
                                <div key={material.name}>
                                    <div className="flex justify-between items-center text-sm mb-1">
                                        <span className="font-medium text-gray-300">{material.name}</span>
                                        <span className={statusColor}>{material.status}</span>
                                    </div>
                                    <div className="w-full bg-gray-700/50 rounded-full h-2.5">
                                        <div className={`${barColor} h-2.5 rounded-full transition-all duration-500`} style={{width: `${levelPercent}%`}}></div>
                                    </div>
                                    <p className="text-xs text-gray-500 mt-1 text-right">{levelPercent.toFixed(1)}% Capacity</p>
                                </div>
                            );
                        })}
                    </div>
                </div>
            ))}
        </div>
    );
    
    const renderServiceProcurement = () => (
        <div className="space-y-6">
            <form onSubmit={handleFindServices} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 space-y-4">
                <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                     <div>
                        <label className="block text-sm font-medium text-gray-300 mb-1">Origin</label>
                        <input type="text" value={deliveryQuery.origin} onChange={e => setDeliveryQuery(q => ({...q, origin: e.target.value}))} placeholder="e.g., Warehouse A" className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500" required />
                    </div>
                     <div>
                        <label className="block text-sm font-medium text-gray-300 mb-1">Destination</label>
                        <input type="text" value={deliveryQuery.destination} onChange={e => setDeliveryQuery(q => ({...q, destination: e.target.value}))} placeholder="e.g., Quantum Tower Site" className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500" required />
                    </div>
                </div>
                <div>
                    <label className="block text-sm font-medium text-gray-300 mb-1">Service Type</label>
                    <select value={deliveryQuery.serviceType} onChange={e => setDeliveryQuery(q => ({...q, serviceType: e.target.value}))} className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500">
                        <option>Same-day</option>
                        <option>Standard</option>
                        <option>Freight</option>
                    </select>
                </div>
                <div className="text-right">
                    <button type="submit" disabled={isSearching} className="bg-blue-600 hover:bg-blue-500 text-white font-bold px-6 py-2 rounded-lg transition-colors disabled:opacity-50 disabled:cursor-not-allowed">
                        {isSearching ? 'Searching...' : 'Find Services'}
                    </button>
                </div>
            </form>

            {isSearching && (
                 <div className="text-center p-8">
                    <div className="w-8 h-8 border-4 border-dashed border-blue-500/50 rounded-full animate-spin-slow mx-auto"></div>
                    <p className="mt-4 text-gray-400">Scanning local logistics networks...</p>
                </div>
            )}

            {deliveryResults && (
                <div>
                    <h4 className="font-semibold text-lg text-white mb-2">Available Services</h4>
                    <div className="space-y-3">
                        {deliveryResults.map(result => (
                             <div key={result.providerName} className="p-3 bg-gray-800/50 rounded-lg border border-gray-700 grid grid-cols-3 items-center gap-4">
                                <p className="font-semibold text-gray-200">{result.providerName}</p>
                                <p className="text-xl font-bold text-green-400 text-center">{formatCurrency(result.price)}</p>
                                <p className="text-gray-300 text-right">ETA: {result.eta}</p>
                            </div>
                        ))}
                    </div>
                </div>
            )}
        </div>
    );

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-4xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(500px, 90vh, 850px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <div className="flex items-center space-x-3">
             <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M6.75 7.5l3 2.25-3 2.25m4.5 0h3m-9 8.25h13.5A2.25 2.25 0 0021 18V6a2.25 2.25 0 00-2.25-2.25H5.25A2.25 2.25 0 003 6v12a2.25 2.25 0 002.25 2.25z" /></svg>
            <h2 className="text-2xl font-bold text-blue-300">Distribution Chain Hub</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>

        <nav className="flex-shrink-0 border-b border-blue-500/20 px-6">
            <div className="flex space-x-4">
                <button onClick={() => setActiveTab('automation')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'automation' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Supply Chain Automation</button>
                <button onClick={() => setActiveTab('procurement')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'procurement' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Service Procurement</button>
            </div>
        </nav>

        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            {activeTab === 'automation' && renderSupplyChainAutomation()}
            {activeTab === 'procurement' && renderServiceProcurement()}
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default DistributionChainHubModal;