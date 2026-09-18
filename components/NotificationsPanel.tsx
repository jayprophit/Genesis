import React from 'react';
import { Notification } from '../types';

interface NotificationsPanelProps {
    notifications: Notification[];
    onClose: () => void;
    theme: 'light' | 'dark';
}

const NotificationIcon: React.FC<{ type: Notification['type'] }> = ({ type }) => {
    switch (type) {
        case 'success':
            return <div className="w-8 h-8 rounded-full bg-green-500/20 flex items-center justify-center"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-green-400" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 13l4 4L19 7" /></svg></div>;
        case 'info':
            return <div className="w-8 h-8 rounded-full bg-blue-500/20 flex items-center justify-center"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-blue-400" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" /></svg></div>;
        case 'warning':
            return <div className="w-8 h-8 rounded-full bg-yellow-500/20 flex items-center justify-center"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-yellow-400" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" /></svg></div>;
        default:
            return null;
    }
}

const timeSince = (date: number) => {
    const seconds = Math.floor((Date.now() - date) / 1000);
    let interval = seconds / 31536000;
    if (interval > 1) return Math.floor(interval) + "y ago";
    interval = seconds / 2592000;
    if (interval > 1) return Math.floor(interval) + "mo ago";
    interval = seconds / 86400;
    if (interval > 1) return Math.floor(interval) + "d ago";
    interval = seconds / 3600;
    if (interval > 1) return Math.floor(interval) + "h ago";
    interval = seconds / 60;
    if (interval > 1) return Math.floor(interval) + "m ago";
    return Math.floor(seconds) + "s ago";
};

const NotificationsPanel: React.FC<NotificationsPanelProps> = ({ notifications, onClose, theme }) => {
    
    const panelBg = theme === 'dark' ? 'bg-gray-800 border-blue-500/20' : 'bg-white border-gray-300';
    const itemHoverBg = theme === 'dark' ? 'hover:bg-gray-700/50' : 'hover:bg-gray-100';
    const textColor = theme === 'dark' ? 'text-gray-300' : 'text-gray-700';
    const subTextColor = theme === 'dark' ? 'text-gray-400' : 'text-gray-500';
    const headerTextColor = theme === 'dark' ? 'text-white' : 'text-black';

    return (
        <div className={`absolute right-0 mt-2 w-80 sm:w-96 ${panelBg} rounded-lg shadow-2xl z-20 animate-fade-in-fast overflow-hidden flex flex-col`} style={{maxHeight: '70vh'}}>
            <header className="p-4 border-b border-gray-700 flex justify-between items-center flex-shrink-0">
                <h3 className={`font-semibold ${headerTextColor}`}>Notifications</h3>
                <button onClick={onClose} className={`text-gray-400 hover:text-white`}>
                    <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
                </button>
            </header>
            <div className="flex-1 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                {notifications.length > 0 ? (
                    <ul>
                        {notifications.map(notif => (
                            <li key={notif.id} className={`p-4 flex items-start space-x-3 transition-colors ${itemHoverBg} ${!notif.read ? 'bg-blue-500/10' : ''}`}>
                                <div className="flex-shrink-0 mt-1"><NotificationIcon type={notif.type} /></div>
                                <div>
                                    <p className={`text-sm ${textColor}`}>{notif.text}</p>
                                    <p className={`text-xs ${subTextColor} mt-1`}>{timeSince(notif.timestamp)}</p>
                                </div>
                                {!notif.read && <div className="w-2.5 h-2.5 bg-blue-500 rounded-full flex-shrink-0 mt-1.5 ml-auto"></div>}
                            </li>
                        ))}
                    </ul>
                ) : (
                    <div className="p-8 text-center text-gray-500">
                        <p>No new notifications.</p>
                    </div>
                )}
            </div>
             <style>{`.animate-fade-in-fast { animation: fade-in-fast 0.15s ease-out forwards; } @keyframes fade-in-fast { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
        </div>
    );
};

export default NotificationsPanel;
