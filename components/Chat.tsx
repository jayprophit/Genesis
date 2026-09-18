
import React, { useRef, useEffect } from 'react';
import { Agent, ChatSession, ImagePayload, ChatOptions, KnowledgeChunk } from '../types';
import MessageBubble from './MessageBubble';
import WelcomeScreen from './WelcomeScreen';
import ThinkingProcess from './ThinkingProcess';
import InteractionHub from './InteractionHub';

interface ChatProps {
    activeChat?: ChatSession;
    activeAgent?: Agent;
    onSendMessage: (message: string, options: ChatOptions, agentConfig: { systemInstruction: string, knowledgeBase: KnowledgeChunk[] }, image?: ImagePayload) => void;
    isLoading: boolean;
    agent: Agent;
    highlightedElementId?: string | null;
    theme: 'light' | 'dark';
    onAudioPlaybackChange: (isPlaying: boolean) => void;
    onListeningChange: (isListening: boolean) => void;
}

const Chat: React.FC<ChatProps> = ({ activeChat, activeAgent, onSendMessage, isLoading, agent, highlightedElementId, theme, onAudioPlaybackChange, onListeningChange }) => {
  const chatContainerRef = useRef<HTMLDivElement>(null);
  
  useEffect(() => {
    if (chatContainerRef.current) {
      chatContainerRef.current.scrollTop = chatContainerRef.current.scrollHeight;
    }
  }, [activeChat?.messages, isLoading]);


  if (!activeChat || !activeAgent) {
    return (
        <div className={`h-full flex flex-col items-center justify-center text-center p-8 ${theme === 'dark' ? 'text-gray-500' : 'text-gray-400'}`}>
            <svg xmlns="http://www.w3.org/2000/svg" className="w-24 h-24 mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1}><path strokeLinecap="round" strokeLinejoin="round" d="M8 12h.01M12 12h.01M16 12h.01M21 12c0 4.418-4.03 8-9 8a9.863 9.863 0 01-4.255-.949L3 20l1.395-3.72C3.512 15.042 3 13.574 3 12c0-4.418 4.03-8 9-8s9 3.582 9 8z" /></svg>
            <h2 className={`text-2xl font-medium ${theme === 'dark' ? 'text-gray-300' : 'text-gray-700'}`}>Welcome to Quantum AI</h2>
            <p className="mt-2">Start a new chat to begin.</p>
        </div>
    );
  }

  const isNewChat = activeChat.messages.length === 1;

  return (
    <div className="h-full flex flex-col">
      <div
        ref={chatContainerRef}
        className="flex-1 p-4 md:p-6 space-y-6 overflow-y-auto scrollbar-thin scrollbar-thumb-blue-500/50 scrollbar-track-transparent"
      >
        {isNewChat ? <WelcomeScreen agent={activeAgent} theme={theme} /> : (
            <>
            {activeChat.messages.map((msg) => (
                <MessageBubble key={msg.id} message={msg} agent={activeAgent} theme={theme} onAudioPlaybackChange={onAudioPlaybackChange}/>
            ))}
            </>
        )}
        {isLoading && <ThinkingProcess agent={activeAgent} />}
      </div>
      <InteractionHub 
        onSendMessage={onSendMessage}
        isLoading={isLoading}
        agent={agent}
        activeChatId={activeChat.id}
        theme={theme}
        highlightedElementId={highlightedElementId}
        onListeningChange={onListeningChange}
      />
    </div>
  );
};

export default Chat;
