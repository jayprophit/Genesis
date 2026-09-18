
import React, { useState, useEffect, useRef } from 'react';

interface TradingHubModalProps {
  isOpen: boolean;
  onClose: () => void;
}

const generatePriceData = (points = 50, volatility = 0.1, startPrice = 100) => {
    let price = startPrice;
    const data = [{ price }];
    for (let i = 1; i < points; i++) {
        price *= 1 + (Math.random() - 0.5) * volatility;
        price = Math.max(price, startPrice * 0.5); // Prevent going too low
        data.push({ price: parseFloat(price.toFixed(4)) });
    }
    return data;
};

const PriceChart: React.FC<{ data: { price: number }[], color: string }> = ({ data, color }) => {
    const svgRef = useRef<SVGSVGElement>(null);
    const [path, setPath] = useState('');

    useEffect(() => {
        if (!svgRef.current || data.length === 0) return;

        const svg = svgRef.current;
        const width = svg.clientWidth;
        const height = svg.clientHeight;
        const padding = 5;

        const maxPrice = Math.max(...data.map(p => p.price));
        const minPrice = Math.min(...data.map(p => p.price));

        const newPath = data
            .map((p, i) => {
                const x = (i / (data.length - 1)) * width;
                const y = height - ((p.price - minPrice) / (maxPrice - minPrice)) * (height - padding * 2) - padding;
                return `${i === 0 ? 'M' : 'L'} ${x.toFixed(2)} ${y.toFixed(2)}`;
            })
            .join(' ');
        setPath(newPath);
    }, [data]);

    return (
        <div className="h-full w-full relative">
            <svg ref={svgRef} className="w-full h-full overflow-visible">
                <path d={path} stroke={color} fill="none" strokeWidth="2" />
                <defs>
                    <linearGradient id="chart-gradient" x1="0" y1="0" x2="0" y2="1">
                        <stop offset="0%" stopColor={color} stopOpacity="0.3"/>
                        <stop offset="100%" stopColor={color} stopOpacity="0"/>
                    </linearGradient>
                </defs>
                <path d={`${path} L ${svgRef.current?.clientWidth || 0} ${svgRef.current?.clientHeight || 0} L 0 ${svgRef.current?.clientHeight || 0} Z`} fill="url(#chart-gradient)" />
            </svg>
        </div>
    );
};


