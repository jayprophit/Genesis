import React from 'react';
import { LoginReward } from '../types';

interface LoginRewardsHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  loginRewards: LoginReward[];
  userStreak: number;
}

const LoginRewardsHubModal: React.FC<LoginRewardsHubModalProps> = ({ isOpen, onClose, loginRewards, userStreak }) => {
  if (!isOpen) return null;

  const today = new Date();
  const isEaster = today.getMonth() === 2 && today.getDate() === 31;

  return (
    <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
      <div
        className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-3xl m-4 text-white relative animate-fade-in flex flex-col"
        style={{ height: 'clamp(500px, 90vh, 700px)' }}
        onClick={(e) => e.stopPropagation()}
      >
        <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
          <div className="flex items-center space-x-3">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1.5}><path strokeLinecap="round" strokeLinejoin="round" d="M8 7V3m8 4V3m-9 8h10M5 21h14a2 2 0 002-2V7a2 2 0 00-2-2H5a2 2 0 00-2 2v12a2 2 0 002 2z" /></svg>
            <h2 className="text-2xl font-bold text-blue-300">Login Rewards</h2>
          </div>
          <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </header>
        <main className="flex-1 p-6 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
          <div className="text-center mb-6 p-4 bg-gray-800/50 rounded-lg">
            <p className="text-gray-400">Current Streak</p>
            <p className="text-4xl font-bold text-blue-300">{userStreak} Day{userStreak !== 1 ? 's' : ''}</p>
            <p className="text-sm text-gray-500 mt-1">Log in tomorrow to continue your streak!</p>
          </div>
          
          {isEaster && (
            <div className="mb-6 p-4 bg-purple-600/20 border border-purple-500/50 rounded-lg text-center">
              <p className="font-bold text-lg text-purple-200">Happy Easter!</p>
              <p className="text-purple-300">A special bonus of 100 QAI has been added to your login reward today!</p>
            </div>
          )}

          <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-5 gap-4">
            {loginRewards.map((reward) => {
              const isClaimed = userStreak >= reward.day;
              const isToday = userStreak === reward.day -1; // Streak is 1 on day 1, so today is streak + 1
              
              let cardClass = 'bg-gray-800/50 border-gray-700';
              if (isClaimed) cardClass = 'bg-green-500/10 border-green-500/30';
              if (isToday) cardClass = 'border-blue-500 ring-2 ring-blue-500 bg-gray-800/80 animate-pulse';
              if (reward.isSpecial) cardClass += ' border-yellow-500/50';

              return (
                <div key={reward.day} className={`p-4 rounded-lg border text-center transition-all ${cardClass}`}>
                  <p className={`font-bold text-lg ${reward.isSpecial ? 'text-yellow-300' : 'text-gray-300'}`}>Day {reward.day}</p>
                  <div className="my-2">
                    <p className="text-sm text-blue-300 font-semibold">{reward.points} Points</p>
                    {reward.assets && <p className="text-sm text-cyan-300 font-semibold">{reward.assets.amount} {reward.assets.symbol}</p>}
                  </div>
                  {isClaimed ? (
                    <div className="text-xs font-bold text-green-400 flex items-center justify-center">
                      <svg xmlns="http://www.w3.org/2000/svg" className="h-4 w-4 mr-1" viewBox="0 0 20 20" fill="currentColor"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" /></svg>
                      Claimed
                    </div>
                  ) : isToday ? (
                     <div className="text-xs font-bold text-blue-300">Today</div>
                  ) : (
                    <div className="text-xs text-gray-500">Upcoming</div>
                  )}
                </div>
              );
            })}
          </div>
        </main>
        <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
      </div>
    </div>
  );
};

export default LoginRewardsHubModal;