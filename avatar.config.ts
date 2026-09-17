import { AvatarConfig } from './types';

export const presetAvatars: { name: string; config: AvatarConfig }[] = [
    {
        name: 'Operator',
        config: {
            base: 'androgynous',
            height: 185,
            build: 60,
            skinColor: '#8d6e63',
            faceShape: 2,
            eyeColor: '#4fc3f7',
            hairstyle: 3,
            outfit: 1,
        }
    },
    {
        name: 'Analyst',
        config: {
            base: 'masculine',
            height: 178,
            build: 40,
            skinColor: '#c5a593',
            faceShape: 4,
            eyeColor: '#66bb6a',
            hairstyle: 6,
            outfit: 2,
        }
    },
    {
        name: 'Creator',
        config: {
            base: 'feminine',
            height: 165,
            build: 30,
            skinColor: '#efdcd5',
            faceShape: 1,
            eyeColor: '#ab47bc',
            hairstyle: 7,
            outfit: 3,
        }
    },
    {
        name: 'Guardian',
        config: {
            base: 'masculine',
            height: 200,
            build: 90,
            skinColor: '#5d4037',
            faceShape: 5,
            eyeColor: '#ffa726',
            hairstyle: 1,
            outfit: 0,
        }
    }
];