const TradingHubModal: React.FC<TradingHubModalProps> = ({ isOpen, onClose }) => {
    const [activeTab, setActiveTab] = useState('crypto');
    const [cryptoData, setCryptoData] = useState(generatePriceData(100, 0.2, 65000));
    const [forexData, setForexData] = useState(generatePriceData(100, 0.02, 1.08));
    const [stockData, setStockData] = useState(generatePriceData(100, 0.05, 175));
    
    useEffect(() => {
        if(!isOpen) return;
        const interval = setInterval(() => {
            setCryptoData(prev => [...prev.slice(1), { price: prev[prev.length - 1].price * (1 + (Math.random() - 0.5) * 0.02) }]);
            setForexData(prev => [...prev.slice(1), { price: prev[prev.length - 1].price * (1 + (Math.random() - 0.5) * 0.001) }]);
            setStockData(prev => [...prev.slice(1), { price: prev[prev.length - 1].price * (1 + (Math.random() - 0.5) * 0.005) }]);
        }, 1000);
        return () => clearInterval(interval);
    }, [isOpen]);

    if (!isOpen) return null;

    const tabs: Record<string, { title: string; data: { price: number }[]; color: string; pair: string }> = {
        crypto: { title: 'Crypto', data: cryptoData, color: '#f97316', pair: 'BTC/USD' },
        forex: { title: 'Forex', data: forexData, color: '#3b82f6', pair: 'EUR/USD' },
        stocks: { title: 'Stocks', data: stockData, color: '#22c55e', pair: 'QAI Corp' },
    };
    
    const currentTabData = tabs[activeTab];

    return (
        <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
            <div
                className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-6xl m-4 text-white relative animate-fade-in flex flex-col"
                style={{ height: 'clamp(600px, 90vh, 900px)' }}
                onClick={(e) => e.stopPropagation()}
            >
                <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
                    <div className="flex items-center space-x-3">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M13 7h8m0 0v8m0-8l-8 8-4-4-6 6" /></svg>
                        <h2 className="text-2xl font-bold text-blue-300">Trading Hub</h2>
                    </div>
                    <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
                    </button>
                </header>

                <nav className="flex-shrink-0 border-b border-blue-500/20 px-6">
                    <div className="flex space-x-4">
                        {Object.keys(tabs).map(tabKey => (
                            <button key={tabKey} onClick={() => setActiveTab(tabKey)} className={`py-3 px-1 text-sm font-semibold transition-colors border-b-2 capitalize ${activeTab === tabKey ? 'border-blue-400 text-white' : 'border-transparent text-gray-400 hover:text-white'}`}>
                                {tabs[tabKey].title}
                            </button>
                        ))}
                    </div>
                </nav>

                <main className="flex-1 p-6 grid grid-cols-1 lg:grid-cols-3 gap-6 overflow-hidden">
                    <div className="lg:col-span-2 flex flex-col">
                        <div className="flex justify-between items-center mb-4">
                            <h3 className="text-xl font-semibold">{currentTabData.pair}</h3>
                            <p className="text-2xl font-bold" style={{color: currentTabData.color}}>{currentTabData.data[currentTabData.data.length -1].price.toLocaleString()}</p>
                        </div>
                        <div className="flex-1 bg-black/30 rounded-lg p-2">
                             <PriceChart data={currentTabData.data} color={currentTabData.color} />
                        </div>
                    </div>
                    <div className="lg:col-span-1 flex flex-col gap-4">
                        <div className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                             <h3 className="text-lg font-semibold text-blue-300 mb-3">Place Order</h3>
                             <form className="space-y-3">
                                <div className="grid grid-cols-2 gap-2">
                                    <button type="button" className="p-3 text-sm font-semibold rounded-md bg-green-600/80 hover:bg-green-600 transition-colors">BUY</button>
                                    <button type="button" className="p-3 text-sm font-semibold rounded-md bg-red-600/80 hover:bg-red-600 transition-colors">SELL</button>
                                </div>
                                <div>
                                    <label className="block text-xs font-medium text-gray-400 mb-1">Amount</label>
                                    <input type="number" placeholder="0.00" className="w-full bg-gray-800 border-gray-700 rounded-md px-3 py-2" />
                                </div>
                                <div>
                                     <label className="block text-xs font-medium text-gray-400 mb-1">Price</label>
                                    <input type="text" readOnly value={currentTabData.data[currentTabData.data.length -1].price.toFixed(4)} className="w-full bg-gray-900 border-gray-700 rounded-md px-3 py-2 font-mono" />
                                </div>
                                <button type="submit" className="w-full p-3 bg-blue-600 hover:bg-blue-500 rounded-md font-semibold transition-colors">Submit Order</button>
                             </form>
                        </div>
                         <div className="flex-1 p-4 bg-gray-800/50 rounded-lg border border-gray-700 flex flex-col">
                             <h3 className="text-lg font-semibold text-blue-300 mb-3">Order Book</h3>
                             <div className="flex-1 text-xs font-mono overflow-y-auto pr-2 scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                                {/* Mock order book data */}
                                {[...Array(10)].map((_, i) => <div key={i} className="flex justify-between text-red-400"><p>{(currentTabData.data[currentTabData.data.length - 1].price * (1.001 + i*0.0005)).toFixed(4)}</p><p>{(Math.random() * 10).toFixed(2)}</p></div>)}
                                <div className="my-2 py-1 border-y border-gray-700 text-center text-sm font-bold text-white">{currentTabData.data[currentTabData.data.length -1].price.toFixed(4)}</div>
                                {[...Array(10)].map((_, i) => <div key={i} className="flex justify-between text-green-400"><p>{(currentTabData.data[currentTabData.data.length - 1].price * (0.999 - i*0.0005)).toFixed(4)}</p><p>{(Math.random() * 10).toFixed(2)}</p></div>)}
                            </div>
                        </div>
                    </div>
                </main>
                <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
            </div>
        </div>
    );
};

export default TradingHubModal;
