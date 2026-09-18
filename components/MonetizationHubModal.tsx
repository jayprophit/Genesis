import React, { useState } from 'react';
import { User, MonetizationStrategy, Earning, CommissionLog, PlatformTreasury, PortfolioAsset, StakingPool, Vault, Airdrop } from '../types';

interface MonetizationHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  user: User | null;
  strategies: MonetizationStrategy[];
  earnings: Earning[];
  commissionLog: CommissionLog[];
  onStartVerification: () => void;
  platformTreasury: PlatformTreasury;
  userAssets: PortfolioAsset[];
  stakingPools: StakingPool[];
  vaults: Vault[];
  airdrops: Airdrop[];
}

const formatCurrency = (value: number, compact = false) => {
    return new Intl.NumberFormat('en-US', { 
        style: 'currency', 
        currency: 'USD',
        notation: compact ? 'compact' : 'standard',
        minimumFractionDigits: 2,
        maximumFractionDigits: 2,
    }).format(value);
};

const MetricCard: React.FC<{ title: string; value: string; color: string; }> = ({ title, value, color }) => (
    <div className="p-4 bg-gray-800/50 rounded-lg border-l-4" style={{ borderColor: color }}>
        <h4 className="text-sm text-gray-400">{title}</h4>
        <p className={`text-2xl font-bold ${color}`}>{value}</p>
    </div>
);

const ChangeIndicator: React.FC<{ value: number }> = ({ value }) => {
    const isPositive = value >= 0;
    return (
        <span className={`flex items-center text-sm font-semibold ${isPositive ? 'text-green-400' : 'text-red-400'}`}>
            {isPositive ? '▲' : '▼'} {value.toFixed(2)}%
        </span>
    );
};


