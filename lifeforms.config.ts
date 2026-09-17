export interface Species {
  name: string;
  type: 'mammal' | 'avian' | 'marine' | 'flora' | 'insect' | 'reptile';
  description: string;
  icon: string; // emoji or simple representation
}

export interface LifeFormCategory {
  name: string;
  species: Species[];
}

export const lifeForms: LifeFormCategory[] = [
  {
    name: 'Terrestrial Fauna',
    species: [
      { name: 'Dog', type: 'mammal', description: 'Canis lupus familiaris. Strong olfactory senses, pack-oriented.', icon: '🐶' },
      { name: 'Cat', type: 'mammal', description: 'Felis catus. Solitary hunter, attuned to subtle vibrations.', icon: '🐱' },
      { name: 'Wolf', type: 'mammal', description: 'Canis lupus. Complex vocalizations and body language.', icon: '🐺' },
      { name: 'Fox', type: 'mammal', description: 'Vulpes. Wide range of calls, adaptable survivor.', icon: '🦊' },
      { name: 'Elephant', type: 'mammal', description: 'Loxodonta. Complex social structure, uses infrasound.', icon: '🐘' },
      { name: 'Bear', type: 'mammal', description: 'Ursidae. Communication through scent marking and vocalizations.', icon: '🐻' },
    ],
  },
  {
    name: 'Avian Life',
    species: [
      { name: 'Eagle', type: 'avian', description: 'Aquila. Apex aerial predator with exceptional vision.', icon: '🦅' },
      { name: 'Parrot', type: 'avian', description: 'Psittaciformes. Known for vocal mimicry and high intelligence.', icon: '🦜' },
      { name: 'Crow', type: 'avian', description: 'Corvus. Problem-solver, complex social interactions.', icon: '🐦' },
      { name: 'Owl', type: 'avian', description: 'Strigiformes. Nocturnal hunter, silent flight, diverse hooting calls.', icon: '🦉' },
    ],
  },
  {
    name: 'Marine Life',
    species: [
      { name: 'Dolphin', type: 'marine', description: 'Delphinus. Highly intelligent, communicates via sonar.', icon: '🐬' },
      { name: 'Octopus', type: 'marine', description: 'Octopoda. Master of camouflage, communicates via chromatophores (light).', icon: '🐙' },
      { name: 'Humpback Whale', type: 'marine', description: 'Megaptera novaeangliae. Communicates through complex, long-range songs.', icon: '🐳' },
      { name: 'Shark', type: 'marine', description: 'Selachimorpha. Senses electrical fields through ampullae of Lorenzini.', icon: '🦈' },
      { name: 'Jellyfish', type: 'marine', description: 'Medusozoa. Primitive nervous system, communicates via bioluminescence.', icon: '🦑' },
    ],
  },
  {
    name: 'Flora',
    species: [
      { name: 'Oak Tree', type: 'flora', description: 'Quercus. Communicates via mycorrhizal networks and chemical signals.', icon: '🌳' },
      { name: 'Venus Flytrap', type: 'flora', description: 'Dionaea muscipula. Responds to physical touch and vibration.', icon: '🪴' },
      { name: 'Redwood', type: 'flora', description: 'Sequoioideae. Ancient organism, part of vast interconnected forest root systems.', icon: '🌲' },
      { name: 'Mushroom', type: 'flora', description: 'Fungus. Central nodes in the "wood wide web", facilitating inter-plant communication.', icon: '🍄' },
    ],
  },
   {
    name: 'Reptiles & Amphibians',
    species: [
        { name: 'Snake', type: 'reptile', description: 'Serpentes. Senses ground vibrations and chemical trails via tongue.', icon: '🐍' },
        { name: 'Frog', type: 'reptile', description: 'Anura. Communicates through a wide variety of croaks and calls.', icon: '🐸' },
        { name: 'Chameleon', type: 'reptile', description: 'Chamaeleonidae. Changes skin color for communication and camouflage.', icon: '🦎' },
    ]
   },
   {
    name: 'Insects & Arachnids',
    species: [
        { name: 'Ant', type: 'insect', description: 'Formicidae. Complex societies built on pheromone trails.', icon: '🐜' },
        { name: 'Bee', type: 'insect', description: 'Apoidea. Communicates location of resources through waggle dance.', icon: '🐝' },
        { name: 'Spider', type: 'insect', description: 'Araneae. Senses world through web vibrations.', icon: '🕷️' },
    ]
   }
];
