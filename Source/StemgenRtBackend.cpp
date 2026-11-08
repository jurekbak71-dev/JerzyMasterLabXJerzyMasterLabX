#include "StemgenRtBackend.h"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <onnxruntime_cxx_api.h>
#include <string_view>
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

namespace masterlab {
namespace {
#if defined(_WIN32)
// The FL Studio process may already load a different onnxruntime.dll from
// System32 (verified in Windows minidumps). Never link to or use that module.
// Load a uniquely named, hash-pinned ORT 1.26 DLL from OUR VST3 directory.
void initializeBundledOrt() {
 static const bool initialized=[]() {
  HMODULE owner=nullptr;
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      reinterpret_cast<LPCWSTR>(&initializeBundledOrt), &owner) || !owner)
   throw std::runtime_error("Cannot locate the MasterLabX module");
  std::wstring filename(32768, L'\\0');
  const DWORD len=GetModuleFileNameW(owner,filename.data(),
                                    static_cast<DWORD>(filename.size()));
  if(len==0 || len>=filename.size())
   throw std::runtime_error("Cannot locate the MasterLabX module path");
  filename.resize(len);
  const auto dir=std::filesystem::path(filename).parent_path();
  const auto dll=(dir/L"JerzyMasterLabX_ort_1_26.dll").wstring();
  HMODULE ort=LoadLibraryExW(dll.c_str(),nullptr,
       LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
  if(!ort)throw std::runtime_error("Cannot load bundled ORT 1.26 DLL (Win32 error "+std::to_string(GetLastError())+")");
  using GetBase=const OrtApiBase* (ORT_API_CALL*)();
  auto getBase=reinterpret_cast<GetBase>(GetProcAddress(ort,"OrtGetApiBase"));
  if(!getBase)throw std::runtime_error("Bundled ONNX Runtime has no OrtGetApiBase export");
  const auto* base=getBase();
  if(!base || !base->GetApi || !base->GetVersionString)
   throw std::runtime_error("Invalid bundled ONNX Runtime API base");
  const char* version=base->GetVersionString();
  if(!version || std::string_view(version)!="1.26.0")
   throw std::runtime_error(std::string("Wrong bundled ONNX Runtime version: ")+(version?version:"<null>"));
  const auto* api=base->GetApi(ORT_API_VERSION);
  if(!api)throw std::runtime_error("Bundled ONNX Runtime does not provide expected API version");
  Ort::InitApi(api); // ORT_API_MANUAL_INIT prevents any implicit System32 import.
  // Intentionally keep the DLL loaded until the host exits: ORT function
  // pointers and sessions must never outlive their owning module.
  return true;
 }();
 (void)initialized;
}
#endif

struct Runner {
 Ort::Session session;
 Ort::MemoryInfo memory{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault)};
 std::vector<std::string> inputNames,outputNames;
 std::vector<const char*> inputPointers,outputPointers;
 std::vector<std::vector<float>> inputData,outputData;
 std::vector<Ort::Value> inputs,outputs;
 Runner(Ort::Env& env,const std::filesystem::path& path,bool parent):session(nullptr){
  Ort::SessionOptions options;options.SetIntraOpNumThreads(1);options.SetInterOpNumThreads(1);
  options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
  options.AddConfigEntry("session.intra_op.allow_spinning","0");
  options.AddConfigEntry("session.inter_op.allow_spinning","0");
  options.AddConfigEntry("mlas.disable_kleidiai","1");
  session=Ort::Session(env,path.c_str(),options);
  Ort::AllocatorWithDefaultOptions allocator;
  const auto count=session.GetInputCount();
  if(count!=session.GetOutputCount()||count<(parent?14u:5u)||count>(parent?14u:6u))
   throw std::runtime_error("StemgenRT tensor count differs");
  for(std::size_t i=0;i<count;++i){
   inputNames.emplace_back(session.GetInputNameAllocated(i,allocator).get());
   outputNames.emplace_back(session.GetOutputNameAllocated(i,allocator).get());
   const auto inputType=session.GetInputTypeInfo(i),outputType=session.GetOutputTypeInfo(i);
   auto inInfo=inputType.GetTensorTypeAndShapeInfo();
   auto outInfo=outputType.GetTensorTypeAndShapeInfo();
   if(inInfo.GetElementType()!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT||outInfo.GetElementType()!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
    throw std::runtime_error("StemgenRT requires float tensors");
   const auto inShape=inInfo.GetShape(),outShape=outInfo.GetShape();
   if(i==0){
    if(inputNames[i]!="audio_chunk"||outputNames[i]!="separated_chunk"||inShape!=std::vector<int64_t>{1,2,128}||
       outShape!=(parent?std::vector<int64_t>{1,4,2,128}:std::vector<int64_t>{1,1,2,128}))
     throw std::runtime_error("StemgenRT audio geometry differs");
   }else if(outShape!=inShape||outputNames[i]!="next_"+inputNames[i])
    throw std::runtime_error("StemgenRT state contract differs");
   auto elements=[](const auto& shape){std::size_t n=1;for(auto x:shape){if(x<=0||x>1000000||n>1000000/static_cast<std::size_t>(x))throw std::runtime_error("Invalid state size");n*=static_cast<std::size_t>(x);}return n;};
   inputData.emplace_back(elements(inShape));outputData.emplace_back(elements(outShape));
   inputs.push_back(Ort::Value::CreateTensor<float>(memory,inputData.back().data(),inputData.back().size(),inShape.data(),inShape.size()));
   outputs.push_back(Ort::Value::CreateTensor<float>(memory,outputData.back().data(),outputData.back().size(),outShape.data(),outShape.size()));
  }
  for(std::size_t i=0;i<count;++i){inputPointers.push_back(inputNames[i].c_str());outputPointers.push_back(outputNames[i].c_str());}
 }
 void setAudio(const LiveStemHop& audio){for(std::size_t c=0;c<2;++c)std::copy(audio[c].begin(),audio[c].end(),inputData[0].begin()+c*128);}
 void run(){
  session.Run(Ort::RunOptions{nullptr},inputPointers.data(),inputs.data(),inputs.size(),outputPointers.data(),outputs.data(),outputs.size());
  for(const auto& data:outputData)for(float x:data)if(!std::isfinite(x))throw std::runtime_error("Nonfinite StemgenRT state/output");
  for(std::size_t i=1;i<inputData.size();++i)std::copy(outputData[i].begin(),outputData[i].end(),inputData[i].begin());
 }
 void reset(){for(auto& x:inputData)std::fill(x.begin(),x.end(),0.f);for(auto& x:outputData)std::fill(x.begin(),x.end(),0.f);}
};
struct Helper {
 Runner& runner;std::mutex mutex;std::condition_variable_any cv;
 bool pending{},done{};std::exception_ptr failure;
 std::jthread thread;
 explicit Helper(Runner& r):runner(r),thread([this](std::stop_token stop){
  std::unique_lock lock(mutex);
  while(cv.wait(lock,stop,[this]{return pending;})){
   pending=false;lock.unlock();std::exception_ptr error;
   try{runner.run();}catch(...){error=std::current_exception();}
   lock.lock();failure=error;done=true;cv.notify_all();
  }
 }){}
 void submit(){std::lock_guard lock(mutex);done=false;failure=nullptr;pending=true;cv.notify_all();}
 std::exception_ptr wait(){std::unique_lock lock(mutex);cv.wait(lock,[this]{return done;});return failure;}
};
}
struct StemgenRtBackend::Impl {
 Ort::Env env{ORT_LOGGING_LEVEL_WARNING,"MasterLabX-StemgenRT"};
 bool parallel,primed{};LiveStemHop previous{};
 std::vector<std::unique_ptr<Runner>> runners;
 // Declared last: helpers stop/join before any runner/session is destroyed.
 std::vector<std::unique_ptr<Helper>> helpers;
 Impl(const std::filesystem::path& folder,bool useParallel):parallel(useParallel){
  if(parallel){
   for(const auto* name:{"drums.onnx","bass.onnx","vocals.onnx"})runners.push_back(std::make_unique<Runner>(env,folder/name,false));
   helpers.push_back(std::make_unique<Helper>(*runners[0]));helpers.push_back(std::make_unique<Helper>(*runners[2]));
  }else runners.push_back(std::make_unique<Runner>(env,folder/"model.onnx",true));
 }
 void reset(){for(auto& r:runners)r->reset();previous={};primed=false;}
 bool process(const LiveStemHop& audio,SeparatedStemHop& output){
  for(const auto& channel:audio)for(float x:channel)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite input audio");
  for(auto& r:runners)r->setAudio(audio);
  if(parallel){
   helpers[0]->submit();helpers[1]->submit();std::exception_ptr failure;
   try{runners[1]->run();}catch(...){failure=std::current_exception();}
   for(auto& h:helpers){auto error=h->wait();if(error)failure=error;}
   if(failure){reset();std::rethrow_exception(failure);}
   for(std::size_t c=0;c<2;++c)for(std::size_t i=0;i<128;++i){
    const auto at=c*128+i;
    output[0][c][i]=runners[0]->outputData[0][at];output[1][c][i]=runners[1]->outputData[0][at];output[3][c][i]=runners[2]->outputData[0][at];
    output[2][c][i]=previous[c][i]-((output[0][c][i]+output[1][c][i])+output[3][c][i]);
   }
  }else{
   try{runners[0]->run();}catch(...){reset();throw;}
   constexpr std::array<std::size_t,4> order{0,1,3,2};
   for(std::size_t s=0;s<4;++s)for(std::size_t c=0;c<2;++c)for(std::size_t i=0;i<128;++i)
    output[s][c][i]=runners[0]->outputData[0][order[s]*256+c*128+i];
  }
  const bool valid=primed;primed=true;previous=audio;return valid;
 }
};
StemgenRtBackend::StemgenRtBackend(const std::filesystem::path& folder,bool parallel)
 :impl_([&]{
#if defined(_WIN32)
   initializeBundledOrt(); // before constructing Ort::Env in Impl
#endif
   return std::make_unique<Impl>(folder,parallel);
 }()){}

StemgenRtBackend::~StemgenRtBackend()=default;
bool StemgenRtBackend::process(const LiveStemHop& input,SeparatedStemHop& output){return impl_->process(input,output);}
void StemgenRtBackend::reset(){impl_->reset();}
}
