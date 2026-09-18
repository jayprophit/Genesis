

import React, { useState, useLayoutEffect, useRef } from 'react';

// Define the structure for each tutorial step
interface TutorialStep {
  targetId: string;
  title: string;
  content: string;
  placement?: 'top' | 'bottom' | 'left' | 'right';
  requiresSidebar?: boolean;
}

const steps: TutorialStep[] = [
  // Step 0: Welcome (no target)
  {
    targetId: 'welcome',
    title: 'Welcome to Quantum AI!',
    content: "Let's take a quick tour of the key features to get you started.",
    placement: 'bottom',
  },
  // Step 1: New Chat
  {
    targetId: 'new-chat',
    title: 'Start a Conversation',
    content: 'Click here to begin a new chat session with the AI.',
    placement: 'bottom',
    requiresSidebar: false,
  },
  // Step 2: Explore Hubs
  {
    targetId: 'sidebar-menu',
    title: 'Explore the Hubs',
    content: 'Access a wide range of powerful tools and capabilities, from financial analysis to creative content generation, all organized here.',
    placement: 'right',
    requiresSidebar: true,
  },
  // Step 3: Upload Document
  {
    targetId: 'upload-doc',
    title: 'Provide Knowledge',
    content: 'Upload .txt documents to give the AI context for your conversation. It can read and reference them.',
    placement: 'top',
  },
  // Step 4: Send Message
  {
    targetId: 'message-input',
    title: 'Engage with the AI',
    content: 'Type your message here and press Enter or the send button to chat.',
    placement: 'top',
  },
  // Step 5: Finish
  {
    targetId: 'finish',
    title: 'You\'re All Set!',
    content: 'You\'re now ready to explore the full capabilities of Quantum AI. Enjoy your experience!',
    placement: 'bottom',
  },
];

interface OnboardingProps {
  onFinish: () => void;
  setIsSidebarOpenForTutorial: (isOpen: boolean) => void;
  setHighlightedElementId: (id: string | null) => void;
}

