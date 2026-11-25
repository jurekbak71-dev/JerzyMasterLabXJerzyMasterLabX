#ifndef MASTERLABPRO_MODULE_ABI_H
#define MASTERLABPRO_MODULE_ABI_H

#include <stddef.h>
#include <stdint.h>

#define MLP_ABI_MAJOR 1u
#define MLP_ABI_MINOR 0u
#define MLP_MAX_PARAMETERS 64u

#if defined(_WIN32)
# define MLP_CALL __cdecl
# if defined(MLP_MODULE_BUILD)
#  define MLP_EXPORT __declspec(dllexport)
# else
#  define MLP_EXPORT
# endif
#else
# define MLP_CALL
# define MLP_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MlpInstance MlpInstance;

typedef enum MlpResult {
  MLP_OK = 0,
  MLP_ERROR_ARGUMENT = 1,
  MLP_ERROR_ABI = 2,
  MLP_ERROR_DESCRIPTOR = 3,
  MLP_ERROR_UNSUPPORTED = 4,
  MLP_ERROR_STATE = 5,
  MLP_ERROR_CAPACITY = 6,
  MLP_ERROR_INTERNAL = 7
} MlpResult;

typedef enum MlpParameterFlags {
  MLP_PARAMETER_AUTOMATABLE = 1u << 0,
  MLP_PARAMETER_INTEGER = 1u << 1,
  MLP_PARAMETER_ENUM = 1u << 2,
  MLP_PARAMETER_LOGARITHMIC = 1u << 3,
  MLP_PARAMETER_AFFECTS_LATENCY = 1u << 4
} MlpParameterFlags;

typedef enum MlpModuleCapabilities {
  MLP_CAP_STEREO = 1ull << 0,
  MLP_CAP_STATE = 1ull << 1,
  MLP_CAP_VARIABLE_LATENCY = 1ull << 2,
  MLP_CAP_NONLINEAR = 1ull << 3
} MlpModuleCapabilities;

typedef struct MlpParameterDescriptor {
  uint32_t struct_size;
  uint32_t id;
  uint32_t flags;
  uint32_t reserved;
  const char* name;
  const char* unit;
  double minimum;
  double maximum;
  double default_value;
  double neutral_value;
  double step;
  double smoothing_ms;
} MlpParameterDescriptor;

typedef struct MlpModuleDescriptor {
  uint32_t struct_size;
  uint32_t abi_major;
  uint32_t abi_minor;
  uint32_t module_version;
  uint64_t type_id_hi;
  uint64_t type_id_lo;
  uint64_t capabilities;
  uint32_t minimum_channels;
  uint32_t maximum_channels;
  uint32_t parameter_count;
  uint32_t reserved;
  const char* name;
  const char* description;
  const char* category;
  const MlpParameterDescriptor* parameters;
} MlpModuleDescriptor;

typedef struct MlpProcessContext {
  uint32_t struct_size;
  uint32_t frames;
  uint32_t channels;
  uint32_t reserved;
  float* const* audio;
} MlpProcessContext;

typedef struct MlpModuleApi {
  uint32_t struct_size;
  uint32_t abi_major;
  uint32_t abi_minor;
  uint32_t reserved;
  const MlpModuleDescriptor* (MLP_CALL *get_descriptor)(void);
  MlpResult (MLP_CALL *create)(MlpInstance** instance);
  void (MLP_CALL *destroy)(MlpInstance* instance);
  MlpResult (MLP_CALL *prepare)(MlpInstance* instance, double sample_rate,
                                uint32_t maximum_frames, uint32_t channels);
  void (MLP_CALL *reset)(MlpInstance* instance);
  MlpResult (MLP_CALL *process)(MlpInstance* instance, const MlpProcessContext* context);
  MlpResult (MLP_CALL *set_parameter)(MlpInstance* instance, uint32_t id, double value);
  MlpResult (MLP_CALL *get_parameter)(const MlpInstance* instance, uint32_t id, double* value);
  uint32_t (MLP_CALL *get_latency_samples)(const MlpInstance* instance);
  MlpResult (MLP_CALL *state_size)(const MlpInstance* instance, uint32_t* bytes);
  MlpResult (MLP_CALL *save_state)(const MlpInstance* instance, void* destination,
                                   uint32_t capacity, uint32_t* written);
  MlpResult (MLP_CALL *load_state)(MlpInstance* instance, const void* source, uint32_t bytes);
} MlpModuleApi;

typedef MlpResult (MLP_CALL *MlpGetModuleApiFn)(uint32_t host_abi_major,
                                                uint32_t host_abi_minor,
                                                MlpModuleApi* api);
MLP_EXPORT MlpResult MLP_CALL masterlabpro_get_module_api(uint32_t host_abi_major,
                                                          uint32_t host_abi_minor,
                                                          MlpModuleApi* api);

#ifdef __cplusplus
}
#endif
#endif
