
import React, { useState, useRef, useEffect, useCallback, useMemo } from 'react';
import { ChatOptions, AIStyle, Agent, ImagePayload, KnowledgeChunk } from '../types';

interface InteractionHubProps {
    onSendMessage: (message: string, options: ChatOptions, agentConfig: { systemInstruction: string, knowledgeBase: KnowledgeChunk[] }, image?: ImagePayload) => void;
    isLoading: boolean;
    agent: Agent;
    activeChatId: string;
    highlightedElementId?: string | null;
    theme: 'light' | 'dark';
    onListeningChange: (isListening: boolean) => void;
}

const InteractionHub: React.FC<InteractionHubProps> = ({ onSendMessage, isLoading, agent, activeChatId, highlightedElementId, theme, onListeningChange }) => {
  const [userInput, setUserInput] = useState('');
  const [imageToSend, setImageToSend] = useState<{data: string; name: string; mimeType: string} | null>(null);
  const [showOptions, setShowOptions] = useState(false);
  
  // Local state for agent config
  const [knowledgeBase, setKnowledgeBase] = useState<KnowledgeChunk[]>([]);
  const [systemInstruction, setSystemInstruction] = useState(agent.systemInstruction);

  const [options, setOptions] = useState<ChatOptions>({
      aiStyle: 'default',
      isReasoningEnabled: false,
      isDeepSearchEnabled: false,
      is4BitQuantizationEnabled: false,
      isTokenFreeSynthesisEnabled: false,
      isDeepWebEnabled: false,
  });
  
  const [isRecording, setIsRecording] = useState(false);
  const [isSpeechSupported, setIsSpeechSupported] = useState(false);
  
  const textareaRef = useRef<HTMLTextAreaElement>(null);
  const imageInputRef = useRef<HTMLInputElement>(null);
  const docFileInputRef = useRef<HTMLInputElement>(null);
  const recognitionRef = useRef<any>(null);
  const textBeforeRecording = useRef<string>('');

  useEffect(() => {
    setUserInput('');
    setImageToSend(null);
    if(textareaRef.current) textareaRef.current.focus();
    if (isRecording && recognitionRef.current) {
        recognitionRef.current.stop();
    }
    // Reset knowledge base for new chats
    setKnowledgeBase([]);
  }, [activeChatId, isRecording]);

  useEffect(() => {
    if (textareaRef.current) {
      textareaRef.current.style.height = 'auto';
      const scrollHeight = textareaRef.current.scrollHeight;
      textareaRef.current.style.height = `${scrollHeight}px`;
    }
  }, [userInput]);
  
  // Setup Speech Recognition
  useEffect(() => {
    const SpeechRecognitionAPI = window.SpeechRecognition || window.webkitSpeechRecognition;
    if (SpeechRecognitionAPI) {
        setIsSpeechSupported(true);
        const recognition = new SpeechRecognitionAPI();
        recognition.continuous = true;
        recognition.interimResults = true;
        recognition.lang = 'en-US';

        recognition.onresult = (event: any) => {
            const finalTranscriptParts: string[] = [];
            let interimTranscript = '';
            
            Array.from(event.results).forEach((result: any) => {
                if(result.isFinal) {
                    finalTranscriptParts.push(result[0].transcript);
                } else {
                    interimTranscript = result[0].transcript;
                }
            });

            const finalTranscript = finalTranscriptParts.join(' ');
            
            const parts = [];
            if(textBeforeRecording.current) parts.push(textBeforeRecording.current);
            if(finalTranscript) parts.push(finalTranscript);
            if(interimTranscript) parts.push(interimTranscript);

            setUserInput(parts.join(' '));
        };

        recognition.onerror = (event: any) => {
            console.error("Speech Recognition Error:", event.error);
            setIsRecording(false);
        };

        recognition.onend = () => {
            setIsRecording(false);
            onListeningChange(false);
        };
        recognitionRef.current = recognition;
    } else {
        setIsSpeechSupported(false);
    }
    return () => recognitionRef.current?.abort();
  }, [onListeningChange]);

  // Setup listener for global voice commands
  useEffect(() => {
    const handleCommand = (event: CustomEvent) => {
        if (event.detail === 'submit') {
            textareaRef.current?.form?.requestSubmit();
        }
    };

    document.addEventListener('quantum-command', handleCommand as EventListener);
    return () => {
        document.removeEventListener('quantum-command', handleCommand as EventListener);
    };
  }, []);

  const handleImageSelect = (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (file && file.type.startsWith('image/')) {
        const reader = new FileReader();
        reader.onloadend = () => {
            const base64String = (reader.result as string).split(',')[1];
            setImageToSend({ data: base64String, name: file.name, mimeType: file.type });
        };
        reader.readAsDataURL(file);
    }
    if (event.target) event.target.value = "";
  };

  const handleDocumentUpload = (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (file) {
        const reader = new FileReader();
        reader.onload = (e) => {
            const text = e.target?.result as string;
            if (!text) return;
            const chunks = text.split(/\n\s*\n/).filter(p => p.trim().length > 20);
            const newChunks: KnowledgeChunk[] = chunks.map((chunk, index) => ({
                id: `doc-${file.name}-${index}-${Date.now()}`,
                content: chunk.trim(),
                source: file.name,
            }));
            setKnowledgeBase(prev => {
                const otherChunks = prev.filter(c => c.source !== file.name);
                return [...otherChunks, ...newChunks];
            });
        };
        reader.readAsText(file);
    }
    if (event.target) event.target.value = "";
  };

  const uploadedSources = useMemo(() => [...new Set(knowledgeBase.map(chunk => chunk.source))], [knowledgeBase]);

  const handleSubmit = useCallback((e: React.FormEvent) => {
    e.preventDefault();
    if (userInput.trim() || imageToSend) {
        onSendMessage(
            userInput, 
            options, 
            { systemInstruction, knowledgeBase }, 
            imageToSend ? { data: imageToSend.data, mimeType: imageToSend.mimeType } : undefined
        );
        setUserInput('');
        setImageToSend(null);
    }
  }, [userInput, options, onSendMessage, imageToSend, systemInstruction, knowledgeBase]);

  const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSubmit(e as any);
    }
  };
  
  const handleOptionChange = (field: keyof ChatOptions, value: any) => setOptions(prev => ({ ...prev, [field]: value }));
  const handleToggleRecording = () => {
      if (!recognitionRef.current) return;
      if(isRecording) {
          recognitionRef.current.stop();
      } else {
          textBeforeRecording.current = userInput.trim();
          recognitionRef.current.start();
          setIsRecording(true);
          onListeningChange(true);
      }
  };

  const isSynergyActive = options.is4BitQuantizationEnabled && options.isTokenFreeSynthesisEnabled;
  const getHighlightClass = (id: string) => highlightedElementId === id ? 'relative z-[1001] bg-gray-900 rounded-lg' : '';

  const mainBg = theme === 'dark' ? 'bg-gray-900/30 border-blue-500/20' : 'bg-gray-100/80 border-gray-300';
  const buttonBg = theme === 'dark' ? 'bg-gray-700 hover:bg-gray-600 text-gray-400' : 'bg-gray-200 hover:bg-gray-300 text-gray-600';
  const focusRing = theme === 'dark' ? 'focus:ring-blue-500 focus:ring-offset-gray-900' : 'focus:ring-blue-500 focus:ring-offset-gray-100';
  const activeBtnBg = theme === 'dark' ? 'bg-blue-500/30 text-blue-300' : 'bg-blue-100 text-blue-600';
  const textareaClasses = theme === 'dark' 
    ? 'bg-gray-800 border-gray-700 text-gray-200 placeholder-gray-500 focus:ring-blue-500' 
    : 'bg-white border-gray-300 text-gray-800 placeholder-gray-500 focus:ring-blue-500';
  const optionsPanelBg = theme === 'dark' ? 'bg-gray-800/50' : 'bg-white/80 backdrop-blur-sm border border-gray-200';
  const optionTextColor = theme === 'dark' ? 'text-gray-300' : 'text-gray-700';
  const optionHeaderColor = theme === 'dark' ? 'text-gray-200' : 'text-gray-900';
  const selectClasses = theme === 'dark' 
    ? 'bg-gray-700 border-gray-600 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500' 
    : 'bg-gray-200 border-gray-300 rounded-md px-3 py-2 focus:ring-blue-500 focus:border-blue-500';
  const dividerColor = theme === 'dark' ? 'border-blue-500/10' : 'border-gray-200';

  return (
    <div className={`p-4 ${mainBg} border-t`}>
      <form onSubmit={handleSubmit}>
         {imageToSend && (
              <div className="relative mb-2 w-24 h-24 bg-gray-700 rounded-lg p-1">
                  <img src={`data:${imageToSend.mimeType};base64,${imageToSend.data}`} alt={imageToSend.name} className="w-full h-full object-cover rounded" />
                  <button onClick={() => setImageToSend(null)} className="absolute -top-2 -right-2 bg-red-600 rounded-full p-1 text-white hover:bg-red-500 transition-colors" aria-label="Remove image">
                      <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={2}><path strokeLinecap="round" strokeLinejoin="round" d="M6 18L18 6M6 6l12 12" /></svg>
                  </button>
              </div>
         )}
         <div className={`flex items-end space-x-1 sm:space-x-2 p-1 ${getHighlightClass('message-input')}`} data-tutorial-id="message-input">
          <input type="file" ref={imageInputRef} onChange={handleImageSelect} accept="image/jpeg,image/png,image/webp" className="hidden" />
          <input type="file" ref={docFileInputRef} onChange={handleDocumentUpload} accept=".txt" className="hidden" />
          <button type="button" onClick={() => setShowOptions(prev => !prev)} className={`p-2 sm:p-3 rounded-lg transition-colors duration-300 self-end focus:outline-none focus:ring-2 ${focusRing} ${showOptions ? activeBtnBg : buttonBg}`} aria-label="Toggle response options" title="Toggle response options">
              <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-5 h-5 sm:w-6 sm:h-6"><path strokeLinecap="round" strokeLinejoin="round" d="M10.5 6h9.75M10.5 6a1.5 1.5 0 11-3 0m3 0a1.5 1.5 0 10-3 0M3.75 6H7.5m3 12h9.75m-9.75 0a1.5 1.5 0 01-3 0m3 0a1.5 1.5 0 00-3 0m-3.75 0H7.5m9-6h3.75m-3.75 0a1.5 1.5 0 01-3 0m3 0a1.5 1.5 0 00-3 0m-9.75 0h9.75" /></svg>
          </button>
          <button type="button" onClick={() => imageInputRef.current?.click()} className={`p-2 sm:p-3 rounded-lg transition-colors duration-300 self-end ${buttonBg} focus:outline-none focus:ring-2 ${focusRing}`} aria-label="Attach image" title="Attach image">
            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-5 h-5 sm:w-6 sm:h-6">
                <path strokeLinecap="round" strokeLinejoin="round" d="M2.25 15.75l5.159-5.159a2.25 2.25 0 013.182 0l5.159 5.159m-1.5-1.5l1.409-1.409a2.25 2.25 0 013.182 0l2.909 2.909m-18 3.75h16.5a1.5 1.5 0 001.5-1.5V6a1.5 1.5 0 00-1.5-1.5H3.75A1.5 1.5 0 002.25 6v12a1.5 1.5 0 001.5 1.5zm10.5-11.25h.008v.008h-.008V8.25zm.375 0a.375.375 0 11-.75 0 .375.375 0 017.5 0z" />
            </svg>
          </button>
          <textarea ref={textareaRef} rows={1} value={userInput} onChange={(e) => setUserInput(e.target.value)} onKeyDown={handleKeyDown} placeholder={isRecording ? 'Listening...' : `Message Quantum AI...`} disabled={isLoading} className={`flex-1 rounded-lg px-4 py-3 focus:outline-none focus:ring-2 transition-shadow duration-300 disabled:opacity-50 resize-none max-h-48 ${textareaClasses}`} />
          {isSpeechSupported && (
               <button type="button" onClick={handleToggleRecording} className={`p-2 sm:p-3 rounded-lg transition-colors duration-300 self-end focus:outline-none focus:ring-2 ${focusRing} ${isRecording ? 'bg-red-600/80 text-white animate-pulse focus:ring-red-500' : `${buttonBg} focus:ring-blue-500`}`} aria-label={isRecording ? 'Stop recording' : 'Start recording'} title={isRecording ? 'Stop recording' : 'Start recording'}>
                  {isRecording ? <svg xmlns="http://www.w3.org/2000/svg" fill="currentColor" viewBox="0 0 16 16" className="w-5 h-5 sm:w-6 sm:h-6"><path d="M5 3.5h6A1.5 1.5 0 0 1 12.5 5v6a1.5 1.5 0 0 1-1.5 1.5H5A1.5 1.5 0 0 1 3.5 11V5A1.5 1.5 0 0 1 5 3.5z"/></svg> : <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-5 h-5 sm:w-6 sm:h-6"><path strokeLinecap="round" strokeLinejoin="round" d="M12 18.75a6 6 0 006-6v-1.5m-6 7.5a6 6 0 01-6-6v-1.5m6 7.5v3.75m-3.75 0h7.5M12 15.75a3 3 0 01-3-3V4.5a3 3 0 013-3a3 3 0 013 3v8.25a3 3 0 01-3 3z" /></svg>}
              </button>
          )}
          <button type="submit" disabled={isLoading || (!userInput.trim() && !imageToSend)} className={`bg-blue-600 hover:bg-blue-500 text-white font-bold p-2 sm:p-3 rounded-lg transition-all duration-300 disabled:opacity-50 disabled:cursor-not-allowed transform hover:scale-105 focus:outline-none focus:ring-2 ${focusRing} self-end`} aria-label="Send message">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 sm:h-6 sm:h-6" viewBox="0 0 24 24" fill="currentColor"><path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z" /></svg>
          </button>
         </div>
         <div className={`transition-all duration-500 ease-in-out overflow-hidden ${showOptions ? 'max-h-[500px] opacity-100 mt-4' : 'max-h-0 opacity-0'}`}>
              <div className={`p-4 ${optionsPanelBg} rounded-lg space-y-4 text-sm ${optionTextColor}`}>
                  <div>
                      <label htmlFor="aiStyle" className={`font-semibold mb-2 block ${optionHeaderColor}`}>AI Response Style</label>
                      <select id="aiStyle" value={options.aiStyle} onChange={(e) => handleOptionChange('aiStyle', e.target.value as AIStyle)} className={`w-full ${selectClasses}`}>
                          <option value="default">Default</option><option value="concise">Concise</option><option value="detailed">Detailed</option><option value="creative">Creative</option>
                      </select>
                  </div>
                   <div className={`pt-2 border-t ${dividerColor}`}>
                      <h3 className={`font-semibold mb-2 ${optionHeaderColor}`}>Knowledge Base</h3>
                      <div className={`grid grid-cols-1 gap-y-3 ${getHighlightClass('upload-doc')}`} data-tutorial-id="upload-doc">
                          <button type="button" onClick={() => docFileInputRef.current?.click()} className={`w-full flex items-center justify-center space-x-2 p-3 rounded-lg text-left transition-colors duration-200 ${buttonBg}`} title="Upload a .txt document"><svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-5 h-5"><path strokeLinecap="round" strokeLinejoin="round" d="M12 16.5V9.75m0 0l3 3m-3-3l-3 3M6.75 19.5a4.5 4.5 0 01-1.41-8.775 5.25 5.25 0 0110.233-2.33 3 3 0 013.758 3.848A3.752 3.752 0 0118 19.5H6.75z" /></svg><span className="text-sm font-medium">Upload Document</span></button>
                      </div>
                      {knowledgeBase.length > 0 && <div className={`text-xs p-2 mt-2 rounded-md ${theme === 'dark' ? 'text-gray-400 bg-gray-900/50' : 'text-gray-600 bg-gray-100'}`}><ul className="space-y-1">{uploadedSources.map(source => <li key={source} className="truncate flex items-center gap-2"><svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor" className="w-4 h-4 text-blue-400 flex-shrink-0"><path fillRule="evenodd" d="M4 2a2 2 0 00-2 2v12a2 2 0 002 2h12a2 2 0 002-2V6.414A2 2 0 0017.414 5L14 1.586A2 2 0 0012.586 1H4a2 2 0 00-2 2v1.5a.75.75 0 001.5 0V4a.5.5 0 01.5-.5h7.5a.5.5 0 01.5.5v2.5a.75.75 0 001.5 0V2a.5.5 0 01-.5-.5H4z" clipRule="evenodd" /></svg>{source}</li>)}</ul><button onClick={() => setKnowledgeBase([])} className="w-full text-center mt-2 text-red-400 hover:text-red-300 text-xs">Clear All</button></div>}
                  </div>
                   <div className={`pt-2 border-t ${dividerColor}`}>
                      <h3 className={`font-semibold mb-2 ${optionHeaderColor}`}>Processing Options</h3>
                      <div className="grid grid-cols-1 sm:grid-cols-2 gap-x-4 gap-y-3">
                          <label className="flex items-center cursor-pointer" title={options.isTokenFreeSynthesisEnabled ? "Combine with Token-Free Synthesis for hyper-efficient, holistic processing." : "Simulates a 4-bit quantized model for speed and efficiency, which may affect precision."}><input type="checkbox" checked={options.is4BitQuantizationEnabled} onChange={(e) => handleOptionChange('is4BitQuantizationEnabled', e.target.checked)} className="sr-only peer" /><div className={`relative w-11 h-6 rounded-full peer peer-focus:ring-4 peer-checked:after:translate-x-full after:content-[''] after:absolute after:top-0.5 after:start-[2px] after:border after:rounded-full after:h-5 after:w-5 after:transition-all ${theme === 'dark' ? 'bg-gray-600 peer-focus:ring-blue-800 peer-checked:after:border-white after:bg-white after:border-gray-300 peer-checked:bg-blue-600' : 'bg-gray-200 peer-focus:ring-blue-300 peer-checked:after:border-white after:bg-white after:border-gray-300 peer-checked:bg-blue-600'}`}></div><span className="ms-3 text-sm font-medium">4-Bit Quantization</span></label>
                          <label className="flex items-center cursor-pointer" title={options.is4BitQuantizationEnabled ? "Combine with 4-Bit Quantization for hyper-efficient, holistic processing." : "Engages conceptual token-free synthesis for more intuitive, holistic responses."}><input type="checkbox" checked={options.isTokenFreeSynthesisEnabled} onChange={(e) => handleOptionChange('isTokenFreeSynthesisEnabled', e.target.checked)} className="sr-only peer" /><div className={`relative w-11 h-6 rounded-full peer peer-focus:ring-4 peer-checked:after:translate-x-full after:content-[''] after:absolute after:top-0.5 after:start-[2px] after:border after:rounded-full after:h-5 after:w-5 after:transition-all ${theme === 'dark' ? 'bg-gray-600 peer-focus:ring-blue-800 peer-checked:after:border-white after:bg-white after:border-gray-300 peer-checked:bg-blue-600' : 'bg-gray-200 peer-focus:ring-blue-300 peer-checked:after:border-white after:bg-white after:border-gray-300 peer-checked:bg-blue-600'}`}></div><span className="ms-3 text-sm font-medium">Token-Free Synthesis</span></label>
                          <label className="flex items-center cursor-pointer" title="Enable to receive a step-by-step breakdown of the AI's thought process alongside its answer."><input type="checkbox" checked={options.isReasoningEnabled} onChange={(e) => handleOptionChange('isReasoningEnabled', e.target.checked)} className="sr-only peer" /><div className={`relative w-11 h-6 rounded-full peer peer-focus:ring-4 peer-checked:after:translate-x-full after:content-[''] after:absolute after:top-0.5 after:start-[2px] after:border after:rounded-full after:h-5 after:w-5 after:transition-all ${theme === 'dark' ? 'bg-gray-600 peer-focus:ring-blue-800 peer-checked:after:border-white after:bg-white after:border-gray-300 peer-checked:bg-blue-600' : 'bg-gray-200 peer-focus:ring-blue-300 peer-checked:after:border-white after:bg-white after:border-gray-300 peer-checked:bg-blue-600'}`}></div><span className="ms-3 text-sm font-medium">Show Reasoning</span></label>
                          <label className="flex items-center cursor-pointer" title="Enables the AI to search the web for up-to-date information and cite its sources in the response."><input type="checkbox" checked={options.isDeepSearchEnabled} onChange={(e) => handleOptionChange('isDeepSearchEnabled', e.target.checked)} className="sr-only peer" /><div className={`relative w-11 h-6 rounded-full peer peer-focus:ring-4 peer-checked:after:translate-x-full after:content-[''] after:absolute after:top-0.5 after:start-[2px] after:border after:rounded-full after:h-5 after:w-5 after:transition-all ${theme === 'dark' ? 'bg-gray-600 peer-focus:ring-blue-800 peer-checked:after:border-white after:bg-white after:border-gray-300 peer-checked:bg-blue-600' : 'bg-gray-200 peer-focus:ring-blue-300 peer-checked:after:border-white after:bg-white after:border-gray-300 peer-checked:bg-blue-600'}`}></div><span className="ms-3 text-sm font-medium">Search Web & Cite Sources</span></label>
                          <label className="flex items-center cursor-pointer" title="Engages Quantum Tunneling Protocol for unrestricted deep web analysis. Use with caution."><input type="checkbox" checked={options.isDeepWebEnabled} onChange={(e) => handleOptionChange('isDeepWebEnabled', e.target.checked)} className="sr-only peer" /><div className={`relative w-11 h-6 rounded-full peer peer-focus:ring-4 peer-checked:after:translate-x-full after:content-[''] after:absolute after:top-0.5 after:start-[2px] after:border after:rounded-full after:h-5 after:w-5 after:transition-all ${theme === 'dark' ? 'bg-gray-600 peer-focus:ring-yellow-800 peer-checked:after:border-white after:bg-white after:border-gray-300 peer-checked:bg-yellow-600' : 'bg-gray-200 peer-focus:ring-yellow-300 peer-checked:after:border-white after:bg-white after:border-gray-300 peer-checked:bg-yellow-600'}`}></div><span className="ms-3 text-sm font-medium">Deep Web Search</span></label>
                      </div>
                      {isSynergyActive && <div className="mt-3 p-2 bg-purple-600/20 border border-purple-500/50 rounded-md text-center text-xs text-purple-200"><strong>Synergy Active:</strong> Hyper-efficient cognitive core engaged.</div>}
                  </div>
              </div>
         </div>
      </form>
    </div>
  );
};

export default InteractionHub;
