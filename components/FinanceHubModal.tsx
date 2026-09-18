import React, { useState } from 'react';
import { User, MarketIndex, PortfolioHolding, TokenomicsDetail, BlockchainStatus } from '../types';
import BlockchainForkTimer from './BlockchainForkTimer';

interface FinanceHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  user: User | null;
  onStartVerification: () => void;
  marketIndices: MarketIndex[];
  portfolio: {
    totalValue: number;
    change: number;
    changePercent: number;
    holdings: PortfolioHolding[];
  };
  qaiTokenomics: TokenomicsDetail;
  blockchainStatus: BlockchainStatus;
}

const formatCurrency = (value: number, digits = 2) => {
    return new Intl.NumberFormat('en-US', { style: 'currency', currency: 'USD', minimumFractionDigits: digits, maximumFractionDigits: digits }).format(value);
};

const formatPercent = (value: number) => {
    const sign = value > 0 ? '+' : '';
    return `${sign}${value.toFixed(2)}%`;
};

const ChangeIndicator: React.FC<{ value: number }> = ({ value }) => {
    const isPositive = value >= 0;
    return (
        <span className={`flex items-center text-sm font-semibold ${isPositive ? 'text-green-400' : 'text-red-400'}`}>
            {isPositive ? (
                <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4 mr-1" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-8.707l-3-3a1 1 0 00-1.414 0l-3 3a1 1 0 001.414 1.414L9 9.414V13a1 1 0 102 0V9.414l1.293 1.293a1 1 0 001.414-1.414z" clipRule="evenodd" /></svg>
            ) : (
                <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4 mr-1" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm-3.707-9.293l3-3a1 1 0 011.414 0l3 3a1 1 0 01-1.414 1.414L11 10.586V7a1 1 0 11-2 0v3.586L7.707 9.293a1 1 0 01-1.414-1.414z" clipRule="evenodd" /></svg>
            )}
            {formatPercent(value)}
        </span>
    );
};

const MetricCard: React.FC<{ title: string; value: string; color: string; }> = ({ title, value, color }) => (
    <div className={`p-4 bg-gray-800/50 rounded-lg border-l-4 ${color.replace('text-', 'border-')}`}>
        <h4 className="text-sm text-gray-400">{title}</h4>
        <p className={`text-2xl font-bold text-white`}>{value}</p>
    </div>
);

