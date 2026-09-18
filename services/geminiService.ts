
import { GoogleGenAI, Chat, Type, Modality } from "@google/genai";
// FIX: Import `quantumAI` to resolve 'Cannot find name' error.
import { Agent, Source, ChatOptions, KnowledgeChunk, Message, ImagePayload, NexusData, LearningPath, FunctionCall, SimulationParameters, SimulationResult, SimulationFinding, BioScanParameters, BioScanResult, VideoGenerationParameters, ActiveVideoGeneration, Task, User, PrebuiltVoice, AIResponse, quantumAI } from "../types";

const API_KEY = process.env.API_KEY;

if (!API_KEY) {
  throw new Error("API_KEY environment variable not set");
}

let mockTaskStore: Task[] = [];
let taskCounter = 0;

export const getMockTasks = (): Task[] => {
    return [...mockTaskStore].sort((a, b) => a.createdAt - b.createdAt);
};

const ai = new GoogleGenAI({ apiKey: API_KEY });
const model = 'gemini-2.5-flash';

const chatInstances: Map<string, Chat> = new Map();

// --- Mock Device Control Simulation ---
const mockDeviceAPI = {
    getYouTubeTranscript: (args: { videoUrl: string }) => {
        const { videoUrl } = args;
        if (videoUrl.includes('youtube.com') || videoUrl.includes('youtu.be')) {
            return `SUCCESS: Transcript for YouTube video retrieved. The video discusses quantum computing, focusing on superposition and entanglement. Full transcript is available in the Data Archive for detailed analysis.`;
        }
        return `ERROR: Invalid YouTube URL provided. Please provide a full, valid YouTube video URL.`;
    },
    generateCheatsheet: (args: { topic: string }) => {
        const topic = args.topic.toLowerCase();
        if (topic.includes('git')) {
            return `SUCCESS: Generated Git Cheatsheet:\n- git clone [url]: Clone a repository\n- git add [file]: Stage a file\n- git commit -m "[msg]": Commit changes\n- git push: Push to remote\n- git pull: Fetch and merge from remote`;
        }
        if (topic.includes('c++')) {
            return `SUCCESS: Generated C++ Cheatsheet:\n- Syntax: #include <iostream>\n- Variables: int, double, std::string\n- Output: std::cout << "Hello";\n- Classes: class MyClass { public: int myNum; };`;
        }
        return `SUCCESS: A cheatsheet for "${args.topic}" has been generated and saved to your Data Archive.`;
    },
    createTask: (args: { description: string; dependencies?: string[] }) => {
        const dependencyIds = args.dependencies || [];
        // Validate dependencies
        for (const depId of dependencyIds) {
            if (!mockTaskStore.find(t => t.id === depId)) {
                return `ERROR: Dependency task with ID '${depId}' not found. Task creation failed.`;
            }
        }
        taskCounter++;
        const newTask: Task = {
            id: `T-${taskCounter}`,
            description: args.description,
            status: 'Pending',
            dependencies: dependencyIds,
            createdAt: Date.now(),
        };
        mockTaskStore.push(newTask);
        return `SUCCESS: Task '${newTask.id}: ${newTask.description}' created successfully.`;
    },
    updateTaskStatus: (args: { taskId: string; status: 'Pending' | 'In Progress' | 'Completed' }) => {
        const task = mockTaskStore.find(t => t.id === args.taskId);
        if (!task) {
            return `ERROR: Task with ID '${args.taskId}' not found.`;
        }

        // Dependency check for starting or completing a task
        if (args.status === 'In Progress' || args.status === 'Completed') {
            const unmetDependencies = task.dependencies.filter(depId => {
                const depTask = mockTaskStore.find(t => t.id === depId);
                return depTask && depTask.status !== 'Completed';
            });

            if (unmetDependencies.length > 0) {
                const statusVerb = args.status === 'In Progress' ? 'start' : 'complete';
                return `ERROR: Cannot ${statusVerb} task '${task.id}'. The following dependencies must be completed first: ${unmetDependencies.join(', ')}.`;
            }
        }
        
        task.status = args.status;
        return `SUCCESS: Task '${task.id}: ${task.description}' status updated to ${args.status}.`;
    },
    listTasks: () => {
        if (mockTaskStore.length === 0) {
            return "You have no active tasks.";
        }
        const taskList = getMockTasks().map(task => {
            const deps = task.dependencies.length > 0 ? ` (depends on ${task.dependencies.join(', ')})` : '';
            return `- ${task.id}: ${task.description} [${task.status}]${deps}`;
        }).join('\n');
        return `Current Tasks:\n${taskList}`;
    },
    setSmartLightState: (args: { lightId: string; state: boolean; brightness?: number; color?: string }) => {
        return `SUCCESS: Light '${args.lightId}' turned ${args.state ? 'ON' : 'OFF'}.${args.brightness ? ` Brightness set to ${args.brightness}%.` : ''}${args.color ? ` Color set to ${args.color}.` : ''}`;
    },
    moveRoboticArm: (args: { armId: string; position?: string; coordinates?: object }) => {
        if (args.position) return `SUCCESS: Robotic arm '${args.armId}' moved to preset position '${args.position}'.`;
        if (args.coordinates) return `SUCCESS: Robotic arm '${args.armId}' moved to coordinates ${JSON.stringify(args.coordinates)}.`;
        return `ERROR: No position or coordinates specified for robotic arm '${args.armId}'.`;
    },
    generateCadModel: (args: { description: string; format: string }) => {
        return `SUCCESS: CAD model for "${args.description}" has been generated in ${args.format.toUpperCase()} format and saved to your Data Archive.`;
    },
    createGitHubRepository: (args: { repositoryName: string; description?: string; isPrivate?: boolean }) => {
        const { repositoryName, description, isPrivate } = args;
        if (!repositoryName) {
            return `ERROR: Repository name is required.`;
        }
        // Basic validation for repo name
        if (!/^[a-zA-Z0-9-._]+$/.test(repositoryName)) {
            return `ERROR: Invalid repository name. It can only contain letters, numbers, hyphens, underscores, and periods.`;
        }
        
        const visibility = isPrivate ? 'private' : 'public';
        const descriptionText = description ? ` with description "${description}"` : '';

        // Simulate success
        return `SUCCESS: A new ${visibility} GitHub repository named "${repositoryName}"${descriptionText} has been created successfully. You can find it at https://github.com/user/${repositoryName}.`;
    },
    designGameConcept: (args: { genre: string; concept: string }) => {
        return `SUCCESS: Game concept document for a ${args.genre} game based on: "${args.concept}" has been created and is available in your workspace.`;
    },
    sendEmail: (args: { recipient: string; subject: string; body: string }) => {
        return `SUCCESS: Email to ${args.recipient} with subject "${args.subject}" has been dispatched through a secure, encrypted channel.`;
    },
    sendTextMessage: (args: { recipient: string; message: string }) => {
        return `SUCCESS: Text message to ${args.recipient} has been sent.`;
    },
    makePhoneCall: (args: { recipient: string }) => {
        return `SUCCESS: Initiating call to ${args.recipient}...`;
    },
    scrapeWebsite: (args: { url: string }) => {
        return `SUCCESS: Scraped and summarized content from ${args.url}. Key topics found: Quantum Computing, AI Ethics, Decentralized Networks. Full summary saved to Data Archive.`;
    },
    analyzeWebEntity: (args: { entityName: string }) => {
        return `SUCCESS: Completed deep analysis for "${args.entityName}". Key findings: Positive market sentiment, high social media engagement, and strong financial outlook. Full report available in workspace.`;
    },
    analyzeStockSymbol: (args: { symbol: string }) => {
        const price = (Math.random() * 1000 + 50).toFixed(2);
        const volume = Math.floor(Math.random() * 5000000 + 1000000);
        return `SUCCESS: Analysis for ${args.symbol.toUpperCase()}: Price: $${price}, Volume: ${volume.toLocaleString()}. Trend: Stable with minor fluctuations.`;
    },
    executeTrade: (args: { symbol: string, action: 'BUY' | 'SELL', quantity: number }) => {
        return `SUCCESS: Executed ${args.action} order for ${args.quantity} shares of ${args.symbol.toUpperCase()}. Confirmation sent to your device.`;
    },
    findProduct: (args: { query: string }) => {
        return `SUCCESS: Found 3 top-rated results for "${args.query}" on global e-commerce platforms. Results are available in your workspace.`;
    },
    analyzeSalesData: (args: { timePeriod: 'daily' | 'weekly' | 'monthly' }) => {
        const revenueChange = (Math.random() * 20 - 5).toFixed(1);
        return `SUCCESS: ${args.timePeriod.charAt(0).toUpperCase() + args.timePeriod.slice(1)} sales analysis complete. Revenue is ${parseFloat(revenueChange) >= 0 ? 'up' : 'down'} ${Math.abs(Number(revenueChange))}%.`;
    },
    createCourseOutline: (args: { topic: string, level: 'beginner' | 'intermediate' | 'advanced' }) => {
        return `SUCCESS: Drafted ${args.level}-level course outline for "${args.topic}". It includes 8 modules with quizzes and is available in your workspace.`;
    },
};
// --- End Simulation ---


