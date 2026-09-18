
import React from 'react';

const GenesisForgePage: React.FC = () => {
  return (
    <div className="w-full h-full p-6 flex flex-col items-center justify-center text-center">
        <svg xmlns="http://www.w3.org/2000/svg" className="h-16 w-16 text-gray-600 mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1}>
            <path strokeLinecap="round" strokeLinejoin="round" d="M5 3v4M3 5h4M6 17v4m-2-2h4m5-16l2.286 6.857L21 12l-5.714 2.143L13 21l-2.286-6.857L5 12l5.714-2.143L13 3z" />
        </svg>
        <h3 className="text-xl font-semibold text-gray-300">Genesis Forge</h3>
        <p className="text-gray-500 mt-2">This hub is currently under construction. Please check back later.</p>
    </div>
  );
};

export default GenesisForgePage;
