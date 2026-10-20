#!/usr/bin/env python3
"""Untouched input and held-out defect checks using actual C++ Brain + hosted VST3."""
import argparse,json,subprocess
from pathlib import Path
import numpy as np
from pedalboard import load_plugin,PeakFilter
from pedalboard._pedalboard import normalize_python_parameter_name
from render_comparisons import RATE,write,plugin_location,source
from render_genre_tests import download_source,fixture,read_pcm24,LICENSE_URL,SOURCE_URL
from render_demonstration import raw,render

def ratio(a,f,q):
    x=a[:,RATE//2:-RATE//2].astype(np.float64);s=np.fft.rfft(x*np.hanning(x.shape[1]));hz=np.fft.rfftfreq(x.shape[1],1/RATE);power=np.sum(abs(s)**2,axis=0)
    width=np.arcsinh(1/(2*q))/np.log(2);mask=abs(np.log2(np.maximum(hz,1)/f))<width
    return float(10*np.log10((power[mask].sum()+1e-24)/(power[~mask].sum()+1e-24)))
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--plugin',type=Path,required=True);ap.add_argument('--brain-tool',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--cache',type=Path,default=Path('build-audio-source'));args=ap.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    stems=download_source(args.cache);mix,_=fixture(stems[0]);drums,_=fixture(stems[1]);synth=source(0)[:,:mix.shape[1]]
    cases=[('healthy_mix',mix,mix,None),('healthy_drums',drums,drums,None),('healthy_synthesis',synth,synth,None)]
    for f,q,gain in [(2200,1.7,8),(4800,3.2,10),(6500,2,9)]:cases.append((f'held_out_{f}Hz',mix,PeakFilter(cutoff_frequency_hz=f,q=q,gain_db=gain)(mix,RATE),(f,q)))
    report={'method':'Actual unchanged Brain proposal; no reference provided to Brain; new defect frequencies plus untouched inputs. Same recording is not an independent music corpus.','source':SOURCE_URL,'license':LICENSE_URL,'cases':[]}
    for name,reference,a,problem in cases:
        folder=args.output/name;folder.mkdir(exist_ok=True);scale=10**(-6/20)/max(np.max(abs(a)),np.max(abs(reference)));a=(a*scale).astype(np.float32);reference=(reference*scale).astype(np.float32);input_file=folder/'PRZED.wav';write(input_file,a);a=read_pcm24(input_file)
        result=subprocess.run([str(args.brain_tool.resolve()),str(input_file.resolve()),str(folder.resolve())],capture_output=True,text=True,check=True)
        proposal=json.loads((folder/'brain-proposal.json').read_text());assert proposal['preview_verified'],'No actual DSP verification'
        p=load_plugin(str(plugin_location(args.plugin)))
        for setting in proposal['parameters']:raw(p,normalize_python_parameter_name(setting['name']),setting['normalized'])
        after,tests=render(p,a);write(folder/'PO_BRAIN.wav',after)
        values={x['id']:x['raw'] for x in proposal['parameters']};entry={'name':name,'preview_lufs':proposal['preview_lufs'],'preview_passes':proposal['preview_passes'],'eq_present':values['eqPresent'],'dynamics_present':values['dynamicsPresent'],'render_tests':tests}
        if problem:
            f,q=problem;target=ratio(reference,f,q);before=ratio(a,f,q);post=ratio(after,f,q);improvement=abs(before-target)-abs(post-target);entry.update(target_ratio=target,before_ratio=before,after_ratio=post,reduction_of_error_db=improvement)
            assert improvement>1,(name,'Held-out resonance did not improve',improvement)
        else:
            # Untouched music must not receive strong speculative EQ or upward compression.
            cuts=[abs(values['gain' if i==1 else 'gain'+str(i)]) for i in range(1,17) if values['bandEnabled'+str(i)]>.5]
            entry['max_eq_correction_db']=max(cuts,default=0);assert entry['max_eq_correction_db']<=3,(name,'Excessive correction on untouched material',cuts)
            assert values['upwardAmount']==0,(name,'Unnecessary upward compression')
        report['cases'].append(entry);print('PASS',name,entry,flush=True)
    (args.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    (args.output/'CZYTAJ.md').write_text('Testy Brain: trzy wejścia bez celowych wad oraz rezonanse 2,2 / 4,8 / 6,5 kHz. Surowe pliki PRZED/PO nie są wyrównane głośnościowo. Wyniki liczbowe w validation.json. Źródło nagrania: '+SOURCE_URL+'\nCC BY-NC-SA 3.0: '+LICENSE_URL+'\nThe Easton Ellises — Falcon. Zmiany: pętla, resampling/gain, dodane rezonanse oraz DSP. Syntetyczny przykład: własna synteza CC0. Nie jest to niezależny zbiór utworów ani dowód jakości odsłuchowej.\n',encoding='utf-8')
if __name__=='__main__':main()
