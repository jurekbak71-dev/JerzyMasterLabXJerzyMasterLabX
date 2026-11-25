#pragma once
#include <array>
#include <complex>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <cstddef>
namespace ml {
inline constexpr std::size_t fftSize=4096, fftBins=fftSize/2+1, fftHop=1024;
using Samples=std::array<float,fftSize>;
using Spectrum=std::array<float,fftBins>;
struct AnalysisFrame {
 std::array<std::array<Samples,2>,3> samples{};
 double sampleRate=44100;
 unsigned long long sourceId=0, endSample=0, epoch=0;
};
// Consumer-side only: FFT never executes on audio callback.
class SpectralAnalysis {
 std::array<std::complex<double>,fftSize> work{};
 std::array<double,fftSize> window{};
 std::array<std::size_t,fftSize> reversed{};
 std::array<std::complex<double>,fftSize/2> roots{};
 double windowSum=0;
public:
 SpectralAnalysis(){
  for(std::size_t i=0;i<fftSize;++i){
   window[i]=.5-.5*std::cos(2*std::numbers::pi*double(i)/double(fftSize));windowSum+=window[i];
   std::size_t x=i,r=0;for(std::size_t n=fftSize;n>1;n>>=1){r=(r<<1)|(x&1);x>>=1;}reversed[i]=r;
  }
  for(std::size_t k=0;k<roots.size();++k)roots[k]=std::polar(1.,-2*std::numbers::pi*double(k)/double(fftSize));
 }
 Spectrum power(const Samples& input) noexcept {
  for(std::size_t i=0;i<fftSize;++i)work[reversed[i]]={std::isfinite(input[i])?input[i]*window[i]:0.,0.};
  for(std::size_t len=2;len<=fftSize;len*=2){
   const auto stride=fftSize/len;
   for(std::size_t i=0;i<fftSize;i+=len)for(std::size_t j=0;j<len/2;++j){auto u=work[i+j],v=work[i+j+len/2]*roots[j*stride];work[i+j]=u+v;work[i+j+len/2]=u-v;}
  }
  Spectrum out{};for(std::size_t k=0;k<fftBins;++k){double scale=(k==0||k==fftSize/2?1.:2.)/windowSum;out[k]=float(std::norm(work[k])*scale*scale);}return out;
 }
 Spectrum stereoDb(const std::array<Samples,2>& input) noexcept {
  auto l=power(input[0]),r=power(input[1]);
  for(std::size_t k=0;k<fftBins;++k)l[k]=float(10*std::log10(std::max(1e-12,(double(l[k])+r[k])*.5)));
  return l;
 }
};
}
