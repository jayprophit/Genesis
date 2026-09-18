

import React, { useState } from 'react';
import { Message, Sender, Agent } from '../types';
import MiniAvatar from './MiniAvatar';
import Avatar from './Avatar';
import AudioPlayer from './AudioPlayer';

interface MessageBubbleProps {
  message: Message;
  agent?: Agent;
  theme: 'light' | 'dark';
  onAudioPlaybackChange: (isPlaying: boolean) => void;
}

// Simple markdown to HTML renderer
const renderText = (text: string, theme: 'light' | 'dark') => {
    const insightBg = theme === 'dark' ? 'bg-blue-900/40' : 'bg-blue-50';
    const insightBorder = theme === 'dark' ? 'border-blue-400' : 'border-blue-500';
    const insightHeader = theme === 'dark' ? 'text-blue-300' : 'text-blue-700';
    const insightText = theme === 'dark' ? 'text-blue-200/90' : 'text-blue-900';

    const preBg = theme === 'dark' ? 'bg-gray-900/70' : 'bg-gray-100';
    const codeBg = theme === 'dark' ? 'bg-gray-700' : 'bg-gray-200';

    const sourceBg = theme === 'dark' ? 'bg-gray-700/50' : 'bg-blue-50';
    const sourceBorder = theme === 'dark' ? 'border-blue-400' : 'border-blue-500';
    const sourceText = theme === 'dark' ? 'text-gray-300' : 'text-gray-600';
    const sourceStrong = theme === 'dark' ? 'text-gray-200' : 'text-gray-800';

    // Escape HTML to prevent XSS, but do it carefully
    let newText = text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');

    // Process markdown-like syntax
    newText = newText
      // Isolate and style Proactive Insights first
      .replace(/\*\*Proactive Insight:\*\*(.*)/g, (match, insight) => {
          return `<div class="my-3 p-3 ${insightBg} border-l-4 ${insightBorder} rounded-r-md">
                    <div class="flex items-center text-sm font-semibold ${insightHeader} mb-2">
                        <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5 mr-2" viewBox="0 0 20 20" fill="currentColor"><path d="M11 3a1 1 0 100 2h2.586l-6.293 6.293a1 1 0 101.414 1.414L15 6.414V9a1 1 0 102 0V4a1 1 0 00-1-1h-5z" /><path d="M5 5a2 2 0 00-2 2v8a2 2 0 002 2h8a2 2 0 002-2v-3a1 1 0 10-2 0v3H5V7h3a1 1 0 000-2H5z" /></svg>
                        Proactive Insight
                    </div>
                    <p class="${insightText}">${insight.trim()}</p>
                  </div>`;
      })
      .replace(/\*\*(.*?)\*\*/g, '<strong>$1</strong>') // Bold
      .replace(/\*(.*?)\*/g, '<em>$1</em>')       // Italic
      // Match code blocks, capturing language and content
      .replace(/```(?:[\w-]*\n)?([\s\S]+?)```/g, (match, code) => {
          // Re-escape the inner code content to avoid it being parsed as HTML
          const escapedCode = code.replace(/&lt;/g, '<').replace(/&gt;/g, '>').replace(/&amp;/g, '&');
          return `<pre class="${preBg} p-3 my-2 rounded-md text-sm font-mono whitespace-pre-wrap overflow-x-auto"><code>${escapedCode}</code></pre>`;
      })
      .replace(/`(.*?)`/g, `<code class="${codeBg} rounded px-1.5 py-1 text-sm font-mono">$1</code>`) // Inline code
      .replace(/\[Source: (.*?)\]/g, `<div class="my-2 p-2 ${sourceBg} border-l-4 ${sourceBorder} rounded-r-md text-xs ${sourceText} italic"><strong class="not-italic ${sourceStrong}">Source:</strong> $1</div>`)
      .replace(/\n/g, '<br />'); // Newlines

    // Correct <br> inside pre tags
    newText = newText.replace(/<pre(.*?)>([\s\S]*?)<\/pre>/g, (match, preAttributes, preContent) => {
        return `<pre${preAttributes}>${preContent.replace(/<br \/>/g, '\n')}</pre>`;
    });

    return { __html: newText };
};


