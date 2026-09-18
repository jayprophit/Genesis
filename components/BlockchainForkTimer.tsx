import React, { useState, useEffect } from 'react';

interface BlockchainForkTimerProps {
    targetDate: number;
}

const calculateTimeLeft = (targetDate: number) => {
    const difference = targetDate - Date.now();
    let timeLeft = {
        days: 0,
        hours: 0,
        minutes: 0,
        seconds: 0
    };

    if (difference > 0) {
        timeLeft = {
            days: Math.floor(difference / (1000 * 60 * 60 * 24)),
            hours: Math.floor((difference / (1000 * 60 * 60)) % 24),
            minutes: Math.floor((difference / 1000 / 60) % 60),
            seconds: Math.floor((difference / 1000) % 60)
        };
    }

    return timeLeft;
};

const TimerBox: React.FC<{ value: number, label: string }> = ({ value, label }) => (
    <div className="flex flex-col items-center justify-center bg-gray-900/50 p-3 rounded-lg w-20">
        <span className="text-2xl font-bold text-white">{value.toString().padStart(2, '0')}</span>
        <span className="text-xs text-gray-400 uppercase">{label}</span>
    </div>
);


const BlockchainForkTimer: React.FC<BlockchainForkTimerProps> = ({ targetDate }) => {
    const [timeLeft, setTimeLeft] = useState(calculateTimeLeft(targetDate));

    useEffect(() => {
        const timer = setTimeout(() => {
            setTimeLeft(calculateTimeLeft(targetDate));
        }, 1000);

        return () => clearTimeout(timer);
    });

    return (
        <div className="flex items-center justify-center space-x-2 md:space-x-4">
            <TimerBox value={timeLeft.days} label="Days" />
            <span className="text-2xl font-bold text-gray-500">:</span>
            <TimerBox value={timeLeft.hours} label="Hours" />
            <span className="text-2xl font-bold text-gray-500">:</span>
            <TimerBox value={timeLeft.minutes} label="Minutes" />
            <span className="text-2xl font-bold text-gray-500">:</span>
            <TimerBox value={timeLeft.seconds} label="Seconds" />
        </div>
    );
};

export default BlockchainForkTimer;