const FinanceHubModal: React.FC<FinanceHubModalProps> = ({ isOpen, onClose, user, onStartVerification, marketIndices, portfolio, qaiTokenomics, blockchainStatus }) => {
  const [activeTab, setActiveTab] = useState('overview');
  
  if (!isOpen) return null;

  const isVerified = user?.kycStatus === 'verified';
  
  const renderTokenomics = () => (
    <div className="space-y-6">
        <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">Blockchain Status</h3>
            <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                    <div>
                        <p className="text-sm text-gray-400">Current Version</p>
                        <p className="text-xl font-semibold text-white">{blockchainStatus.version}</p>
                    </div>
                    <div>
                        <p className="text-sm text-gray-400">Next Major Upgrade</p>
                        <p className="text-xl font-semibold text-white">{new Date(blockchainStatus.nextUpgradeDate).toLocaleDateString()}</p>
                    </div>
                </div>
                 <div className="mt-4 pt-4 border-t border-gray-700/50">
                    <p className="text-sm text-gray-400 text-center mb-2">Next Fork/Halving Event In:</p>
                    <BlockchainForkTimer targetDate={blockchainStatus.nextForkDate} />
                </div>
            </div>
        </div>
        <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">Key Metrics</h3>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                 <MetricCard title="Total Supply" value={qaiTokenomics.totalSupply.toLocaleString()} color="text-blue-400" />
                 <MetricCard title="Circulating Supply" value={qaiTokenomics.circulatingSupply.toLocaleString()} color="text-cyan-400" />
            </div>
        </div>
        
        <div>
             <h3 className="text-lg font-semibold text-blue-300 mb-3">Token Distribution</h3>
             <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                <div className="flex w-full h-8 rounded-full overflow-hidden mb-4">
                    {qaiTokenomics.distribution.map(d => (
                        <div key={d.category} className="h-full" style={{width: `${d.percentage}%`, backgroundColor: d.color}} title={`${d.category}: ${d.percentage}%`}></div>
                    ))}
                </div>
                <div className="grid grid-cols-2 md:grid-cols-3 gap-x-4 gap-y-2 text-sm">
                     {qaiTokenomics.distribution.map(d => (
                        <div key={d.category} className="flex items-center">
                            <div className="w-3 h-3 rounded-sm mr-2" style={{backgroundColor: d.color}}></div>
                            <span className="text-gray-300">{d.category}:</span>
                            <span className="font-semibold text-white ml-1">{d.percentage}%</span>
                        </div>
                    ))}
                </div>
             </div>
        </div>

        <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">Token Utility</h3>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                {qaiTokenomics.utility.map(util => (
                    <div key={util.useCase} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                        <h4 className="font-semibold text-white">{util.useCase}</h4>
                        <p className="text-sm text-gray-400 mt-1">{util.description}</p>
                    </div>
                ))}
            </div>
        </div>

         <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">Staking Rewards</h3>
            <div className="p-4 bg-gray-800/50 rounded-lg border border-green-500/30">
                <div className="flex items-center justify-between">
                    <div>
                        <p className="text-sm text-gray-400">Estimated Annual Percentage Rate (APR)</p>
                        <p className="text-3xl font-bold text-green-400">{qaiTokenomics.stakingRewards.apr.toFixed(2)}%</p>
                    </div>
                    <button className="px-4 py-2 text-sm font-semibold rounded-md transition-colors bg-blue-600 hover:bg-blue-500 text-white">Go to Staking</button>
                </div>
                <p className="text-xs text-gray-500 mt-2">{qaiTokenomics.stakingRewards.details}</p>
            </div>
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
             <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M2.25 18L9 11.25l4.306 4.307a11.95 11.95 0 015.814-5.519l2.74-1.22m0 0l-5.94-2.28m5.94 2.28l-2.28 5.941" /></svg>
            <h2 className="text-2xl font-bold text-blue-300">Finance Hub</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>

         <nav className="flex-shrink-0 border-b border-blue-500/20 px-6">
            <div className="flex space-x-4">
                <button onClick={() => setActiveTab('overview')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'overview' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Overview</button>
                <button onClick={() => setActiveTab('tokenomics')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'tokenomics' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>QAI Tokenomics</button>
            </div>
        </nav>

        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            {!isVerified && activeTab === 'overview' ? (
                 <div className="p-4 mb-6 bg-yellow-600/20 border border-yellow-500/50 rounded-lg text-center">
                    <p className="font-semibold text-yellow-200">KYC Verification Required</p>
                    <p className="text-sm text-yellow-300/80 mt-1">Portfolio management and trading features require identity verification.</p>
                    <button onClick={() => { onStartVerification(); onClose(); }} className="mt-3 px-4 py-2 bg-yellow-500 text-black text-sm rounded-md hover:bg-yellow-400 transition-colors font-semibold">
                        Verify Identity Now
                    </button>
                </div>
            ) : null}

            <div className={`${!isVerified && activeTab === 'overview' ? 'opacity-40 pointer-events-none' : ''}`}>
                {activeTab === 'overview' && (
                    <div className="space-y-6">
                        <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-3">Market Indices</h3>
                            <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                                {marketIndices.map(index => (
                                    <div key={index.symbol} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                                        <p className="font-semibold text-white">{index.symbol}</p>
                                        <p className="text-2xl font-bold text-white">{index.price.toFixed(2)}</p>
                                        <ChangeIndicator value={index.changePercent} />
                                    </div>
                                ))}
                            </div>
                        </div>
                        <div>
                            <h3 className="text-lg font-semibold text-blue-300 mb-3">Portfolio Overview</h3>
                            <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 mb-4">
                                <p className="text-sm text-gray-400">Total Value</p>
                                <p className="text-4xl font-bold text-white">{formatCurrency(portfolio.totalValue)}</p>
                                <div className="flex items-center space-x-2">
                                    <ChangeIndicator value={portfolio.changePercent} />
                                    <span className={portfolio.change >= 0 ? 'text-green-400' : 'text-red-400'}>({portfolio.change >= 0 ? '+' : ''}{formatCurrency(portfolio.change)}) Today</span>
                                </div>
                            </div>
                            <h4 className="font-semibold text-gray-300 mb-2">Holdings</h4>
                            <div className="space-y-2">
                                {portfolio.holdings.map(holding => (
                                    <div key={holding.symbol} className="p-3 bg-gray-800/50 rounded-md grid grid-cols-3 items-center gap-4">
                                        <div>
                                            <p className="font-bold text-white">{holding.symbol}</p>
                                            <p className="text-xs text-gray-400">{holding.type}</p>
                                        </div>
                                        <p className="font-semibold text-white text-right">{formatCurrency(holding.value)}</p>
                                        <div className="flex justify-end"><ChangeIndicator value={holding.changePercent} /></div>
                                    </div>
                                ))}
                            </div>
                        </div>
                    </div>
                )}
                {activeTab === 'tokenomics' && renderTokenomics()}
            </div>
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default FinanceHubModal;