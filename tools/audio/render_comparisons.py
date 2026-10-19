#!/usr/bin/env python3
"""Render original synthetic fixtures through the actual built VST3. No GUI needed."""
import argparse, hashlib, json, math, subprocess, wave, shutil, platform
from pathlib import Path
import numpy as np
from pedalboard import load_plugin
import pedalboard

RATE=48000

def source(kind):
    rng=np.random.default_rng(57994+kind)
    duration=22; n=duration*RATE; t=np.arange(n)/RATE
    audio=np.zeros((2,n),np.float64)
    def add(pos,signal,pan=0):
        start=int(pos*RATE); end=min(n,start+len(signal));signal=signal[:end-start]
        if len(signal):
            audio[0,start:end]+=signal*math.sqrt((1-pan)/2)
            audio[1,start:end]+=signal*math.sqrt((1+pan)/2)
    def pulse(pos,amp=1):
        q=np.arange(int(.5*RATE))/RATE
        phase=2*np.pi*(48*q+95*.025*(1-np.exp(-q/.025)))
        add(pos,amp*.7*np.sin(phase)*np.exp(-q/ .105))
    beat=60/(120 if kind==0 else 82)
    for pos in np.arange(.5,20.5,beat):
        pulse(pos,.95 if kind==0 else .55)
    for pos in np.arange(.5,20.5,beat/2):
        q=np.arange(int(.09*RATE))/RATE; noise=rng.standard_normal(len(q));noise=np.r_[noise[0],np.diff(noise)]
        add(pos,.035*noise*np.exp(-q/.018),rng.uniform(-.6,.6))
    for pos in np.arange(.5+beat,20.5,beat*2):
        q=np.arange(int(.22*RATE))/RATE
        add(pos,.18*rng.standard_normal(len(q))*np.exp(-q/.055)+.18*np.sin(2*np.pi*185*q)*np.exp(-q/.035))
    chords=[[130.8128,155.5635,195.9977],[103.8262,130.8128,155.5635],[116.5409,146.8324,174.6141],[97.9989,123.4708,146.8324]]
    for step,pos in enumerate(np.arange(.5,20.5,beat*4)):
        chord=chords[step%4];length=min(beat*4+1,22-pos);q=np.arange(int(length*RATE))/RATE
        env=np.minimum(q/(.12 if kind==0 else .6),1)*np.exp(-q/(2 if kind==0 else 3))
        for voice,freq in enumerate(chord):
            for side in [-1,1]:
                signal=sum(np.sin(2*np.pi*freq*(1+side*.0015)*k*q)/k**1.9 for k in range(1,7))
                add(pos,.05*signal*env,side*.65)
        for offset in np.arange(0,beat*4,beat/2 if kind==0 else beat):
            z=np.arange(int(.35*RATE))/RATE;freq=chord[0]*.5
            bass=.23*(np.sin(2*np.pi*freq*z)+.18*np.sin(4*np.pi*freq*z))*np.minimum(z/.008,1)*np.exp(-z/.14)
            add(pos+offset,bass)
    # Gentle fade; the initial half second permits parameter smoothing to settle.
    audio*=np.minimum(t/.5,1)*np.minimum(np.maximum((22-t)/1.5,0),1)
    audio*=10**(-6/20)/np.max(np.abs(audio))
    return audio.astype(np.float32)

def write(path,a):
    assert np.isfinite(a).all() and np.max(np.abs(a))<1
    pcm=np.rint(a.T.reshape(-1)*8388607).astype('<i4').view(np.uint8).reshape(-1,4)[:,:3].copy()
    with wave.open(str(path),'wb') as f:
        f.setnchannels(2);f.setsampwidth(3);f.setframerate(RATE);f.writeframes(pcm.tobytes())

def measure(path):
    result=subprocess.run([shutil.which('ffmpeg') or __import__('imageio_ffmpeg').get_ffmpeg_exe(),'-hide_banner','-nostats','-i',str(path),'-af','loudnorm=I=-14:TP=-1:LRA=11:print_format=json','-f','null','-'],capture_output=True,text=True,check=True)
    text=result.stderr; metrics=json.loads(text[text.rfind('{'):text.rfind('}')+1])
    out={k:float(metrics[k]) for k in ['input_i','input_tp','input_lra','input_thresh']}
    with wave.open(str(path),'rb') as wav:packed=np.frombuffer(wav.readframes(wav.getnframes()),dtype=np.uint8).reshape(-1,3)
    integer=packed[:,0].astype(np.int32)|(packed[:,1].astype(np.int32)<<8)|(packed[:,2].astype(np.int32)<<16)
    samples=((integer^0x800000)-0x800000).astype(np.float64)/8388608
    out['sample_peak_dbfs']=float(20*np.log10(np.max(np.abs(samples))))
    out['rms_dbfs']=float(10*np.log10(np.mean(samples*samples)))
    out['crest_db']=out['sample_peak_dbfs']-out['rms_dbfs']
    return out

