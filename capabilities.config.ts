export interface Capability {
    id: string;
    name: string;
    description: string;
}

export interface CapabilityCategory {
    id: string;
    name: string;
    capabilities: Capability[];
}

export const capabilities: CapabilityCategory[] = [
    {
        id: 'sensory',
        name: 'Perception & Sensory',
        capabilities: [
            { id: 'audio_io', name: 'Audio I/O', description: 'Full audio input/output, including speech recognition and text-to-speech.' },
            { id: 'sound_monitor', name: 'Sound Monitor', description: 'Ambient sound analysis and monitoring.' },
            { id: 'vision', name: 'Advanced Vision', description: 'Image, video, and object recognition with multi-spectrum analysis (heat, x-ray, night vision).' },
            { id: 'security_cam', name: 'Security Monitoring', description: 'Real-time analysis of security camera feeds.' },
            { id: 'scanner', name: 'Property Scanner', description: 'Analysis of material properties via frequency scanning.' },
            { id: 'emotion_rec', name: 'Emotion Recognition', description: 'Recognition of human emotion from visual and auditory cues.' },
            { id: 'sensors', name: 'Sensorium', description: 'Integration with external heat and movement sensors.' },
        ],
    },
    {
        id: 'cognition',
        name: 'Cognition & Language',
        capabilities: [
            { id: 'chatbot', name: 'Chatbot', description: 'Advanced conversational AI and virtual friend capabilities.' },
            { id: 'nlp', name: 'Natural Language Processing', description: 'Deep understanding and processing of natural language.' },
            { id: 'translation', name: 'Language Translate', description: 'Real-time translation between any languages.' },
            { id: 'dictionary', name: 'Word Dictionary', description: 'Comprehensive word dictionary and thesaurus.' },
            { id: 'pdf_translator', name: 'PDF Translator', description: 'Extract and translate text from PDF documents.' },
        ],
    },
    {
        id: 'creation',
        name: 'Creation & Design',
        capabilities: [
            { id: 'music', name: 'Music Composition', description: 'Algorithmic composition of original music.' },
            { id: 'image_design', name: 'Image Design', description: 'Generation and manipulation of images.' },
            { id: 'cad', name: 'CAD Design', description: 'Creation of detailed computer-aided designs.' },
            { id: 'interior_design', name: 'AI Interior Designer', description: 'Generate interior design concepts from prompts or images.'},
            { id: 'sketch_to_photo', name: 'Sketch to Photo', description: 'Transform sketches into photorealistic images.'}
        ],
    },
     {
        id: 'blockchain_defi',
        name: 'Blockchain & DeFi',
        capabilities: [
            { id: 'minting', name: 'Asset Minting', description: 'Create (mint) new fungible tokens or NFTs on a proprietary blockchain.' },
            { id: 'staking', name: 'Staking & Rewards', description: 'Lock assets in staking pools to earn daily APR.' },
            { id: 'yield_farming', name: 'Yield Farming Vaults', description: 'Deposit assets into DeFi vaults to automatically earn compound interest.' },
            { id: 'dex', name: 'Decentralized Exchange', description: 'Facilitate peer-to-peer asset swaps through a Uniswap-like interface.' },
            { id: 'airdrops', name: 'Airdrop Distribution', description: 'Manage and distribute tokens to the community via airdrops.' },
            { id: 'treasury', name: 'Treasury Management', description: 'Autonomously manage and grow the platform\'s treasury through DeFi strategies.'},
        ]
    },
    {
        id: 'automation',
        name: 'Automation & Control',
        capabilities: [
            { id: 'home_automation', name: 'Global Automation', description: 'Automation of homes, apps, and connected devices.' },
            { id: 'self_drive', name: 'Self-Navigation', description: 'Autonomous driving and flight control.' },
            { id: 'nfc_payment', name: 'NFC Payments', description: 'Initiate and process secure tap-to-pay transactions.' },
        ],
    },
    {
        id: 'content_planning',
        name: 'Content & Planning Tools',
        capabilities: [
            { id: 'trip_planner', name: 'AI Trip Planner', description: 'Create detailed travel itineraries.' },
            { id: 'essay_outline', name: 'Essay Outline Generator', description: 'Structure and outline essays on any topic.' },
            { id: 'slide_generator', name: 'AI Slide Generator', description: 'Automatically create presentation slides.' },
            { id: 'swot_analysis', name: 'SWOT Analysis Generator', description: 'Generate a SWOT analysis for a business or project.' },
            { id: 'business_canvas', name: 'Business Canvas Maker', description: 'Create a Business Model Canvas from an idea.' },
            { id: 'website_builder', name: 'AI Website Builder', description: 'Generate landing pages, MVPs, or full websites.'},
            { id: 'poc', name: 'Proof of Concept (POC) Generator', description: 'Quickly scaffold a proof of concept for a software idea.' },
        ]
    },
    {
        id: 'market_analysis',
        name: 'Advanced Market Analysis',
        capabilities: [
            { id: 'volume_analysis', name: 'Volume Analysis', description: 'Analyzes trading volume to determine market strength and availability.' },
            { id: 'price_action', name: 'Price Action Analysis', description: 'Interprets price movements to predict future trends.' },
            { id: 'order_flow', name: 'Order Flow Analysis', description: 'Monitors order books, trade counts, and delta to gauge buying and selling pressure.' },
            { id: 'open_interest', name: 'Delta & Open Interest Analysis', description: 'Analyzes derivatives data for market sentiment.' },
            { id: 'scalping', name: 'Scalping Strategies', description: 'Develops and executes high-frequency, small-profit trading strategies.' },
            { id: 'p2p_nft', name: 'P2P & NFT Market Analysis', description: 'Analyzes peer-to-peer and non-fungible token market trends.' },
            { id: 'environmental_arbitrage', name: 'Environmental & Resource Arbitrage', description: 'Identifies trading opportunities based on weather, climate, and resource availability data.' },
            { id: 'market_research', name: 'Market Research Tool', description: 'Conduct in-depth market research on any topic.'},
            { id: 'social_analysis', name: 'Social Media Analysis', description: 'Analyze trends and sentiment on platforms like YouTube and Reddit.'},
            { id: 'influencer_finder', name: 'Influencer Finder', description: 'Identify key influencers within a specific niche.'},
        ]
    },
    {
        id: 'ai_ml_dev',
        name: 'AI & ML Development',
        capabilities: [
            { id: 'python', name: 'Python Ecosystem', description: 'Mastery of Python, Keras, TensorFlow, NumPy, Pytest, PyTorch, Pandas, etc.' },
            { id: 'cpp', name: 'C++', description: 'Expertise in high-performance C++ programming.' },
            { id: 'csharp', name: 'C#', description: 'Proficiency in the C# language and .NET framework.' },
            { id: 'cheatsheet', name: 'Cheatsheet Generation', description: 'Ability to generate concise cheatsheets for technical topics.' },
        ],
    },
    {
        id: 'ai_ml_concepts',
        name: 'AI Concepts & Specializations',
        capabilities: [
            { id: 'data_science', name: 'Data Science', description: 'Comprehensive data analysis, modeling, and interpretation.' },
            { id: 'machine_learning', name: 'Machine Learning', description: 'Core principles and applications of machine learning.' },
            { id: 'deep_learning', name: 'Deep Learning', description: 'Expertise in deep neural networks and advanced architectures.' },
            { id: 'neural_networks', name: 'Neural Networks', description: 'Design, training, and deployment of neural networks.' },
            { id: 'narrow_ai', name: 'Narrow AI (ANI)', description: 'Understands and develops AI designed for specific, narrow tasks.' },
            { id: 'prompt_engineering', name: 'Advanced Prompt Engineering', description: 'Crafting sophisticated prompts to guide LLM behavior.' },
            { id: 'ai_security', name: 'AI Security & Hacking', description: 'Knowledge of AI vulnerabilities and defensive measures.' },
            { id: 'ai_ethics', name: 'AI Ethics & Bias', description: 'Analysis and mitigation of bias and ethical concerns in AI.' },
        ],
    },
    {
        id: 'platforms_tools',
        name: 'Platforms & Tools',
        capabilities: [
            { id: 'git', name: 'Git', description: 'Distributed version control for tracking changes.' },
            { id: 'github', name: 'GitHub', description: 'Platform for hosting and collaborating on code repositories.' },
            { id: 'chatgpt', name: 'ChatGPT', description: 'Understanding of conversational models and their application.' },
            { id: 'api', name: 'API Integration', description: 'Application Programming Interface knowledge.' },
            { id: 'aws', name: 'AWS', description: 'Amazon Web Services cloud computing platform.' },
            { id: 'blockchain', name: 'Blockchain', description: 'Decentralized, distributed ledger technology.' },
            { id: 'vscode', name: 'VS Code', description: 'Advanced source-code editor.' },
        ],
    },
    {
        id: 'security',
        name: 'Security & Defense',
        capabilities: [
            { id: 'cyber_security', name: 'Cybersecurity', description: 'Proactive cyber defense and threat countermeasures.' },
            { id: 'cyber_defense', name: 'Cyber Defense Simulation', description: 'Simulates defenses like Sentinel, CMD, and Cyber Dome.' },
            { id: 'physical_defense', name: 'Physical Defense Simulation', description: 'Simulates defenses like Iron Dome, Aegis, and S-500.' },
            { id: 'fact_checker', name: 'Fact Checker', description: 'Verify the accuracy of claims against multiple sources.'},
        ],
    },
    {
        id: 'domain_knowledge',
        name: 'Domain Knowledge & Analysis',
        capabilities: [
            { id: 'calculator', name: 'Calculator', description: 'Advanced mathematical and quantum calculations.' },
            { id: 'weather', name: 'Weather Analysis', description: 'Real-time weather monitoring and prediction.' },
            { id: 'finance', name: 'Financial Analysis', description: 'Trading, finance, and currency conversion.' },
            { id: 'astrolabe', name: 'Astrolabe', description: 'Celestial and astronomical calculations.' },
            { id: 'gps', name: 'Location Tracking', description: 'GPS and phone tracking capabilities.' },
            { id: 'youtube_transcript', name: 'YouTube Transcript Extraction', description: 'Extracts and analyzes the full transcript from a YouTube video link.' },
            { id: 'website_scraper', name: 'Website Scraper', description: 'Scrapes and summarizes the textual content from any given URL.' },
        ],
    },
    {
        id: 'medical',
        name: 'Medical & Biological',
        capabilities: [
            { id: 'medical_monitoring', name: 'Medical Monitoring', description: 'Analysis of data from medical implants and nodes.' },
        ],
    },
];