
import { AIToolCategory } from './types';

export const aiToolCategories: AIToolCategory[] = [
  {
    id: 'writing',
    name: 'Writing',
    tools: [
      { id: 'content-creation', name: 'Content Creation', description: 'Generate articles, blog posts, or social media updates on any topic.', prompt: 'Write a blog post about the future of AI in...' },
      { id: 'book-writing', name: 'Book Writing', description: 'Assist in outlining, drafting, and refining chapters for a book.', prompt: 'Outline a science fiction novel about...' },
      { id: 'fact-checker', name: 'AI Fact Checker', description: 'Verify claims and get factual information from reliable sources.', prompt: 'Fact check the claim that...' },
      { id: 'swot-analysis', name: 'SWOT Analysis', description: 'Generate a SWOT (Strengths, Weaknesses, Opportunities, Threats) analysis for a business or idea.', prompt: 'Create a SWOT analysis for a new electric vehicle startup.' },
    ],
  },
  {
    id: 'image',
    name: 'Image',
    tools: [
      { id: 'color-analysis', name: 'AI Color Analysis', description: 'Analyze and suggest color palettes from an image or a concept.', prompt: 'Suggest a color palette for a website with a "calm and professional" feel.' },
      { id: 'sketch-converter', name: 'Sketch to Photo', description: 'Convert a simple sketch into a photorealistic image.', prompt: 'Convert my sketch of a futuristic city into a realistic image.' },
      { id: 'logo-generator', name: 'Logo Generator', description: 'Design a unique logo for a brand or project based on a description.', prompt: 'Generate a minimalist logo for a coffee shop named "The Quantum Bean".' },
      { id: 'banner-creator', name: 'Banner Creator', description: 'Create banners and cover images for social media or websites.', prompt: 'Create a YouTube banner for a channel about retro gaming.' },
    ],
  },
  {
    id: 'video',
    name: 'Video',
    tools: [
      { id: 'video-designer', name: 'AI Video Designer', description: 'Generate short video clips from text prompts or images.', prompt: 'Generate a video of a robot walking on Mars.' },
      { id: 'theme-builder', name: 'Theme Builder', description: 'Create visual themes and storyboards for a video project.', prompt: 'Create a visual theme for a short film with a cyberpunk aesthetic.' },
    ],
  },
  {
    id: 'marketing',
    name: 'Marketing',
    tools: [
      { id: 'market-research', name: 'Market Research Tool', description: 'Conduct in-depth market analysis for a product or industry.', prompt: 'Do market research on the global demand for sustainable packaging.' },
      { id: 'business-canvas', name: 'Business Canvas Maker', description: 'Generate a Business Model Canvas for a startup idea.', prompt: 'Create a business model canvas for a subscription box service for pets.' },
    ],
  },
  {
    id: 'code',
    name: 'Code',
    tools: [
      { id: 'website-builder', name: 'AI Website Builder', description: 'Generate the code for a full website based on a description.', prompt: 'Build a single-page portfolio website for a graphic designer.' },
      { id: 'app-builder', name: 'AI App Builder', description: 'Scaffold a mobile or web application from a set of requirements.', prompt: 'Generate the boilerplate code for a to-do list app using React and Firebase.' },
      { id: 'repo-deployment', name: 'GitHub Repository Deployment', description: 'Create and deploy a new code repository to GitHub.', prompt: 'Create a new GitHub repository named "quantum-app" with a basic Python starter.' },
      { id: 'api-builder', name: 'API Builder', description: 'Design and generate code for a RESTful API based on a data model.', prompt: 'Design an API for a simple blog with endpoints for posts and users.' },
    ],
  },
  {
    id: 'chatbot',
    name: 'Chatbot',
    tools: [
      { id: 'unified-qa', name: 'Unified Quantum AI', description: 'The core conversational AI for general purpose tasks and queries.', prompt: 'Explain quantum computing in simple terms.' },
      { id: 'chat-bot-creator', name: 'Custom Chatbot', description: 'Design and configure a specialized chatbot for a specific purpose (e.g., customer service).', prompt: 'Create a customer service chatbot for an e-commerce store that can answer questions about shipping.' },
    ],
  },
  {
    id: 'research',
    name: 'Research',
    tools: [
      { id: 'deep-research', name: 'Deep Research', description: 'Perform a comprehensive analysis of a topic across the web, academic papers, and more.', prompt: 'Conduct deep research on the latest advancements in CRISPR gene editing.' },
      { id: 'nanobrain', name: 'Nanobrain AI', description: 'Simulate and analyze complex systems at a molecular level.', prompt: 'Simulate the interaction of a new drug compound with a target protein.' },
    ],
  },
  {
    id: 'speech',
    name: 'Speech',
    tools: [
        { id: 'tts', name: 'Text-to-Speech', description: 'Convert any text into natural-sounding speech.', prompt: 'Generate audio for the following text: "Welcome to the future."' },
        { id: 'stt', name: 'Speech-to-Text', description: 'Transcribe spoken audio into text with high accuracy.', prompt: 'Transcribe the audio from the uploaded file.' },
    ]
  },
];