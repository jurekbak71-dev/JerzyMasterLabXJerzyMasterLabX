#!/usr/bin/env python3
"""Small educational/noncommercial MUSDB excerpt check, not a full benchmark.

Accepts local official .stem.mp4 excerpts; no dataset/audio is redistributed.
Dependencies: numpy, onnxruntime1.26.0 and ffmpeg. Model state/alignment matches
the pinned parent graph used by the native serial/parallel parity tests.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time
import numpy as np
from fetch_stemgenrt_models import MODELS,verify

def si_sdr(reference,estimate):
    reference=np.asarray(reference,dtype=np.float64)
    estimate=np.asarray(estimate,dtype=np.float64)
    if reference.shape!=estimate.shape or not reference.size or not np.isfinite(reference).all() or not np.isfinite(estimate).all():
        raise ValueError("SI-SDR requires equal finite nonempty arrays")
    reference=reference-reference.mean(axis=0)
    estimate=estimate-estimate.mean(axis=0)
    energy=np.sum(reference*reference)
    if energy<1.e-12:return None
    target=reference*(np.sum(reference*estimate)/energy)
    error=estimate-target
    return float(10*np.log10((np.sum(target*target)+1.e-15)/(np.sum(error*error)+1.e-15)))

def decode(path,stream):
    raw=subprocess.run(['ffmpeg','-v','error','-i',str(path),'-map',f'0:a:{stream}',
                        '-f','f32le','-acodec','pcm_f32le','-ar','44100','-ac','2','-'],
                       check=True,capture_output=True,timeout=60).stdout
    audio=np.frombuffer(raw,dtype='<f4').reshape(-1,2)
    if not len(audio) or not np.isfinite(audio).all():raise ValueError("Invalid decoded input")
    return audio

def render(model,audio):
    import onnxruntime as ort
    if ort.__version__!='1.26.0':raise ValueError("Use qualified ORT version1.26.0")
    options=ort.SessionOptions();options.intra_op_num_threads=1;options.inter_op_num_threads=1
    options.execution_mode=ort.ExecutionMode.ORT_SEQUENTIAL
    for key,value in [('session.intra_op.allow_spinning','0'),('session.inter_op.allow_spinning','0'),('mlas.disable_kleidiai','1')]:options.add_session_config_entry(key,value)
    session=ort.InferenceSession(str(model),options,providers=['CPUExecutionProvider'])
    inputs=session.get_inputs()
    feed={item.name:np.zeros(item.shape,dtype=np.float32) for item in inputs}
    hops=(len(audio)+127)//128
    separated=np.zeros((4,hops*128,2),dtype=np.float32)
    # One extra zero hop flushes the previous-input output at EOF.
    for hop in range(hops+1):
        chunk=np.zeros((1,2,128),dtype=np.float32)
        actual=audio[hop*128:(hop+1)*128]
        if len(actual):chunk[0,:,:len(actual)]=actual.T
        feed['audio_chunk']=chunk
        output=session.run(None,feed)
        if hop:
            separated[:,(hop-1)*128:hop*128,:]=output[0][0,[0,1,3,2],:,:].transpose(0,2,1)
        feed.update({item.name:state for item,state in zip(inputs[1:],output[1:])})
    separated=separated[:,:len(audio),:]
    if not np.isfinite(separated).all():raise ValueError("Nonfinite inference output")
    return separated

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model',type=Path,required=True)
    parser.add_argument('--tracks',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();verify(args.model,MODELS['model.onnx'][1],MODELS['model.onnx'][2])
    if args.out.exists():raise FileExistsError("Preserve old evidence: select a fresh output JSON")
    files=sorted(args.tracks.glob('*.stem.mp4'))
    if not files:raise ValueError("No MUSDB excerpt files")
    report={'kind':'short_MUSDB_test_excerpt_diagnostic_not_full_quality_acceptance',
            'model_sha256':MODELS['model.onnx'][2],'rate':44100,'score':'joint stereo zero-mean SI-SDR',
            'excluded_initial_samples':44100,
            'source':'official MUSDB18-7-STEMS releasev0.4.0, first3alphabetic test tracks',
            'limits':['7-second compressed excerpts,1-second warmup excluded','not museval/BSS_eval published SDR',
                      'no long-song continuity or FL Studio/matched-level listening acceptance','no audio redistributed'],
            'tracks':[]}
    for path in files:
        mixture=decode(path,0)
        sources=[decode(path,i) for i in range(1,5)]
        if any(x.shape!=mixture.shape for x in sources):raise ValueError("Inconsistent reference stem lengths")
        started=time.perf_counter();estimated=render(args.model,mixture);elapsed=time.perf_counter()-started
        if len(mixture)<=44100:raise ValueError("Excerpt too short for warmup exclusion")
        result={'name':path.stem,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
                'samples':len(mixture),'duration_seconds':len(mixture)/44100,'python_parent_wall_seconds':elapsed,
                'sum_reconstruction_peak':float(np.max(np.abs(np.sum(estimated,axis=0)-mixture))),
                'reference_sum_difference_rms':float(np.sqrt(np.mean((np.sum(sources,axis=0)-mixture)**2))),
                'stems':{}}
        for index,name in enumerate(('drums','bass','other','vocals')):
            score=si_sdr(sources[index][44100:],estimated[index,44100:])
            baseline=si_sdr(sources[index][44100:],mixture[44100:])
            result['stems'][name]={'si_sdr_db':score,'mixture_baseline_db':baseline,
                'improvement_db':score-baseline if score is not None and baseline is not None else None}
        report['tracks'].append(result)
        args.out.parent.mkdir(parents=True,exist_ok=True)
        args.out.write_text(json.dumps(report,indent=2,allow_nan=False))
        print(json.dumps(result,allow_nan=False),flush=True)

if __name__=='__main__':main()
