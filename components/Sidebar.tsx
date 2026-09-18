

import React, { useState } from 'react';
import { ChatSession, ActionName, Hub } from '../types';
import AgentIcon from './AgentIcon';
import { hubs } from '../hubs.config';

interface SidebarProps {
  isExpanded: boolean;
  chats: ChatSession[];
  activeChatId: string | null;
  onNewChat: () => void;
  onSelectChat: (chatId: string) => void;
  hubActions: Record<ActionName, () => void>;
  theme: 'light' | 'dark';
  selectedHubId: string | null;
  setSelectedHubId: (id: string | null) => void;
}

const RecursiveMenuItem: React.FC<{ 
  item: Hub; 
  level: number; 
  isExpanded: boolean;
  openItems: string[];
  toggleItem: (id: string) => void;
  handleItemClick: (item: Hub) => void;
  theme: 'light' | 'dark';
  selectedHubId: string | null;
}> = ({ item, level, isExpanded, openItems, toggleItem, handleItemClick, theme, selectedHubId }) => {
  const isOpen = openItems.includes(item.id);
  const hasChildren = item.children && item.children.length > 0;
  const isSelected = selectedHubId === item.id;

  const buttonColor = theme === 'dark' ? 'text-gray-300 hover:bg-gray-700/50' : 'text-gray-700 hover:bg-gray-200/50';
  const selectedColor = theme === 'dark' ? 'bg-blue-500/20 text-blue-300' : 'bg-blue-100 text-blue-800';
  const iconColor = theme === 'dark' ? 'text-cyan-300' : 'text-cyan-600';

  const handleClick = () => {
    if (hasChildren) {
      toggleItem(item.id);
    }
    handleItemClick(item);
  };

  return (
    <div>
      <button
        onClick={handleClick}
        className={`w-full flex items-center justify-between p-3 rounded-lg text-left transition-colors duration-200 ${isSelected ? selectedColor : buttonColor} ${isExpanded ? '' : 'justify-center'}`}
        title={item.name}
        style={{ paddingLeft: isExpanded ? `${0.75 + level * 1.25}rem` : '0.75rem' }}
      >
        <div className="flex items-center space-x-3">
          <span className={`${iconColor} flex-shrink-0 w-6 h-6`}>{item.icon}</span>
          {isExpanded && <span className="font-medium text-sm truncate">{item.name}</span>}
        </div>
        {isExpanded && hasChildren && (
          <svg xmlns="http://www.w3.org/2000/svg" className={`h-4 w-4 transition-transform duration-300 ${isOpen ? 'rotate-90' : ''}`} fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5l7 7-7 7" /></svg>
        )}
      </button>
      {isExpanded && isOpen && hasChildren && (
        <div className="mt-1 space-y-1 animate-fade-in-fast">
          {item.children?.map(child => (
            <RecursiveMenuItem
              key={child.id}
              item={child}
              level={level + 1}
              isExpanded={isExpanded}
              openItems={openItems}
              toggleItem={toggleItem}
              handleItemClick={handleItemClick}
              theme={theme}
              selectedHubId={selectedHubId}
            />
          ))}
        </div>
      )}
    </div>
  );
};

