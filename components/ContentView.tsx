import React, { useState, useEffect } from 'react';

const StatCard: React.FC<{ title: string; value: string; growth: string; icon: React.ReactNode; }> = ({ title, value, growth, icon }) => (
    <div className="bg-white/5 border border-white/10 p-4 rounded-lg backdrop-blur-sm">
        <div className="flex items-center justify-between">
            <p className="text-sm text-gray-400">{title}</p>
            <div className="text-green-400 text-xs font-bold bg-green-500/10 px-2 py-1 rounded-full">
                +{growth}
            </div>
        </div>
        <div className="flex items-end space-x-4 mt-2">
            <div className="text-cyan-400 opacity-80">{icon}</div>
            <p className="text-4xl font-semibold text-white">{value}</p>
        </div>
    </div>
);

const DashboardOrb: React.FC = () => (
    <div className="relative w-48 h-48 md:w-64 md:h-64">
        <div 
            className="absolute inset-0 rounded-full border-2 border-cyan-400/30 animate-spin-slow"
            style={{ animationDuration: '15s'}}
        />
        <div 
            className="absolute inset-[10%] rounded-full border border-cyan-400/30 animate-spin-slow"
            style={{ animationDirection: 'reverse', animationDuration: '10s'}}
        />
        <div 
            className="absolute inset-[20%] rounded-full border border-cyan-400/30 animate-spin-slow"
            style={{ animationDuration: '20s'}}
        />
        <div className="absolute inset-[30%] rounded-full bg-cyan-900/50 flex items-center justify-center animate-dashboard-grid-pulse overflow-hidden">
             <div className="absolute inset-0 opacity-50" style={{
                backgroundImage: 'linear-gradient(rgba(0, 255, 255, 0.5) 1px, transparent 1px), linear-gradient(to right, rgba(0, 255, 255, 0.5) 1px, transparent 1px)',
                backgroundSize: '1rem 1rem',
                animation: 'dashboard-orb-rotate 20s linear infinite',
            }}></div>
        </div>
    </div>
);

const ContentView: React.FC = () => {
    const [stats, setStats] = useState({
        users: 125420,
        courses: 60000,
        items: 400000,
        tokens: 50000000,
    });

    useEffect(() => {
        const interval = setInterval(() => {
            setStats(prev => ({
                users: prev.users + Math.floor(Math.random() * 5),
                courses: prev.courses + Math.floor(Math.random() * 3),
                items: prev.items + Math.floor(Math.random() * 10),
                tokens: prev.tokens + Math.floor(Math.random() * 20),
            }));
        }, 2000);
        return () => clearInterval(interval);
    }, []);

    return (
        <div className="h-full w-full p-6 md:p-8 overflow-y-auto scrollbar-thin scrollbar-thumb-white/10 scrollbar-track-transparent relative">
            <div className="absolute inset-0 opacity-20" style={{
                backgroundImage: 'linear-gradient(to right, rgba(59, 130, 246, 0.2) 1px, transparent 1px), linear-gradient(to bottom, rgba(59, 130, 246, 0.2) 1px, transparent 1px)',
                backgroundSize: '3rem 3rem'
            }}></div>
            <div className="flex flex-col items-center justify-center text-center relative z-10">
                <DashboardOrb />
                <h1 className="text-4xl md:text-5xl font-bold text-white mt-6">Welcome to QUANTUM OS</h1>
                <p className="text-gray-400 mt-2">The ultimate self-running platform combining E-Learning, E-Commerce, Job Marketplace, AI Tools and 18+ blockchain networks.</p>
                <button className="mt-6 bg-green-500/10 border border-green-400/50 text-green-300 font-semibold px-6 py-2 rounded-full flex items-center space-x-2 transition-transform hover:scale-105">
                    <div className="w-2 h-2 rounded-full bg-green-400 animate-pulse"></div>
                    <span>All Systems Operational</span>
                    <span className="text-green-500">99.99% Uptime</span>
                </button>

                <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mt-12 w-full max-w-6xl text-left">
                    <StatCard 
                        title="Total Users" 
                        value={stats.users.toLocaleString()}
                        growth="12.5% this month"
                        icon={<svg xmlns="http://www.w3.org/2000/svg" className="w-8 h-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M12 4.354a4 4 0 110 5.292M15 21H3v-1a6 6 0 016-6h6" /><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M21 13.5a4.5 4.5 0 11-9 0 4.5 4.5 0 019 0z" /></svg>}
                    />
                    <StatCard 
                        title="Active Courses" 
                        value={`${(stats.courses / 1000).toFixed(1)}K+`}
                        growth="9.0% this month"
                        icon={<svg xmlns="http://www.w3.org/2000/svg" className="w-8 h-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M12 14l9-5-9-5-9 5 9 5z" /><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M12 14l6.16-3.422a12.083 12.083 0 01.665 6.479A11.952 11.952 0 0012 20.055a11.952 11.952 0 00-6.824-2.998 12.078 12.078 0 01.665-6.479L12 14z" /></svg>}
                    />
                    <StatCard 
                        title="Marketplace Items" 
                        value={`${(stats.items / 1000).toFixed(1)}K+`}
                        growth="15.2% this month"
                        icon={<svg xmlns="http://www.w3.org/2000/svg" className="w-8 h-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M2.25 3h1.386c.51 0 .955.343 1.087.835l.383 1.437M7.5 14.25a3 3 0 00-3 3h15.75m-12.75-3h11.218c.51 0 .962.343 1.087.835l.383 1.437M7.5 14.25V5.625A2.625 2.625 0 0110.125 3h3.75c1.433 0 2.625 1.192 2.625 2.625v8.625" /></svg>}
                    />
                    <StatCard 
                        title="QOS Tokens" 
                        value={`${(stats.tokens / 1000000).toFixed(1)}M`}
                        growth="Staked"
                        icon={<svg xmlns="http://www.w3.org/2000/svg" className="w-8 h-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z" /><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M14.25 7.756a4.5 4.5 0 10-4.5 0m4.5 0l-1.5-1.5m-3 0l1.5-1.5m0 0l1.5 1.5m-1.5-1.5l-1.5 1.5" /></svg>}
                    />
                </div>
            </div>
        </div>
    );
}

export default ContentView;
