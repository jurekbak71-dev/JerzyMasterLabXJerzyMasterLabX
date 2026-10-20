#!/usr/bin/env python3
"""Noncommercial listening comparisons through a real VST3, with licensed stems."""
import argparse
import hashlib
import json
import platform
import shutil
import subprocess
import urllib.request
from pathlib import Path
import numpy as np
import pedalboard
from pedalboard import load_plugin
from render_comparisons import RATE, source, write, measure, plugin_location

SOURCE_COMMIT='a41eaabb6761c9cdc3a079c45a1a2ccd575afea6'
SOURCE_URL=f'https://raw.githubusercontent.com/faroit/stempeg/{SOURCE_COMMIT}/stempeg/data/The%20Easton%20Ellises%20-%20Falcon%2069.stem.mp4'
SOURCE_SHA256='874a2552f4d6e2421789e9816f0db58337e97e20539579e34a6100029e3cde5d'
LICENSE_URL='https://creativecommons.org/licenses/by-nc-sa/3.0/'
SOURCE_README=f'https://github.com/faroit/stempeg/blob/{SOURCE_COMMIT}/README.md#reading-audio'


def ffmpeg():
    return shutil.which('ffmpeg') or __import__('imageio_ffmpeg').get_ffmpeg_exe()


def download_source(cache):
    cache.mkdir(parents=True,exist_ok=True)
    path=cache/'falcon.stem.mp4'
    if not path.exists():
        with urllib.request.urlopen(SOURCE_URL,timeout=60) as response:
            path.write_bytes(response.read())
    assert hashlib.sha256(path.read_bytes()).hexdigest()==SOURCE_SHA256,'Source checksum mismatch'
    # Names are embedded in the NI STEM metadata; this is not inferred by ear.
    assert b'"name": "Drums"' in path.read_bytes() and b'"name": "Vox"' in path.read_bytes()
    streams=[]
    for index in range(5):
        command=[ffmpeg(),'-v','error','-i',str(path),'-map',f'0:a:{index}','-f','f32le','-ar',str(RATE),'-ac','2','pipe:1']
        data=subprocess.run(command,capture_output=True,check=True).stdout
        streams.append(np.frombuffer(data,dtype='<f4').reshape(-1,2).T.copy())
    length=min(x.shape[1] for x in streams)
    assert length>=RATE*6,'Unexpected short stem source'
    return [x[:,:RATE*6] for x in streams]


