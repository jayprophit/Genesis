
import React, { useState, useEffect, useRef } from 'react';

interface AudioPlayerProps {
    audioData: string; // base64 encoded string
    onPlaybackChange: (isPlaying: boolean) => void;
}

const decode = (base64: string) => {
    const binaryString = atob(base64);
    const len = binaryString.length;
    const bytes = new Uint8Array(len);
    for (let i = 0; i < len; i++) {
        bytes[i] = binaryString.charCodeAt(i);
    }
    return bytes;
};

async function decodeAudioData(data: Uint8Array, ctx: AudioContext, sampleRate: number, numChannels: number): Promise<AudioBuffer> {
    const dataInt16 = new Int16Array(data.buffer);
    const frameCount = dataInt16.length / numChannels;
    const buffer = ctx.createBuffer(numChannels, frameCount, sampleRate);

    for (let channel = 0; channel < numChannels; channel++) {
        const channelData = buffer.getChannelData(channel);
        for (let i = 0; i < frameCount; i++) {
            channelData[i] = dataInt16[i * numChannels + channel] / 32768.0;
        }
    }
    return buffer;
}

const AudioPlayer: React.FC<AudioPlayerProps> = ({ audioData, onPlaybackChange }) => {
    const [isPlaying, setIsPlaying] = useState(false);
    const [progress, setProgress] = useState(0);
    const [duration, setDuration] = useState(0);
    
    const audioContextRef = useRef<AudioContext | null>(null);
    const audioBufferRef = useRef<AudioBuffer | null>(null);
    const sourceRef = useRef<AudioBufferSourceNode | null>(null);
    const startTimeRef = useRef(0);
    const animationFrameRef = useRef(0);

    useEffect(() => {
        onPlaybackChange(isPlaying);
    }, [isPlaying, onPlaybackChange]);

    useEffect(() => {
        // @ts-ignore
        const context = new (window.AudioContext || window.webkitAudioContext)({ sampleRate: 24000 });
        audioContextRef.current = context;

        const setupAudio = async () => {
            try {
                const decodedBytes = decode(audioData);
                const buffer = await decodeAudioData(decodedBytes, context, 24000, 1);
                audioBufferRef.current = buffer;
                setDuration(buffer.duration);
            } catch (error) {
                console.error("Failed to decode audio data:", error);
            }
        };
        setupAudio();

        return () => {
            sourceRef.current?.stop();
            context.close();
            cancelAnimationFrame(animationFrameRef.current);
        };
    }, [audioData]);

    const updateProgress = () => {
        if (isPlaying && audioContextRef.current) {
            const elapsedTime = audioContextRef.current.currentTime - startTimeRef.current;
            setProgress(elapsedTime);
            animationFrameRef.current = requestAnimationFrame(updateProgress);
        }
    };
    
    const play = () => {
        if (!audioContextRef.current || !audioBufferRef.current) return;
        
        if (sourceRef.current) {
            sourceRef.current.stop();
        }

        const source = audioContextRef.current.createBufferSource();
        source.buffer = audioBufferRef.current;
        source.connect(audioContextRef.current.destination);
        source.start(0, progress); // Start from current progress
        
        source.onended = () => {
            const endedNaturally = progress >= duration - 0.1;
            setIsPlaying(false);
            if (endedNaturally) {
                setProgress(0);
            }
             cancelAnimationFrame(animationFrameRef.current);
        };

        sourceRef.current = source;
        startTimeRef.current = audioContextRef.current.currentTime - progress;
        setIsPlaying(true);
        animationFrameRef.current = requestAnimationFrame(updateProgress);
    };

    const pause = () => {
        if (sourceRef.current) {
            sourceRef.current.stop();
            sourceRef.current = null;
        }
        setIsPlaying(false);
        cancelAnimationFrame(animationFrameRef.current);
    };


    const handleTogglePlay = () => {
        if (isPlaying) {
            pause();
        } else {
            play();
        }
    };

    return (
        <div className="flex items-center space-x-3 p-2 bg-gray-900/50 rounded-lg">
            <button onClick={handleTogglePlay} className="p-2 bg-blue-600 hover:bg-blue-500 rounded-full text-white transition-colors">
                {isPlaying ? (
                    <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zM7 8a1 1 0 012 0v4a1 1 0 11-2 0V8zm5-1a1 1 0 00-1 1v4a1 1 0 102 0V8a1 1 0 00-1-1z" clipRule="evenodd" /></svg>
                ) : (
                    <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM9.555 7.168A1 1 0 008 8v4a1 1 0 001.555.832l3-2a1 1 0 000-1.664l-3-2z" clipRule="evenodd" /></svg>
                )}
            </button>
            <div className="flex-1 h-1.5 bg-gray-700 rounded-full">
                <div 
                    className="h-1.5 bg-blue-500 rounded-full"
                    style={{ width: `${(progress / duration) * 100}%` }}
                ></div>
            </div>
             <span className="text-xs text-gray-400 font-mono">{progress.toFixed(1)}s / {duration.toFixed(1)}s</span>
        </div>
    );
};

export default AudioPlayer;