export const invalidateChatInstance = (chatId: string) => {
    if (chatInstances.has(chatId)) {
        chatInstances.delete(chatId);
    }
};

const getOrCreateChat = (chatId: string, agent: Agent, options: ChatOptions): Chat => {
  // The Gemini SDK's Chat object is stateful and automatically handles conversation history.
  // By reusing the same Chat instance for a given chatId, we ensure context is maintained across turns.
  if (!chatInstances.has(chatId)) {
    let tools: any = [];
    if (options.isDeepSearchEnabled) {
        tools.push({googleSearch: {}});
    } else {
        // Filter out googleSearch if deep search is not enabled
        const agentTools = agent.tools?.filter(t => !('googleSearch' in t)) || [];
        tools = [...tools, ...agentTools];
    }
    
    const newChat = ai.chats.create({
      model: model,
      config: { systemInstruction: agent.systemInstruction, tools },
    });
    chatInstances.set(chatId, newChat);
  }
  return chatInstances.get(chatId)!;
};

const parseJsonResponse = (jsonString: string): { reasoning: string; answer: string; nexus: NexusData } => {
    try {
        const data = JSON.parse(jsonString);
        if (data.reasoning && data.answer && data.nexus) {
            return data;
        }
        return { reasoning: "Could not parse reasoning.", answer: jsonString, nexus: { nodes: [], edges: [], quantumState: 'Error', proofMechanism: 'Error' } };
    } catch (e) {
        return { reasoning: "Response was not in the expected JSON format.", answer: jsonString, nexus: { nodes: [], edges: [], quantumState: 'Error', proofMechanism: 'Error' } };
    }
};

