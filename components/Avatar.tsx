import React, { useState, useEffect } from 'react';

type Status = 'loading' | 'loaded' | 'error';

interface AvatarProps {
  src?: string;
  alt?: string;
  className?: string;
  fallback: React.ReactNode;
}

const Avatar: React.FC<AvatarProps> = ({ src, alt = "Avatar", className, fallback }) => {
  const [status, setStatus] = useState<Status>(src ? 'loading' : 'error');

  useEffect(() => {
    if (!src) {
      setStatus('error');
      return;
    }

    setStatus('loading');
    const image = new Image();
    image.src = src;
    image.onload = () => setStatus('loaded');
    image.onerror = () => setStatus('error');
  }, [src]);

  if (status === 'loaded' && src) {
    return <img src={src} alt={alt} className={className} />;
  }

  if (status === 'loading') {
    return (
      <div className={`${className} bg-gray-700 animate-pulse`}></div>
    );
  }

  return <>{fallback}</>;
};

export default Avatar;
