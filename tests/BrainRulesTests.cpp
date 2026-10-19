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
 std::cout<<"Passed: 108 profile/style/intensity cases, finite unique settings, caps, excluded modules, silence/short/NaN rejection, existing EQ, clipping and phase warnings\n";
}
