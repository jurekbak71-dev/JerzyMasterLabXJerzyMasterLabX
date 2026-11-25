#include "RackState.h"
#include "../sdk/masterlabpro_module_abi.h"
#include <bit>
#include <limits>

namespace ml { namespace {
struct Writer{std::vector<uint8_t>& o;bool u32(uint32_t v){if(o.size()>RackStateCodec::maximumBytes-4)return false;for(unsigned i=0;i<4;++i)o.push_back(uint8_t(v>>(8*i)));return true;}bool u64(uint64_t v){if(o.size()>RackStateCodec::maximumBytes-8)return false;for(unsigned i=0;i<8;++i)o.push_back(uint8_t(v>>(8*i)));return true;}bool bytes(const std::vector<uint8_t>& b){if(b.size()>RackStateCodec::maximumBytes-o.size())return false;o.insert(o.end(),b.begin(),b.end());return true;}};
struct Reader{const uint8_t* p;std::size_t n,pos=0;bool u32(uint32_t& v){if(n-pos<4)return false;v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(p[pos++])<<(8*i);return true;}bool u64(uint64_t& v){if(n-pos<8)return false;v=0;for(unsigned i=0;i<8;++i)v|=uint64_t(p[pos++])<<(8*i);return true;}bool bytes(std::vector<uint8_t>& b,uint32_t count){if(count>RackStateCodec::maximumOpaque||n-pos<count)return false;b.assign(p+pos,p+pos+count);pos+=count;return true;}};
}
bool RackStateCodec::encode(const RackState& s,std::vector<uint8_t>& out)noexcept{
 try{out.clear();Writer w{out};if(s.format!=1||s.modules.size()>10)return false;
  if(!w.u32(0x42504c4d)||!w.u32(s.format)||!w.u32(uint32_t(s.modules.size()))||!w.u32(s.analysisSource)||!w.u32(s.deltaListen?1:0))return false;
  for(const auto& m:s.modules){if(!m.instance||m.lane>=10||m.parameters.size()>MLP_MAX_PARAMETERS||m.opaque.size()>maximumOpaque)return false;
   if(!w.u64(m.instance)||!w.u64(m.typeHi)||!w.u64(m.typeLo)||!w.u32(m.version)||!w.u32(m.lane)||!w.u32(m.bypass?1:0)||!w.u32(uint32_t(m.parameters.size()))||!w.u32(uint32_t(m.opaque.size())))return false;
   for(const auto& p:m.parameters)if(!w.u32(p.id)||!w.u64(std::bit_cast<uint64_t>(p.value)))return false;
   if(!w.bytes(m.opaque))return false;
  }return true;
 }catch(...){out.clear();return false;}
}
bool RackStateCodec::decode(const uint8_t* data,std::size_t bytes,RackState& out)noexcept{
 try{if(!data||bytes>maximumBytes)return false;Reader r{data,bytes};RackState s;uint32_t magic=0,count=0,listen=0;
  if(!r.u32(magic)||magic!=0x42504c4d||!r.u32(s.format)||s.format!=1||!r.u32(count)||count>10||!r.u32(s.analysisSource)||!r.u32(listen)||listen>1)return false;
  s.deltaListen=listen!=0;
  for(uint32_t i=0;i<count;++i){SavedModule m;uint32_t bypass=0,params=0,opaque=0;
   if(!r.u64(m.instance)||!m.instance||!r.u64(m.typeHi)||!r.u64(m.typeLo)||!r.u32(m.version)||!r.u32(m.lane)||m.lane>=10||!r.u32(bypass)||bypass>1||!r.u32(params)||params>MLP_MAX_PARAMETERS||!r.u32(opaque)||opaque>maximumOpaque)return false;
   m.bypass=bypass!=0;
   for(uint32_t p=0;p<params;++p){SavedParameter value;uint64_t bits=0;if(!r.u32(value.id)||!r.u64(bits))return false;value.value=std::bit_cast<double>(bits);m.parameters.push_back(value);}
   if(!r.bytes(m.opaque,opaque))return false;
   s.modules.push_back(std::move(m));
  }
  if(r.pos!=bytes)return false;
  out=std::move(s);return true;
 }catch(...){return false;}
}
}