def plugin(path,kind):
    p=load_plugin(str(path))
    def raw(key,v):p.parameters[key].raw_value=float(v)
    for band in range(1,17):raw(f'band_{band}_enabled',0)
    # Fixed, documented manual processing; not an automatic mastering claim.
    bands=[(75,-1.5,.707,1),(320,-1.8,1.0,0),(3500,1.2,.8,0),(9000,1.0,.707,2)] if kind==0 else [(100,-.8,.707,1),(600,-1.2,.8,0),(7000,.8,.707,2)]
    for i,(freq,gain,q,typ) in enumerate(bands,1):
        prefix='eq_' if i==1 else f'band_{i}_'
        raw(prefix+'frequency',((freq-20)/19980)**.3);raw(prefix+'gain',(gain+18)/36);raw(prefix+'q',(q-.1)/9.9)
        raw(f'band_{i}_filter_type',typ/5);raw(f'band_{i}_enabled',1)
    raw('eq_enabled',1);raw('limiter_enabled',1);raw('limiter_before_eq_ceiling_applies_before_eq',0)
    raw('sample_peak_ceiling_dbfs',23/24);raw('limiter_drive_db',(7 if kind==0 else 4)/24);raw('limiter_release_ms',(((140 if kind==0 else 220)-10)/990)**.4)
    return p,bands

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--plugin',required=True,type=Path);parser.add_argument('--output',required=True,type=Path);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    binary=args.plugin/('Contents/x86_64-win/JerzyMasterLabX.vst3' if platform.system()=='Windows' else 'Contents/x86_64-linux/JerzyMasterLabX.so')
    manifest={'platform':platform.system(),'sample_rate':RATE,'pcm_bits':24,'source':'Original deterministic synthetic musical fixtures; no commercial recordings.','plugin_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'pedalboard':pedalboard.__version__,'numpy':np.__version__,'tests':{},'examples':[],'known_limit':'Sample-peak limiter, not True Peak; direct outputs are not peak-normalized.'}
    # Check host PDC after its first sample-rate initialization, without modifying files afterwards.
    dry=load_plugin(str(args.plugin));dry.eq_enabled=False;dry.limiter_enabled=False
    dry(np.zeros((2,RATE),np.float32),RATE,buffer_size=256)
    impulse=np.zeros((2,RATE),np.float32);impulse[:,12000]=.1
    result=dry(impulse,RATE,buffer_size=256)
    assert np.argmax(result[0])==12000 and np.max(np.abs(result-impulse))<1e-6,'host PDC failed'
    manifest['tests']['dry_impulse_alignment']='passed, zero residual samples after host initialization'
    for kind,name in enumerate(['01_rytmiczny','02_przestrzenny']):
        original=source(kind);p,bands=plugin(args.plugin,kind)
        p(np.zeros((2,RATE),np.float32),RATE,buffer_size=256)
        processed=p(original,RATE,buffer_size=256)
        repeated=p(original,RATE,buffer_size=256)
        alternate=p(original,RATE,buffer_size=1024)
        error=float(np.max(np.abs(processed-repeated)));block_error=float(np.max(np.abs(processed-alternate)))
        assert error<1e-6 and block_error<1e-6 and processed.shape==original.shape and np.isfinite(processed).all()
        assert np.max(np.abs(processed-original))>.001,'processing produced no change'
        assert np.max(np.abs(processed))<=10**(-1/20)+1e-6
        before=args.output/f'{name}_PRZED.wav';after=args.output/f'{name}_PO.wav';matched=args.output/f'{name}_PO_LUFS_MATCH.wav'
        write(before,original);write(after,processed);m_before=measure(before);m_after=measure(after)
        gain_db=m_before['input_i']-m_after['input_i'];assert gain_db<=0
        write(matched,processed*10**(gain_db/20));m_matched=measure(matched)
        assert abs(m_matched['input_i']-m_before['input_i'])<=.15
        item={'name':name,'seconds':original.shape[1]/RATE,'bands_hz_db_q_type':bands,'drive_db':7 if kind==0 else 4,'release_ms':140 if kind==0 else 220,'ceiling_dbfs':-1,'eq_then_limiter':True,'determinism_max_error':error,'buffer_256_vs_1024_max_error':block_error,'match_gain_db':gain_db,'files':[]}
        for file,metrics in [(before,m_before),(after,m_after),(matched,m_matched)]:
            item['files'].append({'file':file.name,'measurements':metrics,'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
        manifest['examples'].append(item)
    (args.output/'measurements.json').write_text(json.dumps(manifest,indent=2)+'\n')
    report='# MasterLab X — pliki przed i po\n\nOryginalne syntetyczne fragmenty, 22 s, stereo WAV 24-bit / 48 kHz. Host: Pedalboard, '+platform.system()+'.\n\nPRZED = źródło. PO = rzeczywisty VST3 (bez normalizacji). PO_LUFS_MATCH = ten sam wynik ściszony stałym gainem do LUFS źródła. Zacznij porównanie od PRZED i PO_LUFS_MATCH.\n\nCeiling to sample peak -1 dBFS, nie True Peak. Szczyty między próbkami mogą przekroczyć ceiling — patrz pomiary poniżej. To test techniczny i materiał do Twojego odsłuchu, nie dowód jakości masteringu ani test FL Studio.\n\n| Plik | LUFS-I | Peak dBFS | TP dBTP |\n|---|---:|---:|---:|\n'
    for example in manifest['examples']:
        report+=''
        for item in example['files']:
            m=item['measurements'];report+=f"| {item['file']} | {m['input_i']:.2f} | {m['sample_peak_dbfs']:.2f} | {m['input_tp']:.2f} |\n"
    report+='\nUstawienia: EQ → limiter. Rytmiczny Drive 7 dB, Release 140 ms; przestrzenny Drive 4 dB, Release 220 ms. Szczegóły EQ, testy powtarzalności/buforów, wersje narzędzi i SHA-256 są w measurements.json. Pusty render inicjalizuje PDC hosta; test impulsowy potwierdza następnie wyrównanie 0 próbek.\n'
    (args.output/'CZYTAJ_PRZED_ODSLUCHEM.md').write_text(report,encoding='utf-8')
    print(json.dumps(manifest,indent=2))
if __name__=='__main__':main()
