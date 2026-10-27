"""First-party, NO-WEIGHTS toy formant-waveform experiment.
This is intentionally NOT claimed as intelligible or natural text-to-speech.
Real Genesis neural TTS needs licensed speech recordings and a trained model.
"""
from __future__ import annotations
from pathlib import Path
import numpy as np
import wave

FORMANTS = {
 'a': (850,1220,2810), 'e':(530,1840,2480), 'i':(270,2290,3010),
 'o':(570,840,2410), 'u':(300,870,2240), 'y':(340,1700,2500)
}

def synthesize_waveform(text:str, sample_rate:int=16000) -> np.ndarray:
    """Toy sound articulation by letters; NOT a speech recognisable English voice."""
    sections=[]
    for letter in text.lower()[:200]:
        duration=0.095 if letter!=' ' else 0.05
        n=int(sample_rate*duration)
        t=np.arange(n,dtype=np.float32)/sample_rate
        if letter in FORMANTS:
            f1,f2,f3=FORMANTS[letter]
            # Harmonic excitation with broad formant envelopes; simple DSP only.
            harmonics=np.arange(1,72,dtype=np.float32)
            freq=harmonics*115
            envelope=(np.exp(-0.5*((freq-f1)/120)**2)+
                      .6*np.exp(-0.5*((freq-f2)/150)**2)+
                      .3*np.exp(-0.5*((freq-f3)/230)**2))
            pulse=(np.sin(2*np.pi*t[:,None]*freq[None,:])*
                   envelope[None,:]).sum(axis=1)
            pulse=pulse/(max(np.max(np.abs(pulse)),1e-6))
        elif letter==' ':
            pulse=np.zeros(n,dtype=np.float32)
        else:
            # Broad-band consonant surrogate, deterministic (not phoneme synthesis).
            rng=np.random.default_rng(ord(letter))
            pulse=rng.normal(0,0.12,n).astype(np.float32)
        env=np.minimum(1,t/.008)*np.minimum(1,(duration-t)/.015)
        sections.append((pulse*env).astype(np.float32))
    if not sections:return np.zeros(1,dtype=np.float32)
    return .25*np.concatenate(sections)


def write_wave(text:str,filename:str,sample_rate:int=16000)->None:
    signal=synthesize_waveform(text,sample_rate)
    data=(np.clip(signal,-1,1)*32767).astype('<i2')
    Path(filename).parent.mkdir(parents=True,exist_ok=True)
    with wave.open(str(filename),'wb') as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(sample_rate)
        wav.writeframes(data.tobytes())
