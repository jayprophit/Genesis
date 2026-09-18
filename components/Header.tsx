
import React, { useState, useRef, useEffect, useMemo } from 'react';
import { Hub, ActionName, User, Notification, ChatSession, AppSettings } from '../types';
import Search from './Search';
import NotificationsPanel from './NotificationsPanel';
import ProfileDropdown from './ProfileDropdown';

interface HeaderProps {
  isSidebarExpanded: boolean;
  onToggleSidebar: () => void;
  onNewChat: () => void;
  onToggleTheme: () => void;
  onForkChat: () => void;
  isForking: boolean;
  onSummarizeChat: () => void;
  isSummarizing: boolean;
  onSaveChat: () => void;
  isSaving: boolean;
  settings: AppSettings;
  theme: 'light' | 'dark';
  activeChat: ChatSession | null | undefined;
  selectedHub: Hub | null;
  hubActions: Record<ActionName, () => void>;
  user: User;
  onDisconnectWallet: () => void;
  onAvatarUpload: (imageDataUrl: string) => void;
  onOpenSettings: () => void;
  onStartVerification: () => void;
  onViewKey: () => void;
  notifications: Notification[];
  isSystemStatusVisible: boolean;
  onToggleSystemStatus: () => void;
}

const Header: React.FC<HeaderProps> = (props) => {
    const {
        isSidebarExpanded, onToggleSidebar, onNewChat, onToggleTheme, onForkChat, isForking, onSummarizeChat, isSummarizing, onSaveChat, isSaving, settings, theme, activeChat,
        selectedHub, hubActions, user, notifications, onDisconnectWallet, onAvatarUpload, onOpenSettings, onStartVerification, onViewKey,
        isSystemStatusVisible, onToggleSystemStatus
    } = props;

    const [showNotifications, setShowNotifications] = useState(false);
    const notificationsRef = useRef<HTMLDivElement>(null);
    
    const unreadCount = notifications.filter(n => !n.read).length;

    const newChatShortcut = useMemo(() => {
        const isMac = navigator.platform.toUpperCase().indexOf('MAC') >= 0;
        return isMac ? '⌘ N' : 'Ctrl+N';
    }, []);

    useEffect(() => {
        const handleClickOutside = (event: MouseEvent) => {
            if (notificationsRef.current && !notificationsRef.current.contains(event.target as Node)) {
                setShowNotifications(false);
            }
        };
        document.addEventListener('mousedown', handleClickOutside);
        return () => document.removeEventListener('mousedown', handleClickOutside);
    }, []);

  const headerButtonClasses = theme === 'dark' 
    ? 'text-gray-400 hover:text-white hover:bg-gray-700/50' 
    : 'text-gray-500 hover:text-black hover:bg-gray-200/50';

  return (
    <header className={`flex items-center p-2 md:p-4 border-b ${theme === 'dark' ? 'border-blue-500/20' : 'border-gray-300'} flex-shrink-0 gap-2 md:gap-4`}>
      <div className="flex items-center space-x-2">
        <button onClick={onToggleSidebar} className={`p-2 ${headerButtonClasses} rounded-full transition-colors`} aria-label="Toggle menu">
            {isSidebarExpanded ? (
                <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-6 h-6"><path strokeLinecap="round" strokeLinejoin="round" d="M15.75 19.5L8.25 12l7.5-7.5" /></svg>
            ) : (
                <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-6 h-6"><path strokeLinecap="round" strokeLinejoin="round" d="M3.75 6.75h16.5M3.75 12h16.5m-16.5 5.25h16.5" /></svg>
            )}
        </button>
         {selectedHub ? (
            <div className="flex items-center gap-2">
                <button onClick={() => hubActions.goHome()} className={`p-2 ${headerButtonClasses} rounded-full transition-colors`} aria-label="Go back to chat">
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-6 h-6"><path strokeLinecap="round" strokeLinejoin="round" d="M10.5 19.5L3 12m0 0l7.5-7.5M3 12h18" /></svg>
                </button>
                <div className="hidden sm:flex items-center gap-2">
                    <span className="text-blue-400 w-6 h-6">{selectedHub.icon}</span>
                    <h1 className="text-lg font-bold text-blue-400 tracking-wider">{selectedHub.name}</h1>
                </div>
            </div>
        ) : (
            <div className={`hidden sm:flex items-center p-2 rounded-lg ${theme === 'dark' ? 'bg-gray-800/50' : 'bg-gray-200/50'}`}>
                <h1 className={`text-lg font-bold ${theme === 'dark' ? 'text-blue-400' : 'text-blue-600'} tracking-wider`}>
                    QUANTUM <span className={`font-light ${theme === 'dark' ? 'text-gray-400' : 'text-gray-600'}`}>OS</span>
                </h1>
            </div>
        )}
      </div>
      
      <button
        onClick={() => onNewChat()}
        className={`flex items-center shrink-0 space-x-2 text-sm p-2 sm:px-3 rounded-full transition-colors ${headerButtonClasses}`}
        data-tutorial-id="new-chat"
        title={`New Chat (${newChatShortcut})`}
      >
        <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-5 h-5"><path strokeLinecap="round" strokeLinejoin="round" d="M12 4.5v15m7.5-7.5h-15" /></svg>
        <span className="hidden lg:inline">New Chat</span>
      </button>

      <div className="flex-1 hidden md:block">
          {activeChat && !selectedHub && <Search theme={theme} />}
      </div>
      
      <div className="flex items-center space-x-2 ml-auto">
        <div className="relative" ref={notificationsRef}>
            <button onClick={() => setShowNotifications(s => !s)} className={`p-2 ${headerButtonClasses} rounded-full transition-colors relative`}>
                <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-6 h-6"><path strokeLinecap="round" strokeLinejoin="round" d="M14.857 17.082a23.848 23.848 0 005.454-1.31A8.967 8.967 0 0118 9.75v-.7V9A6 6 0 006 9v.75a8.967 8.967 0 01-2.312 6.022c1.733.64 3.56 1.085 5.455 1.31m5.714 0a24.255 24.255 0 01-5.714 0m5.714 0a3 3 0 11-5.714 0" /></svg>
                {unreadCount > 0 && <span className="absolute top-0 right-0 w-4 h-4 bg-red-500 text-white text-xs rounded-full flex items-center justify-center">{unreadCount}</span>}
            </button>
            {showNotifications && <NotificationsPanel notifications={notifications} onClose={() => setShowNotifications(false)} theme={theme} />}
        </div>
        <ProfileDropdown 
            user={user} 
            onDisconnectWallet={onDisconnectWallet} 
            onAvatarUpload={onAvatarUpload} 
            onOpenSettings={onOpenSettings} 
            onStartVerification={onStartVerification} 
            onViewKey={onViewKey} 
            theme={theme}
        />
         <button 
            onClick={onToggleSystemStatus} 
            className={`p-2 ${headerButtonClasses} rounded-full transition-colors hidden md:block`} 
            aria-label="Toggle system status panel"
            title="Toggle system status panel"
            data-tutorial-id="system-status-toggle"
        >
            {isSystemStatusVisible ? (
                 <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-6 h-6"><path strokeLinecap="round" strokeLinejoin="round" d="M11.25 4.5l7.5 7.5-7.5 7.5m-6-15l7.5 7.5-7.5 7.5" /></svg>
            ) : (
                <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-6 h-6"><path strokeLinecap="round" strokeLinejoin="round" d="M18.75 19.5l-7.5-7.5 7.5-7.5m-6 15L5.25 12l7.5-7.5" /></svg>
            )}
        </button>
      </div>
    </header>
  );
}

export default Header;
