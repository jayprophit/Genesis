
import React from 'react';

interface PlaceholderHubPageProps {
  title: string;
}

const PlaceholderHubPage: React.FC<PlaceholderHubPageProps> = ({ title }) => {
  return (
    <div className="w-full h-full p-6 flex flex-col items-center justify-center text-center">
        <svg xmlns="http://www.w3.org/2000/svg" className="h-16 w-16 text-gray-600 mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={1}>
            <path strokeLinecap="round" strokeLinejoin="round" d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4" />
        </svg>
        <h3 className="text-xl font-semibold text-gray-300">Feature Under Construction</h3>
        <p className="text-gray-500 mt-2">The '{title}' hub is currently in development. Please check back later.</p>
    </div>
  );
};

export default PlaceholderHubPage;
