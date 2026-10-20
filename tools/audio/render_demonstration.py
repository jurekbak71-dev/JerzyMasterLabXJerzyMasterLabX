#!/usr/bin/env python3
"""Known defects -> unchanged actual Brain -> strong isolated manual DSP -> measured audio."""
import argparse,hashlib,json,subprocess,platform
from pathlib import Path
import numpy as np
import pedalboard
from pedalboard import load_plugin,PeakFilter
from pedalboard._pedalboard import normalize_python_parameter_name
from render_comparisons import RATE,write,measure,plugin_location
from render_genre_tests import download_source,fixture,read_pcm24,ffmpeg,LICENSE_URL,SOURCE_URL,SOURCE_SHA256


def plugin(path):
    p=load_plugin(str(plugin_location(path)))
    for name in ['eq_in_chain','limiter_in_chain','stereo_in_chain','compressor_in_chain','dynamic_eq_in_chain','stabilizer_in_chain']:p.parameters[name].raw_value=0
    return p


def raw(p,key,value):
    assert key in p.parameters,key
    assert 0<=value<=1,(key,value)
    p.parameters[key].raw_value=float(value)


def manual(path,index):
    p=plugin(path)
    if index==0:
        raw(p,'eq_in_chain',1);raw(p,'eq_enabled',1)
        for i in range(1,17):raw(p,f'band_{i}_enabled',0)
        raw(p,'band_1_enabled',1);raw(p,'eq_frequency',((120-20)/19980)**.3);raw(p,'eq_gain',8/36);raw(p,'eq_q',(.7-.1)/9.9)
    elif index==1:
        raw(p,'dynamic_eq_in_chain',1);raw(p,'dynamic_eq_enabled',1)
        for i in range(1,4):raw(p,f'dynamic_eq_{i}_enabled',int(i==1))
        for key,value in {'frequency':((3500-20)/19980)**.3,'q':(2.5-.2)/5.8,'threshold':18/60,'dynamic_range':0,'attack':(.1-.1)/99.9,'release':60/990,'static_gain':.5}.items():raw(p,'dynamic_eq_1_'+key,value)
    elif index==2:
        raw(p,'compressor_in_chain',1);raw(p,'compressor_enabled',1);raw(p,'compressor_character',0)
        for key,value in {'threshold':28/60,'ratio':5/11,'attack':(.1-.1)/99.9,'release':50/990,'knee':3/18,'makeup':.5,'dry_wet':1}.items():raw(p,'compressor_'+key,value)
        raw(p,'upward_maximum_gain',8/18);raw(p,'upward_threshold',20/54)
        raw(p,'limiter_in_chain',1);raw(p,'limiter_enabled',1);raw(p,'limiter_drive_db',12/24);raw(p,'sample_peak_ceiling_dbfs',18/24);raw(p,'limiter_release_ms',((80-10)/990)**.4)
    else:
        raw(p,'stereo_in_chain',1);raw(p,'stereo_enabled',1);raw(p,'stereo_mono_bass',1)
        raw(p,'stereo_low_crossover',1);raw(p,'stereo_low_width',0);raw(p,'stereo_mid_width',.35);raw(p,'stereo_high_width',.5);raw(p,'stereo_air_width',.5)
    return p


def render(p,a):
    p(np.zeros((2,RATE),np.float32),RATE,buffer_size=256)
    result=p(a,RATE,buffer_size=256);repeat=p(a,RATE,buffer_size=256);other=p(a,RATE,buffer_size=1024)
    assert result.shape==a.shape and np.isfinite(result).all()
    error=float(np.max(np.abs(result-repeat)));block=float(np.max(np.abs(result-other)))
    assert error<1e-6 and block<1e-6,(error,block)
    assert np.max(np.abs(result))<1,'Raw render would clip exported PCM'
    return result,{'repeat_max_error':error,'buffer_256_vs_1024_max_error':block}


