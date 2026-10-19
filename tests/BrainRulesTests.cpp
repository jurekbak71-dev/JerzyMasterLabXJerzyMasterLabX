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
  c.genre=genre;c.style=style;c.intensity=intensity;auto p=masterlab::proposeMastering(f,c);check(p.valid&&p.changes.size()==86,"complete proposal missing");std::set<std::string> ids;
  for(const auto& change:p.changes)check(ids.insert(change.first).second&&std::isfinite(change.second),"duplicate or non-finite proposed setting");
  for(float gain:p.gains)check(std::abs(gain)<=3,"EQ exceeds correction cap");check(p.drive>=0&&p.drive<=12&&value(p,"order")==0,"unsafe drive or chain order");
 }
 c={};c.useLimiter=false;auto eq=masterlab::proposeMastering(f,c);check(eq.valid&&value(eq,"limiterDrive")==-999&&value(eq,"order")==-999,"excluded limiter was changed");
 c.useLimiter=true;c.useEq=false;c.intensity=.25f;auto quiet=masterlab::proposeMastering(f,c);c.intensity=1.5f;auto strong=masterlab::proposeMastering(f,c);check(quiet.drive==strong.drive&&value(strong,"gain")==-999,"intensity scales loudness or excluded EQ changed");
 c.useLimiter=false;auto none=masterlab::proposeMastering(f,c);check(!none.valid&&none.changes.empty(),"empty selection can be applied");
 c={};auto invalid=f;invalid.integrated=-120;check(!masterlab::proposeMastering(invalid,c).valid,"silence creates proposal");invalid=f;invalid.seconds=7;check(!masterlab::proposeMastering(invalid,c).valid,"short capture creates proposal");invalid=f;invalid.energy[0]=std::numeric_limits<double>::quiet_NaN();check(!masterlab::proposeMastering(invalid,c).valid,"NaN creates proposal");
 invalid=f;invalid.integrated=-8;check(masterlab::proposeMastering(invalid,c).drive==0,"loud input creates additional drive");
 invalid=f;invalid.correlation=-1;invalid.clips=100;check(masterlab::proposeMastering(invalid,c).warnings.size()>=4,"phase and clipping warnings missing");
 c.useEq=false;c.existingEqEnabled=false;auto flat=masterlab::proposeMastering(f,c);c.existingEqEnabled=true;c.existingEq[0].gain=6;auto coloured=masterlab::proposeMastering(f,c);check(coloured.eqEstimate>flat.eqEstimate,"preserved EQ omitted from level estimate");
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
 std::cout<<"Passed: 108 profile/style/intensity cases, finite unique settings, caps, excluded modules, silence/short/NaN rejection, existing EQ, clipping and phase warnings\n";
}
