#include "BrainRules.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <limits>
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
float value(const masterlab::BrainProposal& p,const std::string& id){for(const auto& change:p.changes)if(change.first==id)return change.second;return -999;}
int main(){
 masterlab::BrainFeatures f;f.rate=48000;f.seconds=8;f.integrated=-22;f.peak=.25f;f.rms=.08f;f.correlation=.5f;f.energy={.04,.11,.6,.2,.05};
 masterlab::BrainConfig c;
 for(int genre=0;genre<9;++genre)for(int style=0;style<4;++style)for(float intensity:{.25f,1.f,1.5f}){
  c.genre=genre;c.style=style;c.intensity=intensity;auto p=masterlab::proposeMastering(f,c);check(p.valid&&p.changes.size()==161,"complete proposal missing");std::set<std::string> ids;
  for(const auto& change:p.changes)check(ids.insert(change.first).second&&std::isfinite(change.second),"duplicate or non-finite proposed setting");
  for(float gain:p.gains)check(std::abs(gain)<=3,"EQ exceeds correction cap");check(p.drive>=0&&p.drive<=12&&value(p,"order")==0,"unsafe drive or chain order");
 }
 c={};c.useLimiter=false;auto eq=masterlab::proposeMastering(f,c);check(eq.valid&&value(eq,"limiterDrive")==-999&&value(eq,"order")==-999,"excluded limiter was changed");
 c.useLimiter=true;c.useEq=false;c.intensity=.25f;auto quiet=masterlab::proposeMastering(f,c);c.intensity=1.5f;auto strong=masterlab::proposeMastering(f,c);check(quiet.drive==strong.drive&&value(strong,"gain")==-999,"intensity scales loudness or excluded EQ changed");
 c.useLimiter=false;c.useStabilizer=c.useStereo=c.useDynamics=c.useDynamicEq=false;auto none=masterlab::proposeMastering(f,c);check(!none.valid&&none.changes.empty(),"empty selection can be applied");
 c={};auto invalid=f;invalid.integrated=-120;check(!masterlab::proposeMastering(invalid,c).valid,"silence creates proposal");invalid=f;invalid.seconds=7;check(!masterlab::proposeMastering(invalid,c).valid,"short capture creates proposal");invalid=f;invalid.energy[0]=std::numeric_limits<double>::quiet_NaN();check(!masterlab::proposeMastering(invalid,c).valid,"NaN creates proposal");
 invalid=f;invalid.integrated=-8;check(masterlab::proposeMastering(invalid,c).drive==0,"loud input creates additional drive");
 invalid=f;invalid.correlation=-1;invalid.clips=100;check(masterlab::proposeMastering(invalid,c).warnings.size()>=4,"phase and clipping warnings missing");
 c.useEq=false;c.existingEqEnabled=false;auto flat=masterlab::proposeMastering(f,c);c.existingEqEnabled=true;c.existingEq[0].gain=6;auto coloured=masterlab::proposeMastering(f,c);check(coloured.eqEstimate>flat.eqEstimate,"preserved EQ omitted from level estimate");
 c={};auto stereo=f;stereo.stereoAnalysed=true;stereo.bandCorrelation={-.5f,.8f,.9f,.7f};auto narrowed=masterlab::proposeMastering(stereo,c);check(value(narrowed,"stereoMonoBass")==1&&value(narrowed,"stereoLowWidth")<1&&value(narrowed,"stereoHighWidth")==1,"Brain ignores per-band stereo correlation");
 c={};auto healthy=f;healthy.stereoAnalysed=true;healthy.bandCorrelation={.8f,.9f,.9f,.9f};auto simple=masterlab::proposeMastering(healthy,c);check(value(simple,"stereoPresent")==0&&value(simple,"dynamicsPresent")==0,"Brain adds unnecessary stereo or compressor");
 auto problematic=healthy;problematic.bandCorrelation[0]=-.3f;problematic.peak=.9f;problematic.rms=.05f;auto complex=masterlab::proposeMastering(problematic,c);check(value(complex,"stereoPresent")==1&&value(complex,"dynamicsPresent")==1,"Brain does not select needed processors");
 c.automaticModules=false;auto forced=masterlab::proposeMastering(healthy,c);for(const char* id:{"eqPresent","limPresent","stereoPresent","dynamicsPresent","dynamicEqPresent"})check(value(forced,id)==1,"manual processor selection not respected");
 // Additional five real post-chain processors are opt-in and part of the
 // actual proposal, not merely additional labels in the UI.
 c={};c.useEq=c.useLimiter=c.useStereo=c.useDynamics=c.useDynamicEq=c.useStabilizer=false;
 c.useExciter=c.useAnalog=c.useBass=c.useTransient=c.useTruePeak=true;
 auto expanded=masterlab::proposeMastering(healthy,c);
 check(expanded.valid,"Brain rejects expanded-only processor selection");
 std::set<std::string> expandedIds;
 for(const auto& entry:expanded.changes)
  check(expandedIds.insert(entry.first).second&&std::isfinite(entry.second),"expanded Brain has duplicate/nonfinite values");
 for(const char* id:{"exciterPresent","analogPresent","bassPresent","transientPresent","truePeakPresent"})
  check(value(expanded,id)==1,"expanded Master Brain did not include selected processor");
 check(value(expanded,"eqPresent")==-999 && value(expanded,"limPresent")==-999,
       "expanded Brain silently enables excluded original modules");
 check(value(expanded,"exciterAmountHigh")>=0&&value(expanded,"exciterAmountHigh")<=1,
       "expanded Brain exciter out of range");
 c.useExciter=c.useAnalog=c.useBass=c.useTransient=c.useTruePeak=false;
 check(!masterlab::proposeMastering(healthy,c).valid,"empty expanded selection should not be applied");
 // Validate tonal estimates against rendered sine energy, independently of magnitude().
 for(double rate:{44100.,48000.,96000.,192000.}){
  c={};c.useLimiter=false;f.rate=rate;const auto proposal=masterlab::proposeMastering(f,c);
  double inputSum=0,targetSum=0,estimateSum=0;for(size_t i=0;i<5;++i){inputSum+=proposal.shares[i];targetSum+=proposal.targetShares[i];estimateSum+=proposal.estimatedShares[i];}
  check(std::abs(inputSum-1)<1e-6&&std::abs(targetSum-1)<1e-6&&std::abs(estimateSum-1)<1e-6,"tonal energy shares are not normalized");
  std::array<double,5> energies{};double renderedSum=0;
  constexpr std::array<double,5> centres{40,140,700,3500,11000};
  for(size_t region=0;region<5;++region){
   std::array<masterlab::Biquad,5> filters;
   for(size_t band=0;band<5;++band){const auto suffix=std::to_string(band+1),parameterSuffix=band? suffix:std::string{};
    filters[band].configure(rate,value(proposal,"frequency"+parameterSuffix),value(proposal,"gain"+parameterSuffix),value(proposal,"q"+parameterSuffix),static_cast<masterlab::FilterType>(static_cast<int>(value(proposal,"bandType"+suffix))));}
   double input=0,output=0;const int samples=static_cast<int>(rate);
   for(int i=0;i<samples*2;++i){const float signal=static_cast<float>(.1*std::sin(6.283185307179586*centres[region]*i/rate));float processed=signal;
    for(size_t band=0;band<5;++band)if(value(proposal,"bandEnabled"+std::to_string(band+1))>0)processed=filters[band].tick(processed);
    if(i>=samples){input+=signal*signal;output+=processed*processed;}}
   energies[region]=f.energy[region]*output/input;renderedSum+=energies[region];
  }
  for(size_t i=0;i<5;++i)check(std::abs(energies[i]/renderedSum-proposal.estimatedShares[i])<.0002,"EQ tonal estimate differs from rendered filter energy");
  c.useEq=false;c.existingEqEnabled=false;const auto unchanged=masterlab::proposeMastering(f,c);for(size_t i=0;i<5;++i)check(std::abs(unchanged.shares[i]-unchanged.estimatedShares[i])<1e-6,"excluded flat EQ changes predicted balance");
 }
 // Detailed rules must distinguish persistent resonances from transient timbre.
 c={};f.rate=48000;f.detail.analysed=true;f.detail.coverage=45;f.detail.power.fill(.001);f.detail.sectionLowDb=-24;f.detail.sectionHighDb=-23;
 auto detailed=masterlab::proposeMastering(f,c);check(value(detailed,"eqPresent")==0&&value(detailed,"dynamicEqPresent")==0&&value(detailed,"dynamicsPresent")==0,"healthy detailed input gets unnecessary processors");
 f.detail.prominence[42]=10;f.detail.persistence[42]=.9f;
 auto resonance=masterlab::proposeMastering(f,c);check(value(resonance,"eqPresent")==1&&value(resonance,"gain")<-3&&std::abs(value(resonance,"frequency")-3200)<1,"persistent harsh resonance not targeted");
 f.detail.persistence[42]=.2f;check(value(masterlab::proposeMastering(f,c),"eqPresent")==0,"short transient mistaken for persistent resonance");
 f.detail.sectionLowDb=-38;f.detail.sectionHighDb=-22;f.detail.sectionSpreadDb=16;f.detail.sectionJumpDb=15;
 auto uneven=masterlab::proposeMastering(f,c);check(value(uneven,"dynamicsPresent")==1&&value(uneven,"dynRatio")>=3&&value(uneven,"upwardAmount")>0,"sustained level changes not treated");
 auto bad=f;bad.detail.power[0]=std::numeric_limits<double>::quiet_NaN();check(!masterlab::proposeMastering(bad,c).valid,"invalid detailed spectrum creates proposal");
 masterlab::BrainDetailAnalyzer temporal;temporal.prepare(8000,9);for(int i=0;i<72000;++i){float signal=(i>=24000&&i<48000)?.016f:.1f;temporal.push(signal,signal);}auto levels=temporal.finish();check(levels.sectionSpreadDb>12&&levels.sectionJumpDb>15,"section-level measurement misses a sustained step");temporal.prepare(8000,9);for(int i=0;i<72000;++i)temporal.push(.1f,.1f);check(temporal.finish().sectionSpreadDb<.001,"analyser reset or healthy dynamics incorrect");
 std::set<std::string> parameterIds;for(size_t i=0;i<masterlab::parameterCount;++i)check(parameterIds.insert(masterlab::parameterId(i)).second,"preview parameter mapping duplicates IDs");
 std::cout<<"Passed: 108 profile/style/intensity cases, finite unique settings, caps, excluded modules, silence/short/NaN rejection, existing EQ, clipping and phase warnings\n";
}