const MessageBubble: React.FC<MessageBubbleProps> = ({ message, agent, theme, onAudioPlaybackChange }) => {
  const [isHovered, setIsHovered] = useState(false);
  const [showDriveOptions, setShowDriveOptions] = useState(false);
  const isAI = message.sender === Sender.AI;

  const bubbleClasses = isAI
    ? (theme === 'dark' ? 'bg-gray-800' : 'bg-gray-200 text-gray-800')
    : 'bg-blue-600 text-white';
  
  const wrapperClasses = isAI
    ? 'justify-start'
    : 'justify-end';

  const reasoningBorder = theme === 'dark' ? 'border-blue-500/20' : 'border-gray-300';
  const reasoningHeader = theme === 'dark' ? 'text-blue-300 hover:text-blue-200' : 'text-blue-700 hover:text-blue-900';
  const reasoningTextColor = theme === 'dark' ? 'text-gray-400' : 'text-gray-600';
  const proseClasses = theme === 'dark' ? 'prose prose-sm prose-invert' : 'prose prose-sm';
  
  const handleDownload = () => {
    const codeRegex = /```(?:[\w-]*\n)?([\s\S]+?)```/;
    const codeMatch = message.text.match(codeRegex);
    let content = message.text;
    let filename = `quantum-ai-response.txt`;
    let blobType = 'text/plain';

    if (message.generatedImageUrl) {
      const link = document.createElement('a');
      link.href = message.generatedImageUrl;
      link.download = `qai-generated-image.jpg`;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      return;
    } else if (codeMatch) {
      content = codeMatch[1];
      const langRegex = /```(\w+)/;
      const langMatch = message.text.match(langRegex);
      const extension = langMatch ? langMatch[1] : 'txt';
      filename = `code-snippet.${extension}`;
    }
    
    const blob = new Blob([content], { type: blobType });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = filename;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
  };

  const handleSaveToDrive = (drive: 'Google' | 'Microsoft') => {
    // Placeholder for actual drive integration
    console.log(`Saving to ${drive} Drive...`);
    setShowDriveOptions(false);
  };


  return (
    <div className={`flex items-start gap-3 ${wrapperClasses} group relative`} onMouseEnter={() => setIsHovered(true)} onMouseLeave={() => setIsHovered(false)}>
      {isAI && (
        <Avatar
            src={agent?.avatarUrl}
            alt="AI Avatar"
            className="w-8 h-8 md:w-10 md:h-10 rounded-full object-cover flex-shrink-0 shadow-lg"
            fallback={<MiniAvatar />}
        />
      )}
      <div
        className={`max-w-[80%] sm:max-w-xs md:max-w-md lg:max-w-2xl px-4 py-3 rounded-2xl shadow-md ${bubbleClasses} ${isAI ? 'rounded-bl-none' : 'rounded-br-none'}`}
      >
        {message.sender === Sender.User && message.imageUrl && (
            <div className="mb-2 -mx-1">
                <img src={message.imageUrl} alt="User upload" className="max-w-xs rounded-lg shadow-md" />
            </div>
        )}
        {message.generatedImageUrl && (
            <div className="mb-3 p-1 bg-gray-900/50 rounded-lg border border-blue-500/20">
                <img src={message.generatedImageUrl} alt="AI generated image" className="max-w-full rounded-md shadow-lg" />
            </div>
        )}
         {message.audioData && (
            <div className="mb-2">
                <AudioPlayer audioData={message.audioData} onPlaybackChange={onAudioPlaybackChange} />
            </div>
        )}
        {message.toolExecutionLog && (
            <div className="font-mono text-xs border border-green-500/20 bg-gray-900/50 rounded-md p-3">
                <div className="flex items-center text-green-400 mb-2">
                    <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4 mr-2" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826 3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z" /><path strokeLinecap="round" strokeLinejoin="round" d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" /></svg>
                    <span className="font-bold">ACTUATOR CORE: EXECUTING</span>
                </div>
                <div className="text-gray-400">
                    <span className="text-cyan-400">{message.toolExecutionLog.functionCall.name}</span>
                    <span>(</span>
                    <span className="text-purple-400">{JSON.stringify(message.toolExecutionLog.functionCall.args)}</span>
                    <span>)</span>
                </div>
                <div className="mt-2 pt-2 border-t border-green-500/10 text-gray-300">
                   <span className="text-gray-500 mr-1">&gt;</span> {message.toolExecutionLog.output}
                </div>
            </div>
        )}
        
        {message.reasoning && (
            <details className={`mb-3 border-b ${reasoningBorder} pb-3`}>
              <summary className={`cursor-pointer text-sm font-semibold ${reasoningHeader} flex items-center gap-2 list-none group`}>
                <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4 transition-transform group-open:rotate-90" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={2}>
                  <path strokeLinecap="round" strokeLinejoin="round" d="M9 5l7 7-7 7" />
                </svg>
                <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}>
                    <path strokeLinecap="round" strokeLinejoin="round" d="M21 15.25a2.25 2.25 0 01-2.25 2.25H5.25A2.25 2.25 0 013 15.25V8.75A2.25 2.25 0 015.25 6.5h13.5A2.25 2.25 0 0121 8.75v6.5z" />
                    <path strokeLinecap="round" strokeLinejoin="round" d="M12 1.5v5.25m0 10.5v5.25M4.5 12H9m6 0h4.5" />
                </svg>
                AI Reasoning
              </summary>
              <div 
                className={`${proseClasses} max-w-none break-words mt-2 pt-3 border-t ${reasoningBorder} ${reasoningTextColor}`}
                dangerouslySetInnerHTML={renderText(message.reasoning, theme)} 
              />
            </details>
        )}
        {message.text && <div className={`${proseClasses} max-w-none break-words`} dangerouslySetInnerHTML={renderText(message.text, theme)} />}

        {message.sources && message.sources.length > 0 && (
          <div className={`mt-4 pt-3 border-t ${reasoningBorder}`}>
            <h4 className="text-xs font-bold text-gray-400 mb-2 uppercase tracking-wider">Sources</h4>
            <ul className="space-y-1">
              {message.sources.map((source, index) => (
                <li key={index} className="flex items-center space-x-2">
                   <svg xmlns="http://www.w3.org/2000/svg" className="h-3 w-3 flex-shrink-0 text-blue-400" viewBox="0 0 20 20" fill="currentColor">
                     <path fillRule="evenodd" d="M12.586 4.586a2 2 0 112.828 2.828l-3 3a2 2 0 01-2.828 0 1 1 0 00-1.414 1.414 4 4 0 005.656 0l3-3a4 4 0 00-5.656-5.656l-1.5 1.5a1 1 0 101.414 1.414l1.5-1.5zm-5 5a2 2 0 012.828 0 1 1 0 101.414-1.414 4 4 0 00-5.656 0l-3 3a4 4 0 105.656 5.656l1.5-1.5a1 1 0 10-1.414-1.414l-1.5 1.5a2 2 0 11-2.828-2.828l3-3z" clipRule="evenodd" />
                   </svg>
                   <a
                    href={source.uri}
                    target="_blank"
                    rel="noopener noreferrer"
                    className="text-sm text-blue-400 hover:text-blue-300 truncate transition-colors"
                    title={source.title}
                  >
                    {source.title}
                  </a>
                </li>
              ))}
            </ul>
          </div>
        )}
      </div>
       {isAI && isHovered && (message.text || message.generatedImageUrl) && (
          <div className="absolute top-0 right-0 -translate-y-1/2 flex items-center bg-gray-900 border border-blue-500/20 rounded-full shadow-lg opacity-0 group-hover:opacity-100 transition-opacity duration-300">
                <button onClick={handleDownload} title="Download" className="p-2 text-gray-400 hover:text-white"><svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V3" /></svg></button>
                <div className="relative">
                    <button onClick={() => setShowDriveOptions(s => !s)} title="Save to cloud" className="p-2 text-gray-400 hover:text-white border-l border-blue-500/20"><svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 9l-7 7-7-7" /></svg></button>
                    {showDriveOptions && (
                        <div className="absolute bottom-full right-0 mb-2 w-48 bg-gray-800 border border-blue-500/30 rounded-md shadow-2xl py-1 z-10">
                            <button onClick={() => handleSaveToDrive('Google')} className="w-full text-left px-3 py-1.5 text-sm text-gray-300 hover:bg-gray-700/50 flex items-center">Save to Google Drive</button>
                            <button onClick={() => handleSaveToDrive('Microsoft')} className="w-full text-left px-3 py-1.5 text-sm text-gray-300 hover:bg-gray-700/50 flex items-center">Save to OneDrive</button>
                        </div>
                    )}
                </div>
          </div>
        )}
    </div>
  );
};

export default MessageBubble;
