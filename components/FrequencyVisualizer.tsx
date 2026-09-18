
import React, { useRef, useEffect } from 'react';

interface FrequencyVisualizerProps {
    isRunning: boolean;
    frequency: number; // e.g., in Hz
}

const FrequencyVisualizer: React.FC<FrequencyVisualizerProps> = ({ isRunning, frequency }) => {
    const canvasRef = useRef<HTMLCanvasElement>(null);

    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) return;
        const ctx = canvas.getContext('2d');
        if (!ctx) return;

        let animationFrameId: number;
        let time = 0;

        const render = () => {
            time += 0.02;
            const width = canvas.width;
            const height = canvas.height;
            
            ctx.clearRect(0, 0, width, height);
            
            if (isRunning) {
                ctx.beginPath();
                ctx.lineWidth = 2;
                ctx.strokeStyle = 'rgba(0, 255, 255, 0.7)';

                const centerY = height / 2;
                const amplitude = height / 4;
                const waveFrequency = frequency / 100; // Adjust for visual appeal

                for (let x = 0; x < width; x++) {
                    const y = centerY + amplitude * Math.sin(x * waveFrequency + time);
                    if (x === 0) {
                        ctx.moveTo(x, y);
                    } else {
                        ctx.lineTo(x, y);
                    }
                }
                ctx.stroke();

                // Add glowing effect
                ctx.shadowBlur = 10;
                ctx.shadowColor = 'rgba(0, 255, 255, 0.5)';
                ctx.stroke();
                ctx.shadowBlur = 0; // Reset shadow
            } else {
                // Draw flat line when idle
                ctx.beginPath();
                ctx.lineWidth = 2;
                ctx.strokeStyle = 'rgba(0, 255, 255, 0.2)';
                ctx.moveTo(0, height / 2);
                ctx.lineTo(width, height / 2);
                ctx.stroke();
            }

            animationFrameId = requestAnimationFrame(render);
        };

        render();

        return () => {
            cancelAnimationFrame(animationFrameId);
        };
    }, [isRunning, frequency]);

    return <canvas ref={canvasRef} width="300" height="100" className="w-full h-full" />;
};

export default FrequencyVisualizer;