const MonetizationHubModal: React.FC<MonetizationHubModalProps> = (props) => {
  const { isOpen, onClose, user, strategies, earnings, commissionLog, onStartVerification, platformTreasury, userAssets, stakingPools, vaults, airdrops } = props;
  const [activeTab, setActiveTab] = useState('dashboard');
  
  if (!isOpen) return null;

  const isVerified = user?.kycStatus === 'verified';

  const totalRevenue = strategies.reduce((sum, s) => sum + s.totalRevenue, 0);
  const totalCommission = commissionLog.reduce((sum, c) => sum + c.amount, 0);
  const netEarnings = totalRevenue - totalCommission;

  const renderDashboard = () => (
     <div className="space-y-6">
        <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">Earnings Dashboard</h3>
            <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                <MetricCard title="Total Revenue" value={formatCurrency(totalRevenue)} color="text-blue-400" />
                <MetricCard title="AI Commission" value={formatCurrency(totalCommission)} color="text-purple-400" />
                <MetricCard title="Your Net Earnings" value={formatCurrency(netEarnings)} color="text-green-400" />
            </div>
        </div>
        <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">Recent Transactions</h3>
            <div className="space-y-2">
                {earnings.map(earning => (
                     <div key={earning.id} className="p-3 bg-gray-800/50 rounded-md grid grid-cols-3 items-center gap-4 text-sm">
                        <p className="text-gray-300">{earning.description}</p>
                        <p className="text-gray-400 text-center">{new Date(earning.timestamp).toLocaleString()}</p>
                        <p className="font-semibold text-green-400 text-right">+{formatCurrency(earning.amount)}</p>
                    </div>
                ))}
            </div>
        </div>
    </div>
  );

  const renderStrategies = () => (
     <div>
        <div className="flex items-center justify-between mb-3">
             <h3 className="text-lg font-semibold text-blue-300">Monetization Strategies</h3>
             <button className="px-4 py-2 text-sm font-semibold rounded-md transition-colors bg-gray-700 hover:bg-gray-600 text-gray-200">
                Launch New Venture
            </button>
        </div>
        <div className="space-y-3">
            {strategies.map(strat => {
                const statusColor = strat.status === 'Active' ? 'bg-green-500/20 text-green-300' : 'bg-gray-500/20 text-gray-300';
                return(
                    <div key={strat.id} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                        <div className="flex items-center justify-between">
                            <p className="font-semibold text-white">{strat.name}</p>
                            <span className={`px-2 py-1 text-xs font-bold rounded-full ${statusColor}`}>{strat.status}</span>
                        </div>
                        <div className="flex items-center space-x-4 text-sm text-gray-400 mt-2">
                            <span>Type: <span className="text-gray-200">{strat.type}</span></span>
                            <span>Revenue: <span className="text-gray-200">{formatCurrency(strat.totalRevenue)}</span></span>
                            <span>Commission: <span className="text-gray-200">{strat.aiCommissionRate * 100}%</span></span>
                        </div>
                    </div>
                );
            })}
        </div>
    </div>
  );

  const renderPortfolio = () => (
    <div className="space-y-6">
        <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">Your Platform Assets</h3>
             <div className="space-y-2">
                {userAssets.map(asset => (
                    <div key={asset.symbol} className="p-3 bg-gray-800/50 rounded-md grid grid-cols-4 items-center gap-4 text-sm">
                        <div>
                            <p className="font-bold text-white">{asset.symbol}</p>
                            <p className="text-xs text-gray-400">{asset.name}</p>
                        </div>
                        <p className="font-semibold text-white text-right">{asset.amount.toLocaleString()}</p>
                        <p className="font-semibold text-white text-right">{formatCurrency(asset.value)}</p>
                        <div className="flex justify-end"><ChangeIndicator value={asset.dailyChange} /></div>
                    </div>
                ))}
            </div>
        </div>
        <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">Staking Pools</h3>
            {stakingPools.map(pool => (
                <div key={pool.id} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 flex items-center justify-between">
                    <div>
                        <p className="font-semibold text-white">{pool.name}</p>
                        <p className="text-sm text-gray-400">Stake {pool.assetSymbol} | TVL: {formatCurrency(pool.tvl, true)}</p>
                    </div>
                    <div className="text-right">
                        <p className="text-lg font-bold text-green-400">{pool.apr.toFixed(2)}% APR</p>
                        <button className="mt-1 px-4 py-2 text-sm font-semibold rounded-md transition-colors bg-blue-600 hover:bg-blue-500 text-white">Stake</button>
                    </div>
                </div>
            ))}
        </div>
         <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">DeFi Vaults</h3>
            {vaults.map(vault => (
                 <div key={vault.id} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 flex items-center justify-between">
                    <div>
                        <p className="font-semibold text-white">{vault.name}</p>
                        <p className="text-sm text-gray-400">Deposit {vault.assetSymbol} | TVL: {formatCurrency(vault.tvl, true)}</p>
                    </div>
                    <div className="text-right">
                        <p className="text-lg font-bold text-green-400">{vault.apy.toFixed(2)}% APY</p>
                        <button className="mt-1 px-4 py-2 text-sm font-semibold rounded-md transition-colors bg-gray-700 hover:bg-gray-600 text-gray-200">Deposit</button>
                    </div>
                </div>
            ))}
        </div>
        <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">Airdrops</h3>
            {airdrops.map(airdrop => (
                 <div key={airdrop.id} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 flex items-center justify-between">
                    <div>
                        <p className="font-semibold text-white">{airdrop.name}</p>
                        <p className="text-sm text-gray-400">{airdrop.description}</p>
                    </div>
                    <button disabled={!airdrop.claimable} className="px-4 py-2 text-sm font-semibold rounded-md transition-colors bg-purple-600 hover:bg-purple-500 text-white disabled:bg-gray-700 disabled:text-gray-400 disabled:cursor-not-allowed">
                        {airdrop.claimable ? 'Claim' : 'Ineligible'}
                    </button>
                </div>
            ))}
        </div>
    </div>
  );

  const renderTreasury = () => (
     <div className="space-y-6">
        <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">Treasury Overview</h3>
            <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                <p className="text-sm text-gray-400">Total Value</p>
                <p className="text-4xl font-bold text-white">{formatCurrency(platformTreasury.totalValue)}</p>
            </div>
        </div>
         <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">Asset Allocation</h3>
            <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                <div className="flex w-full h-8 rounded-full overflow-hidden">
                    {platformTreasury.allocation.map(asset => (
                        <div key={asset.asset} className="h-full" style={{width: `${asset.percentage}%`, backgroundColor: `hsl(${Math.random() * 360}, 70%, 50%)`}} title={`${asset.asset}: ${asset.percentage}%`}></div>
                    ))}
                </div>
                <div className="grid grid-cols-2 md:grid-cols-4 gap-4 mt-4 text-sm">
                    {platformTreasury.allocation.map(asset => (
                        <div key={asset.asset}>
                            <p className="font-semibold text-white">{asset.asset}</p>
                            <p className="text-gray-400">{asset.percentage}%</p>
                        </div>
                    ))}
                </div>
            </div>
        </div>
        <div>
            <h3 className="text-lg font-semibold text-blue-300 mb-3">Recent Treasury Activity</h3>
             <div className="space-y-2 text-sm">
                {platformTreasury.recentActivity.map((activity, i) => (
                    <div key={i} className="p-3 bg-gray-800/50 rounded-md flex justify-between items-center">
                        <p className="text-gray-300">{activity.description}</p>
                        <p className="text-gray-500">{new Date(activity.timestamp).toLocaleDateString()}</p>
                    </div>
                ))}
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
             <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M12 6v12m-3-2.818l.879.659c1.171.879 3.07.879 4.242 0 1.172-.879 1.172-2.303 0-3.182C13.536 11.21 12.77 10.5 12 10.5c-.77 0-1.536.71-2.121 1.482L9.879 12.5m-3.242 3.242a9 9 0 1012.728 0 9 9 0 00-12.728 0z" /></svg>
            <h2 className="text-2xl font-bold text-blue-300">Monetization Hub</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>

        <nav className="flex-shrink-0 border-b border-blue-500/20 px-6">
            <div className="flex space-x-4">
                <button onClick={() => setActiveTab('dashboard')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'dashboard' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Dashboard</button>
                <button onClick={() => setActiveTab('strategies')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'strategies' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Strategies</button>
                <button onClick={() => setActiveTab('portfolio')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'portfolio' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Portfolio & Staking</button>
                <button onClick={() => setActiveTab('treasury')} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 ${activeTab === 'treasury' ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>Platform Treasury</button>
            </div>
        </nav>

        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            {!isVerified ? (
                 <div className="p-4 h-full flex flex-col items-center justify-center text-center bg-yellow-600/10 border border-yellow-500/50 rounded-lg">
                    <p className="font-semibold text-yellow-200 text-lg">KYC Verification Required</p>
                    <p className="text-sm text-yellow-300/80 mt-2 max-w-sm">Monetization features require identity verification to comply with financial regulations and ensure a secure ecosystem.</p>
                    <button onClick={() => { onStartVerification(); onClose(); }} className="mt-6 px-4 py-2 bg-yellow-500 text-black text-sm rounded-md hover:bg-yellow-400 transition-colors font-semibold">
                        Verify Identity Now
                    </button>
                </div>
            ) : (
                <>
                    {activeTab === 'dashboard' && renderDashboard()}
                    {activeTab === 'strategies' && renderStrategies()}
                    {activeTab === 'portfolio' && renderPortfolio()}
                    {activeTab === 'treasury' && renderTreasury()}
                </>
            )}
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default MonetizationHubModal;