def features(a):
    # Relative band energies are level independent. Exclude boundary fades.
    phrase_levels=[10*np.log10(np.mean(a[:,RATE//2+i*6*RATE:RATE//2+(i+1)*6*RATE].astype(np.float64)**2)+1e-20) for i in range(3)]
    a=a[:,RATE:-RATE].astype(np.float64);n=a.shape[1]
    bins=np.fft.rfftfreq(n,1/RATE);z=np.fft.rfft(a*np.hanning(n),axis=1);energy=np.mean(np.abs(z)**2,axis=0)
    def e(lo,hi):return float(np.sum(energy[(bins>=lo)&(bins<hi)]))+1e-24
    low=(bins>=20)&(bins<140);left=z[0,low];right=z[1,low];mid=(left+right)*.5;side=(left-right)*.5
    powers=[]
    for offset in range(0,n-RATE//2,RATE//2):powers.append(10*np.log10(np.mean(a[:,offset:offset+RATE//2]**2)+1e-20))
    return {'bass_to_mid_db':10*np.log10(e(60,200)/e(500,2000)),
            'harsh_to_mid_db':10*np.log10(e(2800,4300)/e(500,1800)),
            'low_side_to_mid_db':float(10*np.log10((np.sum(np.abs(side)**2)+1e-24)/(np.sum(np.abs(mid)**2)+1e-24))),
            'low_correlation':float(np.real(np.sum(left*np.conj(right)))/np.sqrt(np.sum(np.abs(left)**2)*np.sum(np.abs(right)**2))),
            'phrase_rms_range_db':float(np.ptp(phrase_levels)),
            'rms_window_spread_db':float(np.percentile(powers,90)-np.percentile(powers,10))}


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--plugin',type=Path,required=True);parser.add_argument('--brain-tool',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--cache',type=Path,default=Path('build-audio-source'));args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    stems=download_source(args.cache);mix,_=fixture(stems[0]);drums,_=fixture(stems[1]);n=mix.shape[1];t=np.arange(n)/RATE
    bass=PeakFilter(cutoff_frequency_hz=120,gain_db=10,q=.7)(mix,RATE)
    harsh=PeakFilter(cutoff_frequency_hz=3500,gain_db=10,q=2.5)(mix,RATE)
    env=np.where((np.maximum(t-.5,0)/6).astype(int)%2==0,1.,.16)
    env=np.convolve(env,np.ones(481)/481,mode='same');uneven=drums*env
    m=np.mean(mix,axis=0);s=(mix[0]-mix[1])*.5*2.5
    active=np.clip((t-.5)/.02,0,1)*np.clip((18.5-t)/.02,0,1)
    s=s+.3*np.sin(2*np.pi*80*t)*active;stereo=np.stack([m+s,m-s])
    cases=[('01_EQ_nadmiar_basu',mix,bass,'Dodano Bell +10 dB / 120 Hz / Q 0,7. Ręcznie: rzeczywisty Spectral EQ −10 dB w tym paśmie.'),
           ('02_Dynamic_EQ_ostra_gora',mix,harsh,'Dodano Bell +10 dB / 3,5 kHz / Q 2,5. Ręcznie: Dynamic EQ, range −12 dB, threshold −42 dBFS, Q 2,5.'),
           ('03_Dynamics_nierowne_uderzenia',drums,uneven,'Środkowe powtórzenie identycznej frazy 6 s ściszono o około 16 dB, wygładzając przejścia. Miara to różnica RMS między trzema powtórzeniami, a nie naturalne różnice między stopą i werblem. Ręcznie: ratio 6:1, threshold −32 dBFS, attack 0,1 ms, release 60 ms, upward maks. 8 dB; następnie limiter Drive 12 dB / ceiling −6 dBFS kontroluje szczyty.'),
           ('04_Imager_przeciwfaza_basu',mix,stereo,'Zwiększono Side 2,5× i dodano ton 80 Hz w przeciwfazie. Ręcznie: Mono Bass, crossover 500 Hz, Low Width 0, Mid 0,7×, pozostałe 1×. To tłumienie problematycznego Side, nie odzyskiwanie brakującego dźwięku.')]
    binary=args.plugin/('Contents/x86_64-win/JerzyMasterLabX.vst3' if platform.system()=='Windows' else 'Contents/x86_64-linux/JerzyMasterLabX.so')
    report={'platform':platform.system(),'plugin_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'brain_tool_sha256':hashlib.sha256(args.brain_tool.read_bytes()).hexdigest(),'source_url':SOURCE_URL,'source_sha256':SOURCE_SHA256,'license':LICENSE_URL,'brain_implementation':'Same actual Processor/MasterBrain C++ linked from plugin library; proposal imported unmodified into real VST3','target_lufs_match':-28,'examples':[]}
    lines=['# Demonstracja rzeczywistego działania DSP i Brain','',
           'W każdym folderze: 00_WZOR_MATCH, 01_PRZED_Z_PROBLEMEM_MATCH, 02_PO_BRAIN_MATCH, 03_PO_RECZNIE_MATCH. Wszystkie WAV stereo 24-bit / 48 kHz i −28 LUFS-I (tolerancja 0,15 LU). MP3 odtwarza kolejno PRZED → BRAIN → RĘCZNIE, z 0,75 s przerwy. Wzór to ten sam materiał przed dodaniem problemu, nie profesjonalny master.',
           '', 'To kontrolowana, celowo wyrazista demonstracja, nie test ślepy ani dowód przewagi nad innymi procesorami. Mocne ręczne nastawy służą pokazaniu funkcji i nie są uniwersalnym presetem masteringowym. Brain jest testowany bez wzmacniania lub poprawiania jego propozycji; jego ograniczenia pozostają słyszalne i są zapisane w raporcie.',
           '', 'Rzeczywisty kod C++ Processor/MasterBrain analizuje 16 s, Rock/Transparent, intensity 1,5, cel −16 LUFS, ceiling −1 dBFS, automatyczny wybór rodzin. Kontrola: analiza nie może zmienić parametrów przed Apply. Zapisuje pełny JSON i BRAIN.jmlx. Python przenosi dokładnie te wartości do rzeczywistego VST3 i sprawdza odczyt. To nie jest implementacja reguł Braina w Pythonie. Ręczny wynik używa wskazanego modułu; w demonstracji Dynamics dodano limiter do opanowania szczytów. Pozostałe procesory są poza łańcuchem.',
           '', 'Próbki pochodzą z rzeczywistych stemów The Easton Ellises — Falcon, CC BY-NC-SA 3.0, '+LICENSE_URL+'. Źródło: '+SOURCE_URL+'. Użycie niekomercyjne, autorstwo i zachowanie licencji. Artyści nie firmują wtyczki. Fragment 6 s powtórzono trzy razy; fade 5 ms, po 0,5 s ciszy na brzegach. Źródło AAC 44,1 kHz; WAV 48 kHz nie przywraca danych. Zmiany: wycinek/stemy, pętla, fade/resampling/gain, celowe zaburzenie oraz DSP MasterLab X.',
           '', 'DANE_TECHNICZNE zawiera bezpośrednie WAV PRZED, PO_BRAIN i PO_RECZNIE (bez końcowej normalizacji), wejście analizy i parametry. Ceiling limitera jest sample-peak, nie True Peak; wersje MATCH po dodatkowym gainie nie mają gwarancji ceiling modułu. Nie wykonano testu FL Studio ani oceny odsłuchowej.',
           '', '## Przypadki i pomiary']
    # Verify host compensation with an all-bypassed real VST3 before comparisons.
    dry=plugin(args.plugin);dry(np.zeros((2,RATE),np.float32),RATE);impulse=np.zeros((2,RATE),np.float32);impulse[:,12000]=.1;imp=dry(impulse,RATE)
    assert np.argmax(imp[0])==12000 and np.max(np.abs(imp-impulse))<1e-6,'PDC error'
    for index,(name,healthy,damaged,description) in enumerate(cases):
        folder=args.output/name;data=folder/'DANE_TECHNICZNE';data.mkdir(parents=True,exist_ok=True)
        scale=10**(-6/20)/max(float(np.max(np.abs(healthy))),float(np.max(np.abs(damaged))))
        healthy=(healthy*scale).astype(np.float32);damaged=(damaged*scale).astype(np.float32)
        input_file=data/'PRZED.wav';write(input_file,damaged);damaged=read_pcm24(input_file)
        result=subprocess.run([str(args.brain_tool.resolve()),str(input_file.resolve()),str(data.resolve())],capture_output=True,text=True)
        if result.returncode:raise RuntimeError('Actual Brain tool failed: '+result.stdout+' '+result.stderr)
        brain=json.loads((data/'brain-proposal.json').read_text());assert len(brain['parameters'])==161
        p=load_plugin(str(plugin_location(args.plugin)))
        for setting in brain['parameters']:
            key=normalize_python_parameter_name(setting['name']);raw(p,key,setting['normalized'])
            assert abs(p.parameters[key].raw_value-setting['normalized'])<2e-6,'Brain parameter import mismatch'
        after_brain,brain_tests=render(p,damaged)
        manual_plugin=manual(args.plugin,index);after_manual,manual_tests=render(manual_plugin,damaged)
        assert np.max(np.abs(after_manual-damaged))>.01,'Manual processor has no effect'
        write(data/'PO_BRAIN.wav',after_brain);write(data/'PO_RECZNIE.wav',after_manual)
        manual_values={k:float(v.raw_value) for k,v in manual_plugin.parameters.items()};(data/'manual-parameters.json').write_text(json.dumps(manual_values,indent=2)+'\n')
        arrays=[healthy,damaged,after_brain,after_manual];versions=['00_WZOR_MATCH','01_PRZED_Z_PROBLEMEM_MATCH','02_PO_BRAIN_MATCH','03_PO_RECZNIE_MATCH'];files=[];matched=[]
        for version,a in zip(versions,arrays):
            temporary=data/(version+'_pomiar.wav');write(temporary,a);measure_raw=measure(temporary);gain=-28-measure_raw['input_i'];a=a*10**(gain/20);assert np.max(np.abs(a))<1,'LUFS match clipping'
            out=folder/(version+'.wav');write(out,a);metrics=measure(out);assert abs(metrics['input_i']+28)<=.15
            temporary.unlink();actual=read_pcm24(out);matched.append(actual)
            files.append({'file':out.name,'sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'match_gain_db':gain,'measurements':metrics,'features':features(actual)})
        f=[x['features'] for x in files];metric=['bass_to_mid_db','harsh_to_mid_db','phrase_rms_range_db','low_side_to_mid_db'][index]
        manual_improvement=abs(f[1][metric]-f[0][metric])-abs(f[3][metric]-f[0][metric]);brain_improvement=abs(f[1][metric]-f[0][metric])-abs(f[2][metric]-f[0][metric])
        # Only the targeted manual demo has an expected correction; never demand
        # that Brain "wins" or tune its output to satisfy a test.
        assert manual_improvement>1,(name,'No measured manual correction',manual_improvement)
        preview=np.concatenate([matched[1],np.zeros((2,int(RATE*.75)),np.float32),matched[2],np.zeros((2,int(RATE*.75)),np.float32),matched[3]],axis=1)
        subprocess.run([ffmpeg(),'-v','error','-y','-f','f32le','-ar',str(RATE),'-ac','2','-i','pipe:0','-c:a','libmp3lame','-b:a','192k',str(folder/'ODSLUCH_PRZED_BRAIN_RECZNIE.mp3')],input=preview.T.astype('<f4').tobytes(),capture_output=True,check=True)
        report['examples'].append({'name':name,'problem':description,'metric':metric,'manual_reduction_of_error_db':manual_improvement,'brain_reduction_of_error_db':brain_improvement,'files':files,'brain_tests':brain_tests,'manual_tests':manual_tests,'helper_output':result.stdout,'normalized_manual_parameters':manual_values})
        lines+=['',f'### {name}',description,'',f'Miara: `{metric}`. Wzór {f[0][metric]:.2f}; problem {f[1][metric]:.2f}; Brain {f[2][metric]:.2f}; ręcznie {f[3][metric]:.2f} dB.',f'Zmniejszenie odchylenia od wzoru w tej mierze: Brain {brain_improvement:.2f} dB; ręcznie {manual_improvement:.2f} dB. Wartość ujemna oznacza pogorszenie tej miary. To nie zastępuje odsłuchu.']
        print(f'PASS {name}: manual correction {manual_improvement:.2f} dB; actual Brain {brain_improvement:.2f} dB; raw repeats and buffer invariance passed',flush=True)
    (args.output/'measurements.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    (args.output/'CZYTAJ_PRZED_ODSLUCHEM.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    (args.output/'LICENCJA_NAGRAN.txt').write_text('The Easton Ellises — Falcon.\n'+SOURCE_URL+'\nCC BY-NC-SA 3.0: '+LICENSE_URL+'\nPrzetworzenie: wybór stemów, wycinek, pętla, gain, resampling, celowe zaburzenia oraz DSP MasterLab X. Wyłącznie użycie niekomercyjne, uznanie autorstwa i ta sama licencja. Artyści nie firmują wtyczki.\n',encoding='utf-8')
    print('PASS: four controlled demonstrations, actual unchanged Brain proposals, actual VST3 processing, measured manual correction, PDC, determinism and loudness match',flush=True)

if __name__=='__main__':
    try:main()
    except Exception:
        import traceback
        print('::error::'+traceback.format_exc().replace('\r','').replace('\n','%0A'),flush=True)
        raise
