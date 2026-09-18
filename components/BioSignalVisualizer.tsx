
import React, { useRef, useEffect } from 'react';

type LifeformType = 'mammal' | 'avian' | 'marine' | 'flora' | 'insect' | 'reptile';

interface BioSignalVisualizerProps {
    isActive: boolean;
    lifeformType: LifeformType;
}

const BioSignalVisualizer: React.FC<BioSignalVisualizerProps> = ({ isActive, lifeformType }) => {
    const canvasRef = useRef<HTMLCanvasElement>(null);

    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) return;
        const ctx = canvas.getContext('2d');
        if (!ctx) return;

        let animationFrameId: number;
        let time = 0;

        const resizeCanvas = () => {
            canvas.width = canvas.offsetWidth;
            canvas.height = canvas.offsetHeight;
        };
        resizeCanvas();
        window.addEventListener('resize', resizeCanvas);

        const render = () => {
            time += 0.02;
            const width = canvas.width;
            const height = canvas.height;
            
            ctx.clearRect(0, 0, width, height);
            
            if (isActive) {
                // Common background particles
                for (let i = 0; i < 20; i++) {
                    ctx.fillStyle = `rgba(0, 255, 255, ${Math.random() * 0.2})`;
                    ctx.beginPath();
                    ctx.arc(Math.random() * width, Math.random() * height, Math.random() * 2, 0, 2 * Math.PI);
                    ctx.fill();
                }

                ctx.lineWidth = 1.5;
                const centerY = height / 2;
                
                // Draw different signals based on type
                switch (lifeformType) {
                    case 'flora': // subtle pulses
                        ctx.strokeStyle = 'rgba(74, 222, 128, 0.7)'; // green
                        const pulse = (Math.sin(time * 2) + 1) / 2;
                        ctx.beginPath();
                        ctx.arc(width / 2, height / 2, (width/4) * pulse, 0, 2*Math.PI);
                        ctx.stroke();
                        break;
                    case 'marine': // low frequency sonar
                        ctx.strokeStyle = 'rgba(59, 130, 246, 0.7)'; // blue
                        for (let i = 0; i < 3; i++) {
                           ctx.beginPath();
                           const amplitude = height / 4;
                           for (let x = 0; x < width; x++) {
                                const y = centerY + amplitude * Math.sin(x * 0.01 + time + i * 2) * Math.cos(time + i);
                                x === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
                            }
                            ctx.stroke();
                        }
                        break;
                    default: // standard bio-signal (mammal, avian etc)
                         ctx.strokeStyle = 'rgba(0, 255, 255, 0.7)'; // cyan
                         ctx.beginPath();
                         for (let x = 0; x < width; x++) {
                            const y = centerY + (height/3) * Math.sin(x * 0.05 + time) * Math.sin(time);
                            x === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
                         }
                         ctx.stroke();
                         break;
                }
            } else {
                ctx.fillStyle = 'rgba(0, 255, 255, 0.5)';
                ctx.font = '12px "Courier New", monospace';
                ctx.textAlign = 'center';
                ctx.fillText('NO SIGNAL', width / 2, height / 2);
            }

            animationFrameId = requestAnimationFrame(render);
        };

        render();

        return () => {
            cancelAnimationFrame(animationFrameId);
            window.removeEventListener('resize', resizeCanvas);
        };
    }, [isActive, lifeformType]);

    return <canvas ref={canvasRef} className="w-full h-full" />;
};

export default BioSignalVisualizer;
