
import React from 'react';
import { Notification } from '../types';

interface NotificationsPageProps {
    notifications: Notification[];
}

const NotificationIcon: React.FC<{ type: Notification['type'] }> = ({ type }) => {
    switch (type) {
        case 'success':
            return <div className="w-8 h-8 rounded-full bg-green-500/20 flex items-center justify-center flex-shrink-0"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-green-400" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 13l4 4L19 7" /></svg></div>;
        case 'info':
            return <div className="w-8 h-8 rounded-full bg-blue-500/20 flex items-center justify-center flex-shrink-0"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-blue-400" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" /></svg></div>;
        case 'warning':
            return <div className="w-8 h-8 rounded-full bg-yellow-500/20 flex items-center justify-center flex-shrink-0"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-yellow-400" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" /></svg></div>;
        default:
            return null;
    }
}

const timeSince = (date: number) => {
    const seconds = Math.floor((Date.now() - date) / 1000);
    let interval = seconds / 31536000;
    if (interval > 1) return Math.floor(interval) + " years ago";
    interval = seconds / 2592000;
    if (interval > 1) return Math.floor(interval) + " months ago";
    interval = seconds / 86400;
    if (interval > 1) return Math.floor(interval) + " days ago";
    interval = seconds / 3600;
    if (interval > 1) return Math.floor(interval) + " hours ago";
    interval = seconds / 60;
    if (interval > 1) return Math.floor(interval) + " minutes ago";
    return Math.floor(seconds) + " seconds ago";
};


const NotificationsPage: React.FC<NotificationsPageProps> = ({ notifications }) => {

    return (
        <div className="w-full h-full p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
            <h1 className="text-2xl font-bold text-blue-300 mb-6">Notifications</h1>
            {notifications.length > 0 ? (
                <div className="max-w-3xl mx-auto space-y-3">
                    {notifications.map(notif => (
                        <div key={notif.id} className={`p-4 flex items-start space-x-4 transition-colors rounded-lg bg-gray-800/50 border border-gray-700/50 ${!notif.read ? 'border-blue-500/30' : ''}`}>
                            <NotificationIcon type={notif.type} />
                            <div className="flex-1">
                                <p className={`text-sm text-gray-300`}>{notif.text}</p>
                                <p className={`text-xs text-gray-400 mt-1`}>{timeSince(notif.timestamp)}</p>
                            </div>
                            {!notif.read && <div className="w-2.5 h-2.5 bg-blue-500 rounded-full flex-shrink-0 mt-1.5 ml-auto"></div>}
                        </div>
                    ))}
                </div>
            ) : (
                <div className="flex flex-col items-center justify-center h-full text-center text-gray-500">
                    <svg xmlns="http://www.w3.org/2000/svg" className="h-16 w-16 mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1}><path strokeLinecap="round" strokeLinejoin="round" d="M14.857 17.082a23.848 23.848 0 005.454-1.31A8.967 8.967 0 0118 9.75v-.7V9A6 6 0 006 9v.75a8.967 8.967 0 01-2.312 6.022c1.733.64 3.56 1.085 5.455 1.31m5.714 0a24.255 24.255 0 01-5.714 0m5.714 0a3 3 0 11-5.714 0" /></svg>
                    <h3 className="text-lg font-semibold">All caught up!</h3>
                    <p className="mt-1">You have no new notifications.</p>
                </div>
            )}
        </div>
    );
};

export default NotificationsPage;
