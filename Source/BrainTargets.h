#pragma once
struct BrainTargets {float lufs=-14.f;float ceilingDbtp=-1.f;};
inline BrainTargets deriveBrainTargets(const BrainFeatures& f,const BrainConfig& c){
 constexpr std::array<float,9> genreLufs{-12.f,-11.f,-12.f,-13.f,-10.5f,-10.f,-14.f,-11.f,-15.f};
 constexpr std::array<float,4> styleOffset{0.f,-.5f,1.f,-1.f};
 const float crest=20.f*std::log10(std::max(f.peak,1.e-9f)/std::max(f.rms,1.e-9f));
 float tolerance=crest>16.f?-1.f:(crest<8.f?.75f:0.f);if(f.clips)tolerance-=1.5f;
 BrainTargets out;out.lufs=std::clamp(genreLufs[std::clamp(c.genre,0,8)]+styleOffset[std::clamp(c.style,0,3)]+tolerance,-18.f,-8.f);
 out.ceilingDbtp=c.style==2?-.8f:-1.f;if(f.clips||f.correlation<-.15f)out.ceilingDbtp=-1.2f;return out;
}
