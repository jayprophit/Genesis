import React, { useState, useRef, useEffect } from 'react';
import { User } from '../types';
import Avatar from './Avatar';

interface ProfileDropdownProps {
    user: User;
    onDisconnectWallet: () => void;
    onAvatarUpload: (imageDataUrl: string) => void;
    onOpenSettings: () => void;
    onStartVerification: () => void;
    onViewKey: () => void;
    theme: 'light' | 'dark';
}

const AuthMethodIcon: React.FC<{ method: User['loginMethod'] }> = ({ method }) => {
    // ... (same as in old Profile.tsx)
    return null;
};

const ProfileDropdown: React.FC<ProfileDropdownProps> = ({ user, onDisconnectWallet, onAvatarUpload, onOpenSettings, onStartVerification, onViewKey, theme }) => {
    const [isOpen, setIsOpen] = useState(false);
    const fileInputRef = useRef<HTMLInputElement>(null);
    const dropdownRef = useRef<HTMLDivElement>(null);

    const handleAvatarClick = () => {
        setIsOpen(prev => !prev);
    };

    const handleFileChange = (event: React.ChangeEvent<HTMLInputElement>) => {
        const file = event.target.files?.[0];
        if (file && file.type.startsWith('image/')) {
            const reader = new FileReader();
            reader.onloadend = () => {
                onAvatarUpload(reader.result as string);
            };
            reader.readAsDataURL(file);
        }
    };

    useEffect(() => {
        const handleClickOutside = (event: MouseEvent) => {
            if (dropdownRef.current && !dropdownRef.current.contains(event.target as Node)) {
                setIsOpen(false);
            }
        };
        document.addEventListener('mousedown', handleClickOutside);
        return () => document.removeEventListener('mousedown', handleClickOutside);
    }, []);

    const formattedAddress = `${user.walletAddress.slice(0, 6)}...${user.walletAddress.slice(-4)}`;
    
    const dropdownBg = theme === 'dark' ? 'bg-gray-800 border-blue-500/20' : 'bg-white border-gray-300';
    const itemHoverBg = theme === 'dark' ? 'hover:bg-gray-700/50' : 'hover:bg-gray-100';
    const itemTextColor = theme === 'dark' ? 'text-gray-300 hover:text-white' : 'text-gray-700 hover:text-black';


    return (
        <div className="relative" ref={dropdownRef}>
            <input type="file" ref={fileInputRef} onChange={handleFileChange} accept="image/*" className="hidden" />
            <button onClick={handleAvatarClick} className="w-10 h-10 rounded-full focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 focus:ring-offset-gray-900">
                <Avatar
                    src={user.avatarUrl}
                    alt="User Avatar"
                    className="w-10 h-10 rounded-full object-cover"
                    fallback={
                        <div className={`w-10 h-10 rounded-full flex items-center justify-center ${theme === 'dark' ? 'bg-gray-700' : 'bg-gray-200'}`}>
                           <span className={`text-lg font-bold ${theme === 'dark' ? 'text-gray-400' : 'text-gray-500'}`}>{user.name.charAt(0).toUpperCase()}</span>
                        </div>
                    }
                />
            </button>

            {isOpen && (
                <div className={`absolute right-0 mt-2 w-72 ${dropdownBg} rounded-lg shadow-2xl py-2 z-20 animate-fade-in-fast`}>
                    <div className="px-4 py-3 border-b border-gray-700">
                        <div className="flex items-center space-x-3">
                            <button onClick={() => fileInputRef.current?.click()} className="group relative w-12 h-12 flex-shrink-0">
                                <Avatar
                                    src={user.avatarUrl}
                                    alt="User Avatar"
                                    className="w-12 h-12 rounded-full object-cover"
                                    fallback={<div className={`w-12 h-12 rounded-full flex items-center justify-center ${theme === 'dark' ? 'bg-gray-700' : 'bg-gray-200'}`}><span className={`text-xl font-bold ${theme === 'dark' ? 'text-gray-400' : 'text-gray-500'}`}>{user.name.charAt(0).toUpperCase()}</span></div>}
                                />
                                <div className="absolute inset-0 bg-black/60 rounded-full flex items-center justify-center opacity-0 group-hover:opacity-100 transition-opacity text-white cursor-pointer"><svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M3 9a2 2 0 012-2h.93a2 2 0 001.664-.89l.812-1.22A2 2 0 0110.07 4h3.86a2 2 0 011.664.89l.812 1.22A2 2 0 0018.07 7H19a2 2 0 012 2v9a2 2 0 01-2 2H5a2 2 0 01-2-2V9z" /><path strokeLinecap="round" strokeLinejoin="round" d="M15 13a3 3 0 11-6 0 3 3 0 016 0z" /></svg></div>
                            </button>
                            <div>
                                <p className={`font-semibold truncate ${theme === 'dark' ? 'text-white' : 'text-black'}`}>{user.name}</p>
                                <p className="text-xs text-gray-400 font-mono" title={user.walletAddress}>{formattedAddress}</p>
                            </div>
                        </div>
                         {user.kycStatus === 'verified' && (
                            <div className="mt-2 text-xs font-bold rounded-full bg-green-500/20 text-green-300 flex items-center justify-center py-1">
                                <svg xmlns="http://www.w3.org/2000/svg" className="h-3 w-3 mr-1" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" /></svg>
                                Verified
                            </div>
                        )}
                    </div>
                    <div className="py-2">
                        {user.kycStatus === 'none' && (
                            <button onClick={() => { onStartVerification(); setIsOpen(false); }} className={`w-full text-left px-4 py-2 text-sm text-cyan-300 ${itemHoverBg} hover:text-cyan-200 transition-colors flex items-center space-x-3`}>
                                <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={2}><path strokeLinecap="round" strokeLinejoin="round" d="M9 12l2 2 4-4m5.618-4.016A11.955 11.955 0 0112 2.944a11.955 11.955 0 01-8.618 3.04A12.02 12.02 0 003 9c0 5.591 3.824 10.29 9 11.622 5.176-1.332 9-6.03 9-11.622 0-1.042-.133-2.052-.382-3.016z" /></svg>
                                <span>Verify Identity</span>
                            </button>
                        )}
                        {(user.loginMethod === 'social' || user.loginMethod === 'biometric') && (
                            <button onClick={() => { onViewKey(); setIsOpen(false); }} className={`w-full text-left px-4 py-2 text-sm ${itemTextColor} ${itemHoverBg} transition-colors flex items-center space-x-3`}>
                                <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M18 8a6 6 0 01-7.743 5.743L4.257 19.743A1 1 0 112.84 18.33l10.9-10.9A6 6 0 0118 8zm-6-3a2 2 0 11-4 0 2 2 0 014 0z" clipRule="evenodd" /></svg>
                                <span>View Private Key</span>
                            </button>
                        )}
                        <button onClick={() => { onOpenSettings(); setIsOpen(false); }} className={`w-full text-left px-4 py-2 text-sm ${itemTextColor} ${itemHoverBg} transition-colors flex items-center space-x-3`}>
                            <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={2}><path strokeLinecap="round" strokeLinejoin="round" d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z" /><path strokeLinecap="round" strokeLinejoin="round" d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" /></svg>
                            <span>Settings</span>
                        </button>
                    </div>
                    <div className="my-1 h-px bg-gray-700"></div>
                    <button onClick={onDisconnectWallet} className={`w-full text-left px-4 py-2 text-sm text-red-400 ${itemHoverBg} hover:text-red-300 transition-colors flex items-center space-x-3`}>
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={2}><path strokeLinecap="round" strokeLinejoin="round" d="M17 16l4-4m0 0l-4-4m4 4H7m6 4v1a3 3 0 01-3 3H6a3 3 0 01-3-3V7a3 3 0 013-3h4a3 3 0 013 3v1" /></svg>
                        <span>Disconnect</span>
                    </button>
                </div>
            )}
             <style>{`.animate-fade-in-fast { animation: fade-in-fast 0.15s ease-out forwards; } @keyframes fade-in-fast { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
        </div>
    );
};

export default ProfileDropdown;
