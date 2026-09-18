import React, { useState } from 'react';
import { User, PortfolioAsset, Beneficiary } from '../types';

interface WalletHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  user: User;
  userAssets: PortfolioAsset[];
  onUpdateBeneficiaries: (beneficiaries: Beneficiary[]) => void;
}

const formatCurrency = (value: number) => {
    return new Intl.NumberFormat('en-US', { style: 'currency', currency: 'USD' }).format(value);
};

const QRCodeStyling = ({ data }: { data: string }) => {
    const size = 200;
    const quietZone = 10;
    const matrix: boolean[][] = [];
    // Super simplified QR-like pattern generator for visual effect
    for (let y = 0; y < 21; y++) {
        matrix[y] = [];
        for (let x = 0; x < 21; x++) {
            const isQuietZone = x < 2 || x > 18 || y < 2 || y > 18;
            if (isQuietZone) {
                matrix[y][x] = false;
                continue;
            }
            matrix[y][x] = (x * y + (x ^ y)) % 3 === 0 || (x > 6 && x < 14 && y > 6 && y < 14) || x % 7 === 0 || y % 7 === 0;
        }
    }
    
    return (
        <svg width={size} height={size} viewBox={`-${quietZone} -${quietZone} ${size + quietZone*2} ${size + quietZone*2}`} className="bg-white p-2 rounded-lg">
            {matrix.map((row, y) => 
                row.map((cell, x) => 
                    cell && <rect key={`${y}-${x}`} x={x * 8} y={y * 8} width={8} height={8} fill="#000" />
                )
            )}
        </svg>
    )
};


