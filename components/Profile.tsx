import React, { useState, useRef, useEffect } from 'react';
import { User } from '../types';
import Avatar from './Avatar';

interface ProfileProps {
    user: User | null;
    onDisconnectWallet: () => void;
    onAvatarUpload: (imageDataUrl: string) => void;
    onOpenSettings: () => void;
    onStartVerification: () => void;
    onViewKey: () => void;
}

const AuthMethodIcon: React.FC<{ method: User['loginMethod'] }> = ({ method }) => {
    switch (method) {
        case 'biometric':
            return <svg xmlns="http://www.w3.org/2000/svg" className="h-3 w-3 text-cyan-400" viewBox="0 0 20 20" fill="currentColor"><path d="M10 12a2 2 0 100-4 2 2 0 000 4z" /><path fillRule="evenodd" d="M.458 10C1.732 5.943 5.522 3 10 3s8.268 2.943 9.542 7c-1.274 4.057-5.022 7-9.542 7S1.732 14.057.458 10zM14 10a4 4 0 11-8 0 4 4 0 018 0z" clipRule="evenodd" /></svg>;
        case 'privateKey':
            return <svg xmlns="http://www.w3.org/2000/svg" className="h-3 w-3 text-yellow-400" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M18 8a6 6 0 01-7.743 5.743L4.257 19.743A1 1 0 112.84 18.33l10.9-10.9A6 6 0 0118 8zm-6-3a2 2 0 11-4 0 2 2 0 014 0z" clipRule="evenodd" /></svg>;
        case 'social':
             return <svg xmlns="http://www.w3.org/2000/svg" className="h-3 w-3 text-blue-400" viewBox="0 0 20 20" fill="currentColor"><path d="M10 2a4 4 0 00-4 4v1H5a1 1 0 00-.994.89l-1 9A1 1 0 004 18h12a1 1 0 00.994-1.11l-1-9A1 1 0 0015 7h-1V6a4 4 0 00-4-4zm2 5V6a2 2 0 10-4 0v1h4z" /></svg>;
        default:
            return null;
    }
};

