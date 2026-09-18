import React from 'react';
import { Task } from '../types';

interface TaskHubModalProps {
  isOpen: boolean;
  onClose: () => void;
  tasks: Task[];
  onRefresh: () => void;
}

const getStatusBadge = (status: Task['status']) => {
    switch (status) {
        case 'Completed': return 'bg-green-500/20 text-green-300';
        case 'In Progress': return 'bg-blue-500/20 text-blue-300';
        case 'Pending': return 'bg-yellow-500/20 text-yellow-300';
        default: return 'bg-gray-500/20 text-gray-300';
    }
};

const TaskHubModal: React.FC<TaskHubModalProps> = ({ isOpen, onClose, tasks, onRefresh }) => {
    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center" onClick={onClose}>
            <div
                className="bg-gray-900/80 backdrop-blur-lg border border-blue-500/20 rounded-2xl shadow-2xl w-full max-w-2xl m-4 text-white relative animate-fade-in flex flex-col"
                style={{ height: 'clamp(400px, 80vh, 700px)' }}
                onClick={(e) => e.stopPropagation()}
            >
                <header className="p-6 border-b border-blue-500/20 flex items-center justify-between flex-shrink-0">
                    <div className="flex items-center space-x-3">
                         <svg xmlns="http://www.w3.org/2000/svg" className="h-7 w-7 text-blue-300" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5H7a2 2 0 00-2 2v12a2 2 0 002 2h10a2 2 0 002-2V7a2 2 0 00-2-2h-2M9 5a2 2 0 002 2h2a2 2 0 002-2M9 5a2 2 0 012-2h2a2 2 0 012 2m-6 9l2 2 4-4" /></svg>
                        <h2 className="text-2xl font-bold text-blue-300">Task Hub</h2>
                    </div>
                    <div className="flex items-center space-x-2">
                        <button onClick={onRefresh} className="text-gray-400 hover:text-white p-2 rounded-full hover:bg-gray-700/50 transition-colors" aria-label="Refresh Tasks" title="Refresh Tasks">
                            <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 4v5h5m16 0v5h-5m0-16h-5v5m-6 16h5v-5" transform="rotate(45 12 12)" /></svg>
                        </button>
                        <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors" aria-label="Close">
                            <svg xmlns="http://www.w3.org/2000/svg" className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
                        </button>
                    </div>
                </header>

                <main className="flex-1 p-6 pt-2 overflow-y-auto scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
                    {tasks.length > 0 ? (
                        <div className="space-y-3">
                            {tasks.map((task) => (
                                <div key={task.id} className="p-4 bg-gray-800/50 rounded-lg border border-gray-700">
                                    <div className="flex items-center justify-between mb-2">
                                        <p className="font-semibold text-gray-200">{task.id}: {task.description}</p>
                                        <span className={`px-2 py-1 text-xs font-bold rounded-full ${getStatusBadge(task.status)}`}>{task.status}</span>
                                    </div>
                                    {task.dependencies.length > 0 && (
                                        <div className="flex items-center space-x-2 mt-3 pt-3 border-t border-gray-700/50">
                                            <span className="text-xs text-gray-400">Depends on:</span>
                                            <div className="flex flex-wrap gap-1">
                                                {task.dependencies.map(depId => (
                                                    <span key={depId} className="px-2 py-0.5 text-xs bg-gray-700 text-gray-300 rounded-md font-mono">{depId}</span>
                                                ))}
                                            </div>
                                        </div>
                                    )}
                                </div>
                            ))}
                        </div>
                    ) : (
                        <div className="flex flex-col items-center justify-center h-full text-gray-500">
                             <svg xmlns="http://www.w3.org/2000/svg" className="w-16 h-16 mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1}><path strokeLinecap="round" strokeLinejoin="round" d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
                            <p className="text-lg">No tasks found.</p>
                            <p className="mt-1 text-sm">Create tasks by talking to the AI.</p>
                            <p className="mt-1 text-xs font-mono">e.g., "create a task to design the UI"</p>
                        </div>
                    )}
                </main>
                <style>{`.animate-fade-in { animation: fade-in 0.3s ease-out forwards; } @keyframes fade-in { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }`}</style>
            </div>
        </div>
    );
};

export default TaskHubModal;