def fixture(audio):
    # Three explicitly disclosed repetitions of the same six-second excerpt.
    audio=audio.astype(np.float64,copy=True)
    fade=int(RATE*.005)
    audio[:,:fade]*=np.linspace(0,1,fade)
    audio[:,-fade:]*=np.linspace(1,0,fade)
    audio=np.tile(audio,(1,3))
    audio=np.pad(audio,((0,0),(RATE//2,RATE//2)))
    assert np.max(np.abs(audio))>1e-5
    gain=10**(-6/20)/np.max(np.abs(audio))
    return (audio*gain).astype(np.float32),float(gain)


CASES=[
    ('01_sama_perkusja','Sama perkusja — rzeczywisty stem',0,
     [(220,-1.5,1,0),(4000,1,.7,0),(9000,.8,.707,2)],dict(threshold=-18,ratio=2,attack=25,release=100,drive=5,lim_release=90)),
    ('02_perkusja_i_bas','Perkusja + bas — rzeczywiste stemy',1,
     [(60,-1,.707,1),(300,-1.5,1,0),(3500,.8,.8,0)],dict(threshold=-20,ratio=1.6,attack=30,release=140,drive=5,lim_release=140,stereo=True,deq=[(180,-24,-1.5)])),
    ('03_zespol_rock_z_wokalem','Zespół electro-rock z wokalem — oryginalny miks',2,
     [(80,-.8,.707,1),(350,-1.5,1,0),(3000,.7,.8,0),(9000,.6,.707,2)],dict(threshold=-20,ratio=1.5,attack=25,release=160,drive=4,lim_release=160,analog=True,stereo=True,stabilizer=12,deq=[(3500,-28,-1.5),(7400,-30,-1)])),
    ('04_sam_wokal','Sam wokal — rzeczywisty stem Vox',3,
     [(70,0,.707,3),(300,-1,1,0),(2700,1,.8,0)],dict(threshold=-24,ratio=2,attack=10,release=130,drive=3,lim_release=150,upward=1,deq=[(6500,-32,-3)])),
    ('05_rock_bez_wokalu','Zespół electro-rock bez wokalu — suma trzech stemów',4,
     [(80,-.8,.707,1),(300,-1.2,1,0),(3000,.8,.8,0)],dict(threshold=-20,ratio=1.5,attack=30,release=160,drive=4,lim_release=160,stereo=True)),
    ('06_elektronika','Elektronika rytmiczna — własny syntetyczny fragment',5,
     [(75,-1.5,.707,1),(320,-1.5,1,0),(3500,.8,.8,0),(9000,.8,.707,2)],dict(threshold=-20,ratio=1.6,attack=20,release=120,drive=6,lim_release=100,stereo=True,wide=True,stabilizer=15,profile=1,deq=[(140,-24,-1.5)])),
    ('07_elektronika_przestrzenna','Elektronika przestrzenna / ambient — własny syntetyczny fragment',6,
     [(120,-.5,.707,1),(600,-1,.8,0),(7000,.7,.707,2)],dict(threshold=-26,ratio=1.25,attack=40,release=240,drive=3,lim_release=240,stereo=True,wide=True,stabilizer=10)),
]


def configure(path,bands,config):
    p=load_plugin(str(plugin_location(path)))
    def raw(key,value):
        assert key in p.parameters, f'Unsupported parameter: {key}'
        assert 0<=value<=1,(key,value)
        p.parameters[key].raw_value=float(value)
    for key in ['stereo_in_chain','compressor_in_chain','dynamic_eq_in_chain','stabilizer_in_chain']:
        raw(key,0)
    raw('eq_in_chain',1);raw('limiter_in_chain',1);raw('eq_enabled',1);raw('limiter_enabled',1)
    raw('module_chain_order',0);raw('limiter_before_eq_ceiling_applies_before_eq',0)
    for band in range(1,17):raw(f'band_{band}_enabled',0)
    for i,(freq,gain,q,typ) in enumerate(bands,1):
        prefix='eq_' if i==1 else f'band_{i}_'
        raw(prefix+'frequency',((freq-20)/19980)**.3);raw(prefix+'gain',(gain+18)/36);raw(prefix+'q',(q-.1)/9.9)
        raw(f'band_{i}_filter_type',typ/5);raw(f'band_{i}_enabled',1)
    raw('sample_peak_ceiling_dbfs',23/24)
    raw('limiter_drive_db',config['drive']/24)
    raw('limiter_release_ms',((config['lim_release']-10)/990)**.4)
    raw('compressor_in_chain',1);raw('compressor_enabled',1)
    raw('compressor_character',int(config.get('analog',False)))
    raw('compressor_threshold',(config['threshold']+60)/60);raw('compressor_ratio',(config['ratio']-1)/11)
    raw('compressor_attack',(config['attack']-.1)/99.9);raw('compressor_release',(config['release']-10)/990)
    raw('compressor_knee',6/18);raw('compressor_makeup',.5);raw('compressor_dry_wet',1)
    raw('upward_maximum_gain',config.get('upward',0)/18);raw('upward_threshold',18/54)
    if config.get('stereo'):
        raw('stereo_in_chain',1);raw('stereo_enabled',1);raw('stereo_mono_bass',1)
        raw('stereo_low_width',.3);raw('stereo_mid_width',.5)
        raw('stereo_high_width',.55 if config.get('wide') else .5);raw('stereo_air_width',.525 if config.get('wide') else .5)
    if config.get('stabilizer'):
        raw('stabilizer_in_chain',1);raw('stabilizer_enabled',1);raw('stabilizer_amount',config['stabilizer']/100)
        raw('stabilizer_profile',config.get('profile',0)/2);raw('stabilizer_response_time',.65);raw('stabilizer_spectral_smoothing',.7)
    for i in range(1,4):raw(f'dynamic_eq_{i}_enabled',0)
    if config.get('deq'):
        raw('dynamic_eq_in_chain',1);raw('dynamic_eq_enabled',1)
        for i,(hz,threshold,depth) in enumerate(config['deq'],1):
            prefix=f'dynamic_eq_{i}_';raw(prefix+'frequency',((hz-20)/19980)**.3)
            raw(prefix+'threshold',(threshold+60)/60);raw(prefix+'dynamic_range',(depth+12)/24)
            raw(prefix+'static_gain',.5);raw(prefix+'enabled',1)
    return p


def read_pcm24(path):
    import wave
    with wave.open(str(path),'rb') as wav:
        assert wav.getnchannels()==2 and wav.getframerate()==RATE and wav.getsampwidth()==3
        packed=np.frombuffer(wav.readframes(wav.getnframes()),dtype=np.uint8).reshape(-1,3)
    integer=packed[:,0].astype(np.int32)|(packed[:,1].astype(np.int32)<<8)|(packed[:,2].astype(np.int32)<<16)
    return (((integer^0x800000)-0x800000).astype(np.float32)/8388608).reshape(-1,2).T.copy()


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--plugin',required=True,type=Path);parser.add_argument('--output',required=True,type=Path);parser.add_argument('--cache',type=Path,default=Path('build-audio-source'));args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    stems=download_source(args.cache)
    # Verify that actual voice and drum streams are audible, not silent placeholders.
    for stream in [1,2,4]:assert np.sqrt(np.mean(stems[stream]**2))>1e-4
    real=[stems[1],stems[1]+stems[2],stems[0],stems[4],stems[1]+stems[2]+stems[3]]
    binary=args.plugin/('Contents/x86_64-win/JerzyMasterLabX.vst3' if platform.system()=='Windows' else 'Contents/x86_64-linux/JerzyMasterLabX.so')
    manifest={'platform':platform.system(),'sample_rate':RATE,'pcm_bits':24,'plugin_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'pedalboard':pedalboard.__version__,'numpy':np.__version__,'ffmpeg':subprocess.run([ffmpeg(),'-version'],capture_output=True,text=True,check=True).stdout.splitlines()[0],'source_url':SOURCE_URL,'source_sha256':SOURCE_SHA256,'source_license':LICENSE_URL,'examples':[]}
    # Host PDC is tested independently, never corrected by shifting rendered files.
    dry=load_plugin(str(plugin_location(args.plugin)))
    for key in ['eq_in_chain','limiter_in_chain','stereo_in_chain','compressor_in_chain','dynamic_eq_in_chain','stabilizer_in_chain']:dry.parameters[key].raw_value=0
    dry(np.zeros((2,RATE),np.float32),RATE,buffer_size=256)
    impulse=np.zeros((2,RATE),np.float32);impulse[:,12000]=.1
    aligned=dry(impulse,RATE,buffer_size=256)
    assert np.argmax(aligned[0])==12000 and np.max(np.abs(aligned-impulse))<1e-6,'Host PDC mismatch'
    manifest['dry_impulse_alignment']='passed; zero residual samples after host initialization'
    lines=['# Odsłuch MasterLab X — 7 porównań','',f'Rzeczywisty VST3, host Pedalboard / {platform.system()}, WAV stereo 24-bit / 48 kHz.','',
           '**Zacznij od PRZED i PO_LUFS_MATCH.** PO to bezpośredni wynik wtyczki, bez końcowej normalizacji. PO_LUFS_MATCH to ten sam wynik ze stałym gainem do LUFS źródła; plik MATCH nie musi spełniać ceiling wtyczki, gdy zostanie podgłośniony.',
           '', 'Pierwsze pięć przykładów: prawdziwe nagranie The Easton Ellises — Falcon. To **ten sam fragment 6 s powtórzony trzy razy**, z fade 5 ms na brzegach i 0,5 s ciszy przed/po. Oryginał jest AAC 44,1 kHz; WAV 48 kHz nie odzyskuje utraconych danych. Ostatnie dwa przykłady to własna synteza, nie nagrany zespół.',
           '', 'Wejścia otrzymały stały gain do peak −6 dBFS. Ustawienia dobrano ręcznie osobno dla materiału; **to nie jest wynik Master Brain**. Wszystkie parametry i gainy podaje measurements.json. Limiter jest sample-peak −1 dBFS, bez gwarancji True Peak. Nie oceniono odsłuchowo jakości ani nie testowano FL Studio.',
           '', '## Źródło i licencja', '',f'The Easton Ellises — Falcon. Źródło: {SOURCE_URL}',f'Potwierdzenie licencji nagrania: {SOURCE_README}',f'Nagranie i jego przetworzone wersje: **CC BY-NC-SA 3.0**, {LICENSE_URL}. Wyłącznie użycie niekomercyjne, z uznaniem autorstwa i zachowaniem tej licencji. Artyści nie firmują wtyczki. Zmiany: wybór/sumowanie stemów, wycinek, zapętlenie, fade, resampling, gain i DSP MasterLab X. Oryginalne syntetyczne przykłady 06/07: CC0 1.0, https://creativecommons.org/publicdomain/zero/1.0/.',
           '', '## Wyniki', '', '| Próbka | Wersja | LUFS-I | Peak dBFS | True Peak dBTP | Crest dB |','|---|---|---:|---:|---:|---:|']
    for name,title,index,bands,config in CASES:
        folder=args.output/name;folder.mkdir(exist_ok=True)
        if index<5:audio,gain=fixture(real[index])
        else:audio=source(index-5);gain=1.0
        before=folder/'PRZED.wav';write(before,audio);original=read_pcm24(before)
        p=configure(args.plugin,bands,config);p(np.zeros((2,RATE),np.float32),RATE,buffer_size=256)
        after=p(original,RATE,buffer_size=256)
        repeat=p(original,RATE,buffer_size=256);alternate=p(original,RATE,buffer_size=1024)
        repeat_error=float(np.max(np.abs(after-repeat)));block_error=float(np.max(np.abs(after-alternate)))
        assert after.shape==original.shape and np.isfinite(after).all()
        assert repeat_error<1e-6 and block_error<1e-6,(name,repeat_error,block_error)
        assert np.max(np.abs(after-original))>1e-4,'No audible processing change'
        assert np.max(np.abs(after))<=10**(-1/20)+1e-6,'Final sample ceiling exceeded'
        post=folder/'PO.wav';write(post,after)
        m_before=measure(before);m_after=measure(post);gain_match=m_before['input_i']-m_after['input_i']
        matched_audio=after*10**(gain_match/20)
        assert np.max(np.abs(matched_audio))<1,'Loudness matching would clip'
        matched=folder/'PO_LUFS_MATCH.wav';write(matched,matched_audio);m_matched=measure(matched)
        assert abs(m_matched['input_i']-m_before['input_i'])<=.15,'LUFS matching failed'
        files=[]
        for path,metrics in [(before,m_before),(post,m_after),(matched,m_matched)]:
            data=read_pcm24(path);metrics['correlation']=float(np.corrcoef(data)[0,1]);metrics['clipped_samples']=int(np.sum(np.abs(data)>=1));assert metrics['clipped_samples']==0
            files.append({'file':str(path.relative_to(args.output)).replace('\\','/'),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'measurements':metrics})
            lines.append(f"| {title} | {path.stem} | {metrics['input_i']:.2f} | {metrics['sample_peak_dbfs']:.2f} | {metrics['input_tp']:.2f} | {metrics['crest_db']:.2f} |")
        item={'name':name,'title':title,'seconds':original.shape[1]/RATE,'source_kind':'Falcon original licensed stems, three repeats' if index<5 else 'original synthesis','source_linear_gain_before_pcm':gain,'manual_config':config,'bands_hz_db_q_type':bands,'normalized_parameters':{key:float(value.raw_value) for key,value in p.parameters.items()},'repeat_max_error':repeat_error,'buffer_256_vs_1024_max_error':block_error,'match_gain_db':gain_match,'files':files}
        manifest['examples'].append(item)
        # Convenient listening preview: BEFORE, 0.75 s silence, AFTER LUFS MATCH.
        preview=folder/'ODSLUCH_PRZED_POTEM_PO_MATCH.mp3'
        pcm=np.concatenate([original,np.zeros((2,int(RATE*.75)),np.float32),matched_audio],axis=1).T.astype('<f4').tobytes()
        subprocess.run([ffmpeg(),'-v','error','-y','-f','f32le','-ar',str(RATE),'-ac','2','-i','pipe:0','-c:a','libmp3lame','-b:a','192k',str(preview)],input=pcm,capture_output=True,check=True)
        item['preview_sha256']=hashlib.sha256(preview.read_bytes()).hexdigest()
        print(f"PASS {name}: {m_before['input_i']:.2f} -> {m_after['input_i']:.2f} LUFS; repeat {repeat_error}; buffer {block_error}",flush=True)
    (args.output/'measurements.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    (args.output/'CZYTAJ_PRZED_ODSLUCHEM.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    (args.output/'LICENCJA_NAGRAN.txt').write_text('\n'.join(lines[10:17])+'\n\n'+LICENSE_URL+'\n',encoding='utf-8')
    assert len(manifest['examples'])==7
    print('PASS: 7 cases, 21 WAV, 7 MP3 previews, finite audio, zero clipped samples, fixed host PDC, repeat/buffer invariance and LUFS match',flush=True)

if __name__=='__main__':
    try:main()
    except Exception:
        import traceback
        print('::error::'+traceback.format_exc().replace('\r','').replace('\n','%0A'),flush=True)
        raise