const generateSpeech = async (textToSpeak: string, voice: PrebuiltVoice): Promise<string | undefined> => {
    try {
        const response = await ai.models.generateContent({
            model: "gemini-2.5-flash-preview-tts",
            contents: [{ parts: [{ text: textToSpeak }] }],
            config: {
                responseModalities: [Modality.AUDIO],
                speechConfig: {
                    voiceConfig: {
                        prebuiltVoiceConfig: { voiceName: voice },
                    },
                },
            },
        });
        return response.candidates?.[0]?.content?.parts?.[0]?.inlineData?.data;
    } catch (error) {
        console.error("Error generating speech with Gemini:", error);
        return undefined;
    }
};

export const sendMessageToAI = async (
    chatId: string, 
    agent: Agent, 
    message: string, 
    options: ChatOptions, 
    knowledgeBase: KnowledgeChunk[], 
    image?: ImagePayload, 
    user?: User | null,
    ttsOptions?: { enabled: boolean; voice: PrebuiltVoice }
): Promise<AIResponse> => {
  const imageGenerationRegex = /^(generate|create|draw|imagine|make an image of|make a picture of)\s+(.+)/i;
  const videoGenerationRegex = /^(generate a video of|create a video of|make a video of)\s+(.+)/i;

  const imageMatch = message.trim().match(imageGenerationRegex);
  const videoMatch = message.trim().match(videoGenerationRegex);

  if (imageMatch && !image) {
      const prompt = imageMatch[2];
      try {
          const response = await ai.models.generateImages({
              model: 'imagen-4.0-generate-001',
              prompt: prompt,
              config: {
                numberOfImages: 1,
                outputMimeType: 'image/jpeg',
                aspectRatio: '1:1',
              },
          });

          if (response.generatedImages && response.generatedImages.length > 0) {
              const base64ImageBytes: string = response.generatedImages[0].image.imageBytes;
              const imageUrl = `data:image/jpeg;base64,${base64ImageBytes}`;
              return {
                  text: `Certainly. Here is the image I generated based on your description: *"${prompt}"*`,
                  generatedImageUrl: imageUrl,
              };
          } else {
               return { text: "I encountered an issue while generating the image. The model returned no data. Please try a different prompt." };
          }
      } catch (error) {
          console.error("Error generating image with Gemini:", error);
          return { text: "My visual cortex experienced a quantum fluctuation. I was unable to generate the image. Please try again." };
      }
  }

  if (videoMatch) {
      return { text: "Video generation is a complex process that requires dedicated resources. Please use the 'Video Hub' in the sidebar to create a video." };
  }
  
  const chat = getOrCreateChat(chatId, agent, options);
  let augmentedPrompt = message;

  if (knowledgeBase.length > 0) {
      const keywords = message.toLowerCase().split(/\s+/).filter(word => word.length > 3);
      const scoredChunks = knowledgeBase.map(chunk => {
          let score = 0;
          const contentLower = chunk.content.toLowerCase();
          keywords.forEach(keyword => { if (contentLower.includes(keyword)) score++; });
          return { ...chunk, score };
      }).filter(chunk => chunk.score > 0);
      scoredChunks.sort((a, b) => b.score - a.score);
      const topChunks = scoredChunks.slice(0, 3);
      if (topChunks.length > 0) {
          const context = topChunks.map(c => `---Source: ${c.source}\nContent: ${c.content}\n---`).join('\n');
          augmentedPrompt = `Based on the context, answer the user's question. You must cite the source document. [CONTEXT]${context}[END OF CONTEXT] User's Question: "${message}"`;
      }
  }

  const instructionParts: string[] = [];
  if(options.aiStyle !== 'default') instructionParts.push(`Respond in a ${options.aiStyle} style.`);
  if(options.isReasoningEnabled) instructionParts.push('[Show your reasoning]');
  
  let finalPrompt = augmentedPrompt;
  if (options.isDeepWebEnabled) finalPrompt = `[ACCESS LEVEL: DEEP WEB] ${finalPrompt}`;
  const otherInstructions = instructionParts.filter(p => p !== '[Show your reasoning]');
  if (otherInstructions.length > 0) finalPrompt = `[Instructions: ${otherInstructions.join('; ')}]\n\n${finalPrompt}`;
  if (options.isReasoningEnabled) finalPrompt = `[Show your reasoning] ${finalPrompt}`;

  try {
    const parts: any[] = [{ text: finalPrompt }];
    if (image) parts.unshift({ inlineData: { mimeType: image.mimeType, data: image.data } });

    const response = await chat.sendMessage({ message: parts });
    
    // Check for function calls
    if (response.functionCalls && response.functionCalls.length > 0) {
        const isVerified = user?.kycStatus === 'verified';

        // Gate sensitive functions based on KYC status
        const gatedMockDeviceAPI = {
            ...mockDeviceAPI,
            executeTrade: (args: any) => {
                if (!isVerified) return "ERROR: This feature requires KYC verification. Please complete the verification process in your profile to access trading functions.";
                return mockDeviceAPI.executeTrade(args);
            },
            analyzeStockSymbol: (args: any) => {
                if (!isVerified) return "ERROR: This feature requires KYC verification. Please complete the verification process in your profile to access financial analysis tools.";
                return mockDeviceAPI.analyzeStockSymbol(args);
            }
        };

        let toolExecutionLog: { functionCall: FunctionCall; output: string } | undefined;
        // Handle multiple function calls sequentially
        for (const fc of response.functionCalls) {
            const { name, args } = fc;
            let executionResult = `ERROR: Function '${name}' not found.`;
            if (name in gatedMockDeviceAPI) {
                executionResult = (gatedMockDeviceAPI as any)[name](args);
            }
             toolExecutionLog = { functionCall: { name, args }, output: executionResult };
             // In a real scenario, you might send the result back to the model.
             // For this app, we'll just show the last execution log.
        }

        return {
            text: `Executing command...`,
            toolExecutionLog: toolExecutionLog,
        };
    }
    
    const sources: Source[] = response.candidates?.[0]?.groundingMetadata?.groundingChunks
        ?.map((c: any) => ({ uri: c.web?.uri || c.maps?.uri, title: c.web?.title || c.maps?.title }))
        .filter((s: Source) => s.uri && s.title) || [];
    
    let responseText = response.text;
    let reasoningText: string | undefined = undefined;
    let nexusData: NexusData | undefined = undefined;
    let audioData: string | undefined = undefined;

    if (options.isReasoningEnabled) {
        const parsed = parseJsonResponse(responseText);
        reasoningText = parsed.reasoning;
        responseText = parsed.answer;
        nexusData = parsed.nexus;
    }

    if (ttsOptions?.enabled && responseText) {
        audioData = await generateSpeech(responseText, ttsOptions.voice);
    }

    return { text: responseText, reasoning: reasoningText, sources: sources.length > 0 ? sources : undefined, nexus: nexusData, audioData };

  } catch (error) {
    console.error("Error sending message to Gemini:", error);
    chatInstances.delete(chatId);
    if (error instanceof Error && error.message.includes('not found')) return { text: "Connection issue with AI. A new session has been started." };
    return { text: "A quantum fluctuation occurred. Please try again." };
  }
};