const WalletHubModal: React.FC<WalletHubModalProps> = ({ isOpen, onClose, user, userAssets, onUpdateBeneficiaries }) => {
    const [activeTab, setActiveTab] = useState('wallet');
    const [newBeneficiary, setNewBeneficiary] = useState({ name: '', walletAddress: '', allocation: 0});
    
    if (!isOpen) return null;

    const handleAddBeneficiary = (e: React.FormEvent) => {
        e.preventDefault();
        const newB: Beneficiary = { ...newBeneficiary, id: `beneficiary-${Date.now()}`};
        onUpdateBeneficiaries([...user.beneficiaries, newB]);
        setNewBeneficiary({ name: '', walletAddress: '', allocation: 0});
    }

    const renderWallet = () => (
        <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
            <div className="space-y-4">
                <h3 className="text-lg font-semibold text-blue-300">Send Assets</h3>
                <form className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 space-y-4">
                     <div>
                        <label className="block text-sm font-medium text-gray-300 mb-1">Recipient Address</label>
                        <input type="text" placeholder="0x..." className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500 font-mono" />
                    </div>
                     <div className="flex gap-4">
                         <div className="flex-1">
                            <label className="block text-sm font-medium text-gray-300 mb-1">Amount</label>
                            <input type="number" placeholder="0.00" className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500" />
                        </div>
                        <div className="w-28">
                             <label className="block text-sm font-medium text-gray-300 mb-1">Asset</label>
                            <select className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500">
                                {userAssets.map(a => <option key={a.symbol}>{a.symbol}</option>)}
                            </select>
                        </div>
                     </div>
                     <div className="text-right">
                        <button type="submit" className="bg-blue-600 hover:bg-blue-500 text-white font-bold px-6 py-2 rounded-lg transition-colors">Send</button>
                    </div>
                </form>

                 <h3 className="text-lg font-semibold text-blue-300 pt-4">Tap to Pay (NFC)</h3>
                 <button className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 space-y-4 w-full flex flex-col items-center justify-center text-cyan-300 hover:bg-gray-800 transition-colors">
                    <div className="relative w-16 h-16 flex items-center justify-center">
                        <div className="absolute inset-0 rounded-full border-2 border-cyan-400/50 animate-ping"></div>
                        <svg xmlns="http://www.w3.org/2000/svg" className="w-8 h-8" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M8.25 7.5l.415-.207a.75.75 0 011.06 0l.415.207a.75.75 0 001.06 0l.415-.207a.75.75 0 011.06 0l.415.207a.75.75 0 001.06 0l.415-.207a.75.75 0 011.06 0l.415.207M8.25 12l.415-.207a.75.75 0 011.06 0l.415.207a.75.75 0 001.06 0l.415-.207a.75.75 0 011.06 0l.415.207a.75.75 0 001.06 0l.415-.207a.75.75 0 011.06 0l.415.207M8.25 16.5l.415-.207a.75.75 0 011.06 0l.415.207a.75.75 0 001.06 0l.415-.207a.75.75 0 011.06 0l.415.207a.75.75 0 001.06 0l.415-.207a.75.75 0 011.06 0l.415.207" /></svg>
                    </div>
                     <span className="mt-2 font-semibold">Initiate NFC Payment</span>
                 </button>
            </div>
             <div className="space-y-4 text-center">
                <h3 className="text-lg font-semibold text-blue-300">Receive Assets</h3>
                <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 flex flex-col items-center">
                    <QRCodeStyling data={user.walletAddress} />
                    <p className="mt-4 text-sm text-gray-400">Your Wallet Address:</p>
                    <p className="font-mono text-xs text-white break-all bg-gray-900/50 p-2 rounded-md">{user.walletAddress}</p>
                </div>
            </div>
        </div>
    );

    const renderInheritance = () => (
         <div className="space-y-6">
            <div>
                <h3 className="text-lg font-semibold text-blue-300 mb-2">Designated Beneficiaries</h3>
                {user.beneficiaries.length > 0 ? (
                    <div className="space-y-2">
                        {user.beneficiaries.map(b => (
                            <div key={b.id} className="p-3 bg-gray-800/50 rounded-lg border border-gray-700 flex items-center justify-between">
                                <div>
                                    <p className="font-semibold text-white">{b.name}</p>
                                    <p className="text-xs text-gray-400 font-mono">{b.walletAddress}</p>
                                </div>
                                <p className="text-lg font-bold text-blue-300">{b.allocation}%</p>
                            </div>
                        ))}
                    </div>
                ) : <p className="text-sm text-gray-500 text-center p-4">No beneficiaries designated.</p>}
            </div>
             <div>
                <h3 className="text-lg font-semibold text-blue-300 mb-2">Add New Beneficiary</h3>
                <form onSubmit={handleAddBeneficiary} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 space-y-4">
                     <div>
                        <label className="block text-sm font-medium text-gray-300 mb-1">Name / Relationship</label>
                        <input type="text" value={newBeneficiary.name} onChange={e => setNewBeneficiary(b => ({...b, name: e.target.value}))} placeholder="e.g., Eldest Child" className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2" required />
                    </div>
                     <div>
                        <label className="block text-sm font-medium text-gray-300 mb-1">Heir's Wallet Address</label>
                        <input type="text" value={newBeneficiary.walletAddress} onChange={e => setNewBeneficiary(b => ({...b, walletAddress: e.target.value}))} placeholder="0x..." className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2 font-mono" required />
                    </div>
                    <div>
                        <label className="block text-sm font-medium text-gray-300 mb-1">Asset Allocation (%)</label>
                        <input type="number" value={newBeneficiary.allocation} onChange={e => setNewBeneficiary(b => ({...b, allocation: Number(e.target.value)}))} min="1" max="100" className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2" required />
                    </div>
                    <p className="text-xs text-yellow-300/80 bg-yellow-600/10 p-2 rounded-md">Note: This action writes to a smart contract. Ensure all details are correct before submitting. This is a simulation.</p>
                     <div className="text-right">
                        <button type="submit" className="bg-blue-600 hover:bg-blue-500 text-white font-bold px-6 py-2 rounded-lg transition-colors">Authorize Heir</button>
                    </div>
                </form>
            </div>
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
             <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M21 12a2.25 2.25 0 00-2.25-2.25H5.25A2.25 2.25 0 003 12m18 0v6.243A2.25 2.25 0 0118.75 20.25H5.25A2.25 2.25 0 013 18.243V12m18 0l-2.66-2.66m0 0l-2.66 2.66m2.66-2.66v2.66m-2.66-2.66l2.66 2.66m-3-3.243a2.25 2.25 0 010-3.182l2.66-2.66m-2.66 2.66l-2.66-2.66" /></svg>
            <h2 className="text-2xl font-bold text-blue-300">Wallet & Payments</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>

         <nav className="flex-shrink-0 border-b border-blue-500/20 px-6">
            <div className="flex space-x-4">
                <button onClick={() => setActiveTab('wallet')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'wallet' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Wallet</button>
                <button onClick={() => setActiveTab('inheritance')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'inheritance' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Digital Will (Inheritance)</button>
                <button onClick={() => setActiveTab('history')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'history' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Transaction History</button>
            </div>
        </nav>

        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            {activeTab === 'wallet' && renderWallet()}
            {activeTab === 'inheritance' && renderInheritance()}
            {activeTab === 'history' && <p className="text-center text-gray-500">Transaction history is not yet available.</p>}
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default WalletHubModal;