const Sidebar: React.FC<SidebarProps> = ({ 
    isExpanded, 
    chats, 
    activeChatId, 
    onNewChat, 
    onSelectChat, 
    hubActions,
    theme,
    selectedHubId,
    setSelectedHubId
}) => {
  const [openItems, setOpenItems] = useState<string[]>([]);

  const toggleItem = (id: string) => {
    setOpenItems(prev => prev.includes(id) ? prev.filter(i => i !== id) : [...prev, id]);
  };

  const handleItemClick = (item: Hub) => {
    setSelectedHubId(item.id);
    if (item.action) {
      if (hubActions[item.action]) {
        hubActions[item.action]();
      }
    }
  };

  const sidebarBg = theme === 'dark' ? 'bg-gray-900/80 border-blue-500/20' : 'bg-gray-100/80 border-gray-300';
  const categoryHeaderColor = theme === 'dark' ? 'text-blue-300' : 'text-blue-800 font-semibold';
  const historyActiveClasses = theme === 'dark' ? 'bg-blue-500/20 text-blue-300' : 'bg-blue-100 text-blue-800';
  const historyInactiveClasses = theme === 'dark' ? 'text-gray-400 hover:bg-gray-700/50 hover:text-gray-200' : 'text-gray-500 hover:bg-gray-200 hover:text-gray-800';
  const footerButtonClasses = theme === 'dark' ? 'text-gray-400 hover:bg-gray-700/50 hover:text-gray-200' : 'text-gray-500 hover:bg-gray-200 hover:text-gray-800';
  const dividerColor = theme === 'dark' ? 'border-blue-500/20' : 'border-gray-300';
  
  const SettingsIcon = () => <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 flex-shrink-0" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826 3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z" /><path strokeLinecap="round" strokeLinejoin="round" d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" /></svg>;
  const HelpIcon = () => <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 flex-shrink-0" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M9.663 17h4.673M12 3v1m6.364 1.636l-.707.707M21 12h-1M4 12H3m3.343-5.657l-.707-.707m2.828 9.9a5 5 0 117.072 0l-.548.547A3.374 3.374 0 0014 18.469V19a2 2 0 11-4 0v-.531c0-.895-.356-1.754-.988-2.386l-.548-.547z" /></svg>;

  return (
    <aside className={`${sidebarBg} backdrop-blur-lg flex flex-col border-r transition-all duration-300 ${isExpanded ? 'w-80 p-4' : 'w-20 p-2'}`}>
      
      <div className="flex-1 min-h-0 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent pr-1 space-y-1 pt-4" data-tutorial-id="sidebar-menu">
          {hubs.map(categoryItem => (
              <RecursiveMenuItem
                key={categoryItem.id}
                item={categoryItem}
                level={0}
                isExpanded={isExpanded}
                openItems={openItems}
                toggleItem={toggleItem}
                handleItemClick={handleItemClick}
                theme={theme}
                selectedHubId={selectedHubId}
              />
          ))}

          <div className="mt-4 pt-4 border-t border-blue-500/10">
              {isExpanded && <h3 className={`text-sm font-semibold ${categoryHeaderColor} uppercase tracking-wider mb-2 px-3`}>History</h3>}
              <div className="space-y-2">
                 {chats.length > 0 ? chats.map((chat) => (
                    <button
                        key={chat.id}
                        onClick={() => onSelectChat(chat.id)}
                        className={`w-full flex items-center p-3 rounded-lg text-left transition-colors duration-200 ${isExpanded ? '' : 'justify-center'} ${
                        (selectedHubId === 'chat' && activeChatId === chat.id) ? historyActiveClasses : historyInactiveClasses
                        }`}
                        title={chat.title}
                    >
                        <div className="flex items-center space-x-3">
                            <AgentIcon agentId={chat.agentId} className="w-5 h-5 flex-shrink-0" />
                            {isExpanded && <p className="font-medium text-sm truncate">{chat.title}</p>}
                        </div>
                    </button>
                    )) : isExpanded && (
                    <p className="text-sm text-gray-500 italic px-2">No chat history.</p>
                    )}
              </div>
          </div>
      </div>


      <div className={`flex-shrink-0 mt-auto border-t ${dividerColor} pt-2 space-y-1`}>
           <button onClick={() => hubActions.openSettings()} className={`w-full flex items-center p-3 rounded-lg text-left transition-colors duration-200 ${footerButtonClasses} ${isExpanded ? '' : 'justify-center'}`} title="Settings">
              <div className="flex items-center space-x-3">
                <SettingsIcon />
                {isExpanded && <span className="font-medium text-sm">Settings</span>}
              </div>
            </button>
             <button onClick={() => hubActions.openMessages()} className={`w-full flex items-center p-3 rounded-lg text-left transition-colors duration-200 ${footerButtonClasses} ${isExpanded ? '' : 'justify-center'}`} title="Help & Q&A">
              <div className="flex items-center space-x-3">
                <HelpIcon />
                {isExpanded && <span className="font-medium text-sm">Help & Q&A</span>}
              </div>
            </button>
      </div>
      <style>{`
        .animate-fade-in-fast { animation: fade-in-fast 0.2s ease-out forwards; } 
        @keyframes fade-in-fast { from { opacity: 0; transform: translateY(-5px); } to { opacity: 1; transform: translateY(0); } }
      `}</style>
    </aside>
  );
};

export default Sidebar;