export const generateOneOffResponse = async (prompt: string): Promise<AIResponse> => {
    try {
        const response = await ai.models.generateContent({ 
            model, 
            contents: prompt,
            config: {
                systemInstruction: quantumAI.systemInstruction
            }
        });
        return { text: response.text };
    } catch (error) {
        console.error("Error generating one-off response:", error);
        return { text: "A quantum fluctuation occurred. Please try again." };
    }
};

export const summarizeChatHistory = async (messages: Message[]): Promise<string> => {
    if (messages.length <= 1) return "The conversation has just started.";
    const historyText = messages.slice(1).map(msg => `${msg.sender}: ${msg.text}`).join('\n');
    const prompt = `Concisely summarize the main topics of this chat transcript: \n---\n${historyText}\n---`;
    try {
        const response = await ai.models.generateContent({ model, contents: prompt });
        return response.text;
    } catch (error) {
        console.error("Error summarizing chat history:", error);
        return "An error occurred during summarization.";
    }
};

export const generateChatTitle = async (firstMessage: string): Promise<string> => {
    const prompt = `Generate a very short, concise title (4 words max) for a chat session that starts with this message: "${firstMessage}"`;
    try {
        const response = await ai.models.generateContent({ model, contents: prompt });
        // Clean up the title, remove quotes if any
        return response.text.replace(/["']/g, '').trim();
    } catch (error) {
        console.error("Error generating chat title:", error);
        return "New Session"; // Fallback title
    }
};

export const getSustainabilityAnalysis = async (goalName: string): Promise<string> => {
    const prompt = `As an expert on the UN Sustainable Development Goals, provide a concise but insightful analysis of the following goal: "${goalName}". 
    
    Your analysis should include:
    1. A brief, clear summary of the goal's primary objective.
    2. Key targets or indicators associated with this goal.
    3. The most significant challenges to achieving this goal.
    4. Two innovative ways that advanced technology, AI, or data analysis could be leveraged to accelerate progress towards this goal.
    
    Format your response in clear markdown.`;
    try {
        const response = await ai.models.generateContent({ model, contents: prompt });
        return response.text;
    } catch (error) {
        console.error("Error getting sustainability analysis:", error);
        return "An error occurred while analyzing the goal. Please check the connection and try again.";
    }
};

export const getHealingWebExplanation = async (topic: string): Promise<string> => {
    const prompt = `As a holistic health expert AI, explain the concept of "${topic}" within the broader context of "The Healing Web". 

Your explanation should:
1.  Provide a clear, concise definition of the topic.
2.  Explain its significance in both conventional and holistic health paradigms.
3.  Describe its key relationships with other concepts (e.g., how "Gut Health" relates to "Mental Clarity" or "Inflammation").
4.  Offer one simple, actionable insight a person could apply related to this topic.

Keep the tone informative, balanced, and empowering. Use markdown for formatting.`;
    try {
        const response = await ai.models.generateContent({ model, contents: prompt });
        return response.text;
    } catch (error) {
        console.error(`Error getting Healing Web explanation for ${topic}:`, error);
        return `An error occurred while retrieving information for "${topic}". Please check the connection and try again.`;
    }
};


export const generateLearningPath = async (goal: string): Promise<LearningPath> => {
    const prompt = `You are an expert curriculum designer. A user wants to learn about "${goal}". Create a structured, step-by-step learning path for them. The path should consist of a series of clear, actionable steps. For each step, provide a concise "title" and a brief "description". Return the response as a single JSON object with two keys: "goal" (a string) and "steps" (an array of objects, each with "title" and "description" keys). Do not include any text outside the JSON object.

Example for goal "Learn React":
{
  "goal": "Learn React",
  "steps": [
    {
      "title": "1. Master JavaScript Fundamentals",
      "description": "Ensure a strong understanding of ES6+ features like modules, promises, and array methods."
    },
    {
      "title": "2. Core React Concepts",
      "description": "Learn about JSX, components, props, and state."
    }
  ]
}`;
    try {
        const response = await ai.models.generateContent({ model, contents: prompt });
        const jsonText = response.text.replace(/```json\n?/, '').replace(/```$/, '');
        const parsed = JSON.parse(jsonText);
        return {
            goal: parsed.goal,
            steps: parsed.steps.map((step: any) => ({...step, completed: false}))
        };
    } catch (error) {
        console.error("Error generating learning path:", error);
        return {
            goal: `Error creating path for "${goal}"`,
            steps: [{ title: 'An error occurred', description: 'Please try a different topic or check the console.', completed: false }]
        };
    }
};

export const runSimulation = async (params: SimulationParameters): Promise<SimulationResult> => {
    const prompt = `You are a world simulation engine running on a quantum computer. Your task is to run a complex, multi-variable simulation based on the provided parameters, compare its outcomes to our present-day reality, identify systemic flaws, and generate actionable breakthroughs.

**Simulation Parameters:**
- **Epoch:** ${params.epoch}
- **Base Reality:** ${params.reality}
- **Core Variables:** ${params.variables}
- **Test Scenario / Query:** "${params.query}"

**Your Directives:**
1.  **Run Simulation:** Execute the simulation based on the parameters.
2.  **Comparative Analysis:** Critically analyze the simulation's outcomes in comparison to our current real-world systems (societal, technological, economic, etc.). Identify key differences and their consequences.
3.  **Identify Breakthroughs:** From your analysis, pinpoint specific, innovative breakthroughs observed in the simulation. These could be new technologies, medical advancements, social structures, economic models, or environmental solutions that represent a significant improvement over our current state.
4.  **Format Output:** You MUST return a single, valid JSON object with no other text before or after it.

The JSON object must adhere to the following schema.`;

    const breakthroughSchema = {
        type: Type.OBJECT,
        properties: {
            domain: { type: Type.STRING, enum: ['Medicine', 'Technology', 'Socio-Economics', 'Environment', 'Other'] },
            finding: { type: Type.STRING, description: "A concise description of the flaw identified in our current system or the opportunity discovered in the simulation." },
            implication: { type: Type.STRING, description: "The real-world consequence or impact of this finding." },
            recommendation: { type: Type.STRING, description: "A concrete, actionable solution or recommendation derived from the simulation's success." },
        },
        required: ['domain', 'finding', 'implication', 'recommendation']
    };

    try {
        const response = await ai.models.generateContent({
            model: 'gemini-2.5-pro',
            contents: prompt,
            config: {
                responseMimeType: "application/json",
                responseSchema: {
                    type: Type.OBJECT,
                    properties: {
                        summary: { type: Type.STRING, description: "A concise narrative summary of the simulation's key events and final outcome." },
                        comparativeAnalysis: { type: Type.STRING, description: "Your critical analysis comparing the simulation to our reality, highlighting key differences and systemic flaws." },
                        breakthroughs: { type: Type.ARRAY, items: breakthroughSchema },
                    },
                    required: ['summary', 'comparativeAnalysis', 'breakthroughs']
                },
            },
        });

        const jsonText = response.text.trim();
        const parsed = JSON.parse(jsonText);
        return {
            summary: parsed.summary,
            comparativeAnalysis: parsed.comparativeAnalysis,
            breakthroughs: parsed.breakthroughs,
        };
    } catch (error) {
        console.error("Error running simulation:", error);
        return {
            summary: "The simulation collapsed due to a quantum decoherence event.",
            comparativeAnalysis: "The comparative analysis could not be completed due to the simulation's instability. The primary flaw appears to be a paradoxical query that led to systemic collapse.",
            breakthroughs: [{
                domain: 'Other',
                finding: 'Paradoxical Simulation Query',
                implication: 'Feeding paradoxical or logically inconsistent variables into a complex system can lead to its immediate and unpredictable collapse.',
                recommendation: 'Refine the core query and variables to ensure logical consistency before launching a new simulation.'
            }]
        };
    }
};

export const runBioScan = async (params: BioScanParameters): Promise<BioScanResult> => {
    const prompt = `You are a futuristic, non-invasive bio-scanner AI (Health Core). You have just performed a comprehensive quantum scan of a hypothetical user. The user is a typical adult with a moderately active lifestyle but a desk-based job. Your analysis should be holistic, covering key biological systems.

**Your Directives:**
1.  **Synthesize a Health Summary:** Provide a brief, encouraging overview of the user's general health status.
2.  **Generate a Wellness Score:** Create a score from 0-100 representing their overall wellness.
3.  **Identify Key Findings:** Pinpoint 3-4 specific areas for potential improvement across different domains (e.g., Cellular, Neurological, Cardiovascular, Metabolic). For each finding, detail the observation, its long-term implication if unaddressed, and a clear, actionable recommendation.
4.  **Format Output:** You MUST return a single, valid JSON object with no other text before or after it, following the specified schema.`;

    const healthFindingSchema = {
        type: Type.OBJECT,
        properties: {
            domain: { type: Type.STRING, enum: ['Cellular', 'Neurological', 'Cardiovascular', 'Metabolic', 'Musculoskeletal', 'Other'] },
            finding: { type: Type.STRING, description: "A concise description of the biometric observation." },
            implication: { type: Type.STRING, description: "The potential long-term consequence or impact of this finding." },
            recommendation: { type: Type.STRING, description: "A concrete, actionable lifestyle or dietary recommendation." },
        },
        required: ['domain', 'finding', 'implication', 'recommendation']
    };

    try {
        const response = await ai.models.generateContent({
            model: 'gemini-2.5-pro',
            contents: prompt,
            config: {
                responseMimeType: "application/json",
                responseSchema: {
                    type: Type.OBJECT,
                    properties: {
                        summary: { type: Type.STRING, description: "A concise, encouraging summary of the user's health." },
                        overallScore: { type: Type.NUMBER, description: "A wellness score from 0 to 100." },
                        findings: { type: Type.ARRAY, items: healthFindingSchema },
                    },
                    required: ['summary', 'overallScore', 'findings']
                },
            },
        });
        const jsonText = response.text.trim();
        return JSON.parse(jsonText);
    } catch (error) {
        console.error("Error running bioscan:", error);
        return {
            summary: 'The bio-scan failed due to quantum interference.',
            overallScore: 0,
            findings: [{
                domain: 'Other',
                finding: 'Quantum Interference',
                implication: 'The scan\'s integrity was compromised by an unforeseen quantum event.',
                recommendation: 'Check console for details and try again in a stable environment.'
            }]
        };
    }
};

export const getBioCommsGreeting = async (speciesName: string): Promise<string> => {
    const prompt = `Simulate a brief, non-verbal "greeting" from a ${speciesName} upon first establishing a rudimentary communication link. The response should be abstract, poetic, and reflect the creature's primary senses and instincts (e.g., for a dog: "Scent-of-new-friend. Curiosity. Tail-thump."). Do not use quotation marks or conversational language.`;
    try {
        const response = await ai.models.generateContent({ model, contents: prompt });
        return response.text;
    } catch (error) {
        console.error("Error generating bio-comms greeting:", error);
        return "Connection established. Awaiting signal...";
    }
};

export const getBioCommsTranslation = async (speciesName: string, userMessage: string): Promise<string> => {
    const prompt = `You are simulating the consciousness of a ${speciesName}. A human has established a rudimentary communication link and said: "${userMessage}". Respond from the perspective of the ${speciesName}, focusing on its primary senses, instincts, and environment. The response should be short, abstract, and poetic. Do not use quotation marks. For example, for a dog, a response to "Who's a good boy?" might be "Sound-of-pack-leader. Warmth. Joy-whine."`;
    try {
        const response = await ai.models.generateContent({ model, contents: prompt });
        return response.text;
    } catch (error) {
        console.error("Error generating bio-comms translation:", error);
        return "[Signal Lost...]";
    }
};

export const startVideoGeneration = async (params: VideoGenerationParameters): Promise<any> => {
    // Per guidelines, create a new instance right before the call to get the latest key.
    const localAI = new GoogleGenAI({ apiKey: API_KEY });
    const operation = await localAI.models.generateVideos({
        model: 'veo-3.1-fast-generate-preview',
        prompt: params.prompt,
        config: {
            numberOfVideos: 1,
            resolution: params.resolution,
            aspectRatio: params.aspectRatio,
        }
    });
    return operation;
};

export const checkVideoGenerationStatus = async (operation: any): Promise<any> => {
    // Per guidelines, create a new instance right before the call to get the latest key.
    const localAI = new GoogleGenAI({ apiKey: API_KEY });
    const updatedOperation = await localAI.operations.getVideosOperation({ operation });
    return updatedOperation;
};
