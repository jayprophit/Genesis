import React, { useState, useRef, useEffect } from 'react';

interface SearchProps {
    theme: 'light' | 'dark';
}

const Search: React.FC<SearchProps> = ({ theme }) => {
    const [searchTerm, setSearchTerm] = useState('');
    const inputRef = useRef<HTMLInputElement>(null);

    const inputClasses = theme === 'dark'
        ? 'bg-gray-800 border-gray-700 text-gray-200 placeholder-gray-500 focus:ring-blue-500'
        : 'bg-white border-gray-300 text-gray-800 placeholder-gray-400 focus:ring-blue-500';

    useEffect(() => {
        const handleKeyDown = (event: KeyboardEvent) => {
            if ((event.metaKey || event.ctrlKey) && event.key === 'k') {
                event.preventDefault();
                inputRef.current?.focus();
            }
        };

        window.addEventListener('keydown', handleKeyDown);
        return () => window.removeEventListener('keydown', handleKeyDown);
    }, []);

    const handleSearch = (e: React.FormEvent) => {
        e.preventDefault();
        if (!searchTerm.trim()) return;
        console.log('Searching for:', searchTerm);
        setSearchTerm('');
        inputRef.current?.blur();
    };
    
    const isMac = typeof navigator !== 'undefined' && navigator.platform.toUpperCase().indexOf('MAC') >= 0;
    const shortcut = isMac ? '⌘K' : 'Ctrl+K';

    return (
        <div className="relative w-full max-w-xl mx-auto">
            <form onSubmit={handleSearch} className="w-full">
                <div className="absolute inset-y-0 left-0 flex items-center pl-3 pointer-events-none">
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-5 h-5 text-gray-500">
                        <path strokeLinecap="round" strokeLinejoin="round" d="M21 21l-5.197-5.197m0 0A7.5 7.5 0 105.196 5.196a7.5 7.5 0 0010.607 10.607z" />
                    </svg>
                </div>
                <input
                    ref={inputRef}
                    type="text"
                    value={searchTerm}
                    onChange={(e) => setSearchTerm(e.target.value)}
                    placeholder="Search..."
                    className={`w-full h-10 pl-10 pr-16 border rounded-full focus:outline-none focus:ring-2 ${inputClasses}`}
                />
                <div className="absolute inset-y-0 right-0 flex items-center pr-3 pointer-events-none">
                    <kbd className={`text-xs font-sans border rounded-md px-2 py-1 ${theme === 'dark' ? 'text-gray-400 border-gray-600' : 'text-gray-500 border-gray-300'}`}>
                        {shortcut}
                    </kbd>
                </div>
            </form>
        </div>
    );
};

export default Search;