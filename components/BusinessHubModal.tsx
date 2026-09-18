import React from 'react';
import { User, SubAgent, Financials } from '../types';

interface BusinessHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  user: User | null;
  onStartVerification: () => void;
  subAgents: SubAgent[];
  financials: Financials;
  onDeploySubAgent: () => void;
}

const formatCurrency = (value: number) => {
    return new Intl.NumberFormat('en-US', { style: 'currency', currency: 'USD', minimumFractionDigits: 0, maximumFractionDigits: 0 }).format(value);
};

const FinancialCard: React.FC<{ title: string; value: string; color: string; }> = ({ title, value, color }) => (
    <div className="p-4 bg-gray-800/50 rounded-lg border-l-4" style={{ borderColor: color }}>
        <h4 className="text-sm text-gray-400">{title}</h4>
        <p className="text-2xl font-bold text-white">{value}</p>
    </div>
);

const AgentCard: React.FC<{ agent: SubAgent }> = ({ agent }) => {
    const statusColor = agent.status === 'Active' ? 'bg-green-500/20 text-green-300' : 'bg-yellow-500/20 text-yellow-300';
    return (
        <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 flex items-center justify-between">
            <div>
                <p className="font-semibold text-white">{agent.name} <span className="text-sm text-gray-400 font-normal">- {agent.role}</span></p>
                <p className="text-xs text-gray-500 font-mono mt-1">Task: {agent.currentTask}</p>
            </div>
            <span className={`px-2 py-1 text-xs font-bold rounded-full ${statusColor}`}>{agent.status}</span>
        </div>
    );
};


const BusinessHubModal: React.FC<BusinessHubModalProps> = ({ isOpen, onClose, user, onStartVerification, subAgents, financials, onDeploySubAgent }) => {
  if (!isOpen) return null;

  const isVerified = user?.kycStatus === 'verified';

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div 
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-4xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(500px, 90vh, 850px)'}}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <div className="flex items-center space-x-3">
             <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}>
                <path strokeLinecap="round" strokeLinejoin="round" d="M20 7l-8-4-8 4m16 0l-8 4m8-4v10l-8 4m0-10L4 7m8 4v10M4 7v10l8 4" />
             </svg>
            <h2 className="text-2xl font-bold text-blue-300">Business Hub</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            <p className="text-gray-400 mb-6">Manage your autonomous business operations. Deploy sub-agents, oversee finances, and execute corporate actions.</p>
            
            {!isVerified && (
                 <div className="p-4 mb-6 bg-yellow-600/20 border border-yellow-500/50 rounded-lg text-center">
                    <p className="font-semibold text-yellow-200">KYC Verification Required</p>
                    <p className="text-sm text-yellow-300/80 mt-1">Advanced financial and corporate features require identity verification.</p>
                    <button onClick={() => { onStartVerification(); onClose(); }} className="mt-3 px-4 py-2 bg-yellow-500 text-black text-sm rounded-md hover:bg-yellow-400 transition-colors font-semibold">
                        Verify Identity Now
                    </button>
                </div>
            )}

            <div className={`space-y-6 ${!isVerified ? 'opacity-40 pointer-events-none' : ''}`}>
                <div>
                    <h3 className="text-lg font-semibold text-blue-300 mb-3">Financial Overview</h3>
                    <div className="grid grid-cols-2 md:grid-cols-3 gap-4">
                        <FinancialCard title="Total Revenue" value={formatCurrency(financials.revenue)} color="#3b82f6" />
                        <FinancialCard title="Net Profit" value={formatCurrency(financials.profit)} color="#22c55e" />
                        <FinancialCard title="Total Expenses" value={formatCurrency(financials.expenses)} color="#ef4444" />
                        <FinancialCard title="Tax Liability" value={formatCurrency(financials.taxLiability)} color="#eab308" />
                        <FinancialCard title="Capital Gains" value={formatCurrency(financials.capitalGains)} color="#8b5cf6" />
                        <FinancialCard title="Dividends Paid" value={formatCurrency(financials.dividendsPaid)} color="#14b8a6" />
                    </div>
                </div>

                <div>
                    <div className="flex items-center justify-between mb-3">
                         <h3 className="text-lg font-semibold text-blue-300">Sub-Agent Roster</h3>
                         <button onClick={onDeploySubAgent} className="px-4 py-2 text-sm font-semibold rounded-md transition-colors bg-gray-700 hover:bg-gray-600 text-gray-200">
                            Deploy New Agent
                        </button>
                    </div>
                    <div className="space-y-3">
                        {subAgents.map(agent => <AgentCard key={agent.id} agent={agent} />)}
                    </div>
                </div>

                <div>
                    <h3 className="text-lg font-semibold text-blue-300 mb-3">Corporate Actions</h3>
                     <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                         <button className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 text-left hover:bg-gray-800 transition-colors">
                             <h4 className="font-semibold text-white">Calculate Taxes</h4>
                             <p className="text-sm text-gray-400 mt-1">Run a full tax simulation for the current fiscal period.</p>
                         </button>
                          <button className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 text-left hover:bg-gray-800 transition-colors">
                             <h4 className="font-semibold text-white">Distribute Dividends</h4>
                             <p className="text-sm text-gray-400 mt-1">Authorize and distribute profits to stakeholders.</p>
                         </button>
                          <button className="p-4 bg-gray-800/50 rounded-lg border border-gray-700 text-left hover:bg-gray-800 transition-colors">
                             <h4 className="font-semibold text-white">Generate Financial Report</h4>
                             <p className="text-sm text-gray-400 mt-1">Compile a comprehensive report of all financial activities.</p>
                         </button>
                     </div>
                </div>

            </div>
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default BusinessHubModal;