const Profile: React.FC<ProfileProps> = ({ user, onDisconnectWallet, onAvatarUpload, onOpenSettings, onStartVerification, onViewKey }) => {
    const [isMenuOpen, setIsMenuOpen] = useState(false);
    const fileInputRef = useRef<HTMLInputElement>(null);
    const menuRef = useRef<HTMLDivElement>(null);

    const handleAvatarClick = () => {
        if (user) {
            fileInputRef.current?.click();
        }
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
            if (menuRef.current && !menuRef.current.contains(event.target as Node)) {
                setIsMenuOpen(false);
            }
        };
        document.addEventListener('mousedown', handleClickOutside);
        return () => document.removeEventListener('mousedown', handleClickOutside);
    }, []);

    if (!user) {
        return null;
    }

    const formattedAddress = `${user.walletAddress.slice(0, 6)}...${user.walletAddress.slice(-4)}`;

    return (
        <div className="flex-shrink-0 p-3 mb-2 bg-gray-800/50 border border-gray-700/50 rounded-lg flex items-center justify-between relative" ref={menuRef}>
            <div className="flex items-center space-x-3">
                 <button onClick={handleAvatarClick} className="group relative w-10 h-10" title="Change avatar">
                    <Avatar 
                        src={user.avatarUrl}
                        alt="User Avatar"
                        className="w-10 h-10 rounded-full object-cover border-2 border-blue-500/50"
                        fallback={
                            <div className="w-10 h-10 rounded-full bg-gray-700 flex items-center justify-center">
                               <span className="text-lg font-bold text-gray-400">{user.name.charAt(0).toUpperCase()}</span>
                            </div>
                        }
                    />
                    <div className="absolute inset-0 bg-black/60 rounded-full flex items-center justify-center opacity-0 group-hover:opacity-100 transition-opacity text-white">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={2}><path strokeLinecap="round" strokeLinejoin="round" d="M3 9a2 2 0 012-2h.93a2 2 0 001.664-.89l.812-1.22A2 2 0 0110.07 4h3.86a2 2 0 011.664.89l.812 1.22A2 2 0 0018.07 7H19a2 2 0 012 2v9a2 2 0 01-2 2H5a2 2 0 01-2-2V9z" /><path strokeLinecap="round" strokeLinejoin="round" d="M15 13a3 3 0 11-6 0 3 3 0 016 0z" /></svg>
                    </div>
                </button>
                <input type="file" ref={fileInputRef} onChange={handleFileChange} accept="image/*" className="hidden" />
                <div>
                    <p className="font-semibold text-white truncate w-32 flex items-center">{user.name} <span className="ml-1.5" title={`Authenticated via ${user.loginMethod}`}><AuthMethodIcon method={user.loginMethod} /></span></p>
                    <div className="flex items-center space-x-2">
                        <p className="text-xs text-gray-400 truncate w-20 font-mono" title={user.walletAddress}>{formattedAddress}</p>
                        {user.kycStatus === 'verified' && (
                            <span className="px-2 py-0.5 text-xs font-bold rounded-full bg-green-500/20 text-green-300 flex items-center">
                                <svg xmlns="http://www.w3.org/2000/svg" className="h-3 w-3 mr-1" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" /></svg>
                                Verified
                            </span>
                        )}
                    </div>
                </div>
            </div>
            <button onClick={() => setIsMenuOpen(prev => !prev)} className="p-2 text-gray-400 hover:text-white rounded-full hover:bg-gray-700/50 transition-colors">
                <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={2}>
                  <path strokeLinecap="round" strokeLinejoin="round" d="M12 5v.01M12 12v.01M12 19v.01M12 6a1 1 0 110-2 1 1 0 010 2zm0 7a1 1 0 110-2 1 1 0 010 2zm0 7a1 1 0 110-2 1 1 0 010 2z" />
                </svg>
            </button>
            {isMenuOpen && (
                 <div className="absolute right-0 bottom-full mb-2 w-48 bg-gray-800 border border-blue-500/20 rounded-lg shadow-2xl py-2 z-10">
                     <button onClick={() => { onOpenSettings(); setIsMenuOpen(false); }} className="w-full text-left px-4 py-2 text-sm text-gray-300 hover:bg-gray-700/50 hover:text-white transition-colors flex items-center space-x-3">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={2}><path strokeLinecap="round" strokeLinejoin="round" d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z" /><path strokeLinecap="round" strokeLinejoin="round" d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" /></svg>
                        <span>Settings</span>
                     </button>
                      {(user.loginMethod === 'social' || user.loginMethod === 'biometric') && (
                         <button onClick={() => { onViewKey(); setIsMenuOpen(false); }} className="w-full text-left px-4 py-2 text-sm text-gray-300 hover:bg-gray-700/50 hover:text-white transition-colors flex items-center space-x-3">
                            <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M18 8a6 6 0 01-7.743 5.743L4.257 19.743A1 1 0 112.84 18.33l10.9-10.9A6 6 0 0118 8zm-6-3a2 2 0 11-4 0 2 2 0 014 0z" clipRule="evenodd" /></svg>
                            <span>View Private Key</span>
                         </button>
                      )}
                      {user.kycStatus === 'none' && (
                         <button onClick={() => { onStartVerification(); setIsMenuOpen(false); }} className="w-full text-left px-4 py-2 text-sm text-cyan-300 hover:bg-gray-700/50 hover:text-cyan-200 transition-colors flex items-center space-x-3">
                            <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={2}><path strokeLinecap="round" strokeLinejoin="round" d="M9 12l2 2 4-4m5.618-4.016A11.955 11.955 0 0112 2.944a11.955 11.955 0 01-8.618 3.04A12.02 12.02 0 003 9c0 5.591 3.824 10.29 9 11.622 5.176-1.332 9-6.03 9-11.622 0-1.042-.133-2.052-.382-3.016z" /></svg>
                            <span>Verify Identity</span>
                         </button>
                      )}
                     <div className="my-1 h-px bg-blue-500/20"></div>
                     <button onClick={onDisconnectWallet} className="w-full text-left px-4 py-2 text-sm text-red-400 hover:bg-gray-700/50 hover:text-red-300 transition-colors flex items-center space-x-3">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={2}><path strokeLinecap="round" strokeLinejoin="round" d="M17 16l4-4m0 0l-4-4m4 4H7m6 4v1a3 3 0 01-3 3H6a3 3 0 01-3-3V7a3 3 0 013-3h4a3 3 0 013 3v1" /></svg>
                        <span>Disconnect</span>
                     </button>
                 </div>
            )}
        </div>
    );
};

export default Profile;