const Onboarding: React.FC<OnboardingProps> = ({ onFinish, setIsSidebarOpenForTutorial, setHighlightedElementId }) => {
  const [stepIndex, setStepIndex] = useState(0);
  const [popoverStyle, setPopoverStyle] = useState<React.CSSProperties>({});
  const [highlightStyle, setHighlightStyle] = useState<React.CSSProperties>({});
  const popoverRef = useRef<HTMLDivElement>(null);

  const currentStep = steps[stepIndex];

  useLayoutEffect(() => {
    const { targetId, requiresSidebar, placement } = currentStep;

    setIsSidebarOpenForTutorial(!!requiresSidebar);
    setHighlightedElementId(targetId);

    const timeoutId = setTimeout(() => {
      // For the upload-doc step, we need to ensure the options panel is open
      if (targetId === 'upload-doc') {
        const optionsButton = document.querySelector('[aria-label="Toggle response options"]') as HTMLButtonElement;
        const optionsPanel = document.querySelector('.transition-all.duration-500.ease-in-out.overflow-hidden');
        if (optionsButton && optionsPanel && !optionsPanel.classList.contains('max-h-[500px]')) {
             optionsButton.click();
        }
      }

      const targetElement = document.querySelector(`[data-tutorial-id='${targetId}']`);

      if (targetId === 'welcome' || targetId === 'finish' || !targetElement) {
        setHighlightStyle({
            position: 'fixed',
            inset: 0,
            boxShadow: '0 0 0 9999px rgba(0, 0, 0, 0.7)',
            transition: 'opacity 0.3s ease-in-out',
            borderRadius: '0px',
        });
        setPopoverStyle({
            top: '50%',
            left: '50%',
            transform: 'translate(-50%, -50%)',
            opacity: 1,
        });
        return;
      }
      
      const targetRect = targetElement.getBoundingClientRect();
      const popoverEl = popoverRef.current;
      if (!popoverEl) return;

      const popoverRect = popoverEl.getBoundingClientRect();
      const spacing = 16;
      
      const newPopoverStyle: React.CSSProperties = { opacity: 1 };
      
      switch (placement) {
        case 'top':
          newPopoverStyle.top = `${targetRect.top - popoverRect.height - spacing}px`;
          newPopoverStyle.left = `${targetRect.left + targetRect.width / 2 - popoverRect.width / 2}px`;
          break;
        case 'bottom':
          newPopoverStyle.top = `${targetRect.bottom + spacing}px`;
          newPopoverStyle.left = `${targetRect.left + targetRect.width / 2 - popoverRect.width / 2}px`;
          break;
        case 'left':
          newPopoverStyle.left = `${targetRect.left - popoverRect.width - spacing}px`;
          newPopoverStyle.top = `${targetRect.top + targetRect.height / 2 - popoverRect.height / 2}px`;
          break;
        case 'right':
        default:
          newPopoverStyle.left = `${targetRect.right + spacing}px`;
          newPopoverStyle.top = `${targetRect.top + targetRect.height / 2 - popoverRect.height / 2}px`;
          break;
      }
      
      // Clamp position to be within viewport
      if (parseFloat(String(newPopoverStyle.top)) < spacing) newPopoverStyle.top = `${spacing}px`;
      if (parseFloat(String(newPopoverStyle.left)) < spacing) newPopoverStyle.left = `${spacing}px`;
      if (parseFloat(String(newPopoverStyle.left)) + popoverRect.width > window.innerWidth - spacing) {
          newPopoverStyle.left = `${window.innerWidth - popoverRect.width - spacing}px`;
      }
      if (parseFloat(String(newPopoverStyle.top)) + popoverRect.height > window.innerHeight - spacing) {
          newPopoverStyle.top = `${window.innerHeight - popoverRect.height - spacing}px`;
      }

      setPopoverStyle(newPopoverStyle);

      setHighlightStyle({
        position: 'fixed',
        left: `${targetRect.left}px`,
        top: `${targetRect.top}px`,
        width: `${targetRect.width}px`,
        height: `${targetRect.height}px`,
        boxShadow: '0 0 0 9999px rgba(0, 0, 0, 0.7)',
        borderRadius: '8px',
        transition: 'all 0.3s ease-in-out',
        pointerEvents: 'none',
      });
      
    }, requiresSidebar ? 350 : 100); // Increased delay slightly for panel opening

    return () => clearTimeout(timeoutId);
  }, [stepIndex, setIsSidebarOpenForTutorial, setHighlightedElementId]);

  const handleNext = () => {
    if (stepIndex < steps.length - 1) {
      setStepIndex(stepIndex + 1);
    } else {
      handleFinish();
    }
  };
  
  const handleFinish = () => {
    setHighlightedElementId(null);
    setIsSidebarOpenForTutorial(false);
    onFinish();
  };

  return (
    <div className="fixed inset-0 z-50">
      <div style={highlightStyle}></div>
      <div
        ref={popoverRef}
        style={popoverStyle}
        className="fixed z-[1001] w-80 max-w-[90vw] bg-gray-800 border border-blue-500/50 rounded-lg shadow-2xl p-6 transition-all duration-300 ease-in-out opacity-0"
      >
        <h3 className="text-xl font-bold text-blue-300 mb-2">{currentStep.title}</h3>
        <p className="text-gray-300 mb-6">{currentStep.content}</p>
        <div className="flex justify-between items-center">
          <button
            onClick={handleFinish}
            className="text-sm text-gray-400 hover:text-white transition-colors"
          >
            Skip Tutorial
          </button>
          <div className="flex items-center space-x-2">
            <span className="text-sm text-gray-500">{stepIndex + 1} / {steps.length}</span>
            <button
              onClick={handleNext}
              className="px-4 py-2 bg-blue-600 text-white rounded-md hover:bg-blue-500 transition-colors font-semibold"
            >
              {stepIndex === steps.length - 1 ? 'Finish' : 'Next'}
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Onboarding;
