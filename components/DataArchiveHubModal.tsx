import React, { useState, useMemo } from 'react';
import { ArchivedFile } from '../types';

interface DataArchiveHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  files: ArchivedFile[];
}

const getFileIcon = (type: ArchivedFile['type']) => {
    switch (type) {
        case 'Image': return <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-purple-400" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M4 3a2 2 0 00-2 2v10a2 2 0 002 2h12a2 2 0 002-2V5a2 2 0 00-2-2H4zm12 12H4l4-8 3 6 2-4 3 6z" clipRule="evenodd" /></svg>;
        case 'Video': return <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-red-400" viewBox="0 0 20 20" fill="currentColor"><path d="M2 6a2 2 0 012-2h6a2 2 0 012 2v8a2 2 0 01-2 2H4a2 2 0 01-2-2V6zM14.553 7.106A1 1 0 0014 8v4a1 1 0 001.553.832l3-2a1 1 0 000-1.664l-3-2z" /></svg>;
        case 'Document': return <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-blue-400" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M4 4a2 2 0 012-2h4.586A2 2 0 0112 2.586L15.414 6A2 2 0 0116 7.414V16a2 2 0 01-2 2H6a2 2 0 01-2-2V4zm2 6a1 1 0 011-1h6a1 1 0 110 2H7a1 1 0 01-1-1zm1 3a1 1 0 100 2h6a1 1 0 100-2H7z" clipRule="evenodd" /></svg>;
        case 'CAD Model': return <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-yellow-400" viewBox="0 0 20 20" fill="currentColor"><path d="M10 2a1 1 0 00-1 1v1a1 1 0 002 0V3a1 1 0 00-1-1zM4 9a1 1 0 011-1h1a1 1 0 010 2H5a1 1 0 01-1-1zm12 0a1 1 0 011-1h1a1 1 0 110 2h-1a1 1 0 01-1-1zM9 4a1 1 0 01-1-1V2a1 1 0 012 0v1a1 1 0 01-1 1zm7 11a1 1 0 01-1-1v-1a1 1 0 112 0v1a1 1 0 01-1 1zM9 16a1 1 0 01-1-1v-1a1 1 0 112 0v1a1 1 0 01-1 1zM2 10a1 1 0 011 1v1a1 1 0 11-2 0v-1a1 1 0 011-1zm15 1a1 1 0 00-1-1h-1a1 1 0 100 2h1a1 1 0 001-1zM4 11a1 1 0 011-1h1a1 1 0 110 2H5a1 1 0 01-1-1z" /></svg>;
        case 'Simulation Report': return <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 text-cyan-400" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M6 2a2 2 0 00-2 2v12a2 2 0 002 2h8a2 2 0 002-2V7.414A2 2 0 0015.414 6L12 2.586A2 2 0 0010.586 2H6zm2 10a1 1 0 10-2 0v3a1 1 0 102 0v-3zm2-3a1 1 0 011 1v5a1 1 0 11-2 0v-5a1 1 0 011-1zm4-1a1 1 0 10-2 0v7a1 1 0 102 0V8z" clipRule="evenodd" /></svg>;
    }
}

const DataArchiveHubModal: React.FC<DataArchiveHubModalProps> = ({ isOpen, onClose, files }) => {
    const [currentPath, setCurrentPath] = useState('/');

    const folders = useMemo(() => {
        const folderSet = new Set(files.map(f => f.path).filter(p => p !== '/'));
        return ['/', ...Array.from(folderSet)];
    }, [files]);
    
    const filteredFiles = useMemo(() => files.filter(f => f.path === currentPath), [files, currentPath]);

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
            <div
                className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-5xl m-4 text-white relative animate-fade-in flex flex-col"
                style={{ height: 'clamp(500px, 90vh, 800px)' }}
                onClick={(e) => e.stopPropagation()}
            >
                <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
                    <div className="flex items-center space-x-3">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M8.25 7.5V6.108c0-1.135.845-2.098 1.976-2.192.373-.03.748-.057 1.123-.08M15.75 18H18a2.25 2.25 0 002.25-2.25V6.108c0-1.135-.845-2.098-1.976-2.192a48.424 48.424 0 00-1.123-.08M15.75 18.75v-1.875a3.375 3.375 0 00-3.375-3.375h-1.5a1.125 1.125 0 01-1.125-1.125v-1.5A3.375 3.375 0 006.375 7.5H5.25m11.9-3.664A2.251 2.251 0 0015 2.25h-1.5a2.251 2.251 0 00-2.15 1.586m5.8 0c.065.21.1.433.1.664v.75h-6v-.75c0-.231.035-.454.1-.664M6.75 7.5H4.875c-.621 0-1.125.504-1.125 1.125v11.25c0 .621.504 1.125 1.125 1.125h9.75c.621 0 1.125-.504 1.125-1.125V16.5" /></svg>
                        <h2 className="text-2xl font-bold text-blue-300">Data Archive</h2>
                    </div>
                    <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
                        <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
                    </button>
                </header>
                <main className="flex-1 flex overflow-hidden">
                    <aside className="w-64 p-4 border-r border-blue-500/20 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                        <h3 className="font-semibold text-gray-400 uppercase tracking-wider text-sm mb-3">Folders</h3>
                        <ul>
                            {folders.map(folder => (
                                <li key={folder}>
                                    <button onClick={() => setCurrentPath(folder)} className={`w-full flex items-center p-2 rounded-md text-left text-sm transition-colors ${currentPath === folder ? 'bg-blue-500/20 text-blue-300' : 'text-gray-300 hover:bg-gray-700/50'}`}>
                                        <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 mr-2" viewBox="0 0 20 20" fill="currentColor"><path d="M2 6a2 2 0 012-2h5l2 2h5a2 2 0 012 2v6a2 2 0 01-2 2H4a2 2 0 01-2-2V6z" /></svg>
                                        {folder === '/' ? 'Root' : folder.split('/').filter(Boolean).pop()}
                                    </button>
                                </li>
                            ))}
                        </ul>
                    </aside>
                    <div className="flex-1 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                        <table className="w-full text-sm text-left">
                            <thead className="text-xs text-gray-400 uppercase bg-gray-900/50">
                                <tr>
                                    <th scope="col" className="px-6 py-3">Name</th>
                                    <th scope="col" className="px-6 py-3">Type</th>
                                    <th scope="col" className="px-6 py-3">Size</th>
                                    <th scope="col" className="px-6 py-3">Created</th>
                                    <th scope="col" className="px-6 py-3"><span className="sr-only">Actions</span></th>
                                </tr>
                            </thead>
                            <tbody>
                                {filteredFiles.map(file => (
                                    <tr key={file.id} className="border-b border-gray-800 hover:bg-gray-800/50">
                                        <td className="px-6 py-4 font-medium text-white whitespace-nowrap flex items-center">{getFileIcon(file.type)}<span className="ml-2">{file.name}</span></td>
                                        <td className="px-6 py-4">{file.type}</td>
                                        <td className="px-6 py-4">{file.size}</td>
                                        <td className="px-6 py-4">{new Date(file.createdAt).toLocaleDateString()}</td>
                                        <td className="px-6 py-4 text-right">
                                            <button className="font-medium text-blue-400 hover:underline">Download</button>
                                        </td>
                                    </tr>
                                ))}
                            </tbody>
                        </table>
                        {filteredFiles.length === 0 && (
                            <div className="p-8 text-center text-gray-500">
                                <p>This folder is empty.</p>
                            </div>
                        )}
                    </div>
                </main>
                <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
            </div>
        </div>
    );
};

export default DataArchiveHubModal;
