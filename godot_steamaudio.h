/******************************************************************************
MIT License

Copyright (c) 2023 saturnian-tides

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#ifndef GODOT_STEAMAUDIO_H
#define GODOT_STEAMAUDIO_H

#include "core/object/ref_counted.h"
#include "servers/audio/audio_stream.h"
#include "scene/3d/node_3d.h"
#include <phonon.h>

#define MAX_OCCLUSION_NUM_SAMPLES 16
#define MAX_AMBISONICS_ORDER_DEFAULT 2
class AudioStreamPlayerSteamAudio;
class AudioStreamPlaybackSteamAudio;
class AudioStreamSteamAudio;

inline int num_channels_for_order(int order) {
    return ((order+1)*(order+1));
}

inline Vector3 IPLVec3toGDVec3(IPLVector3 vec_in) {
    return Vector3{vec_in.x,vec_in.y,vec_in.z};
}
inline IPLVector3 GDVec3toIPLVec3(Vector3 vec_in) { 
    return IPLVector3{vec_in.x,vec_in.y,vec_in.z};
}

struct SteamAudioSource {
    IPLSource src;
    AudioStreamPlayerSteamAudio * steamaudio_player;
    bool source_initialized = false;
};

struct EffectSteamAudio {
//Effect settings
    IPLBinauralEffectSettings binaural_settings{};
    IPLDirectEffectSettings direct_settings{};
    IPLReflectionEffectSettings refl_settings{};
    IPLPathEffectSettings path_settings{};
    IPLAmbisonicsDecodeEffectSettings ambisonics_dec_settings{};
    IPLAmbisonicsEncodeEffectSettings ambisonics_enc_settings{};

//Effects
    IPLBinauralEffect binaural_effect = nullptr;
    IPLDirectEffect direct_effect = nullptr;
    IPLReflectionEffect refl_effect = nullptr;
    IPLPathEffect path_effect = nullptr;
    IPLAmbisonicsDecodeEffect ambisonics_dec_effect = nullptr;
    IPLAmbisonicsEncodeEffect ambisonics_enc_effect = nullptr;
};

struct DirectOutputsSteamAudio {
     float distance_attenuation = 0.0f;
     IPLCoordinateSpace3 listener_orientation;
     IPLVector3 ambisonics_direction;
     IPLSimulationOutputs direct_sim_outputs{};
};

struct IndirectOutputsSteamAudio {
    IPLSimulationOutputs indirect_sim_outputs{};
};

struct SimOutputsSteamAudio {
    DirectOutputsSteamAudio direct_outputs[2];
    IndirectOutputsSteamAudio indirect_outputs[2];
   
    std::atomic<int> direct_idx = 0;
    std::atomic<int> indirect_idx = 0;
    std::atomic<bool> direct_valid[2] = {false,false};
    std::atomic<bool> indirect_valid[2] = {false,false};
    std::atomic<bool> direct_read_done = false;
    std::atomic<bool> indirect_read_done = false;
    
    bool indirect_sim_started = false;
};
 
inline int get_read_direct_idx(SimOutputsSteamAudio * sim_outputs) {
    return (1-sim_outputs->direct_idx.load());
}

inline int get_read_indirect_idx(SimOutputsSteamAudio * sim_outputs) {
    return (1-sim_outputs->indirect_idx.load());
}

inline int get_write_direct_idx(SimOutputsSteamAudio * sim_outputs) {
    return (sim_outputs->direct_idx.load());
}

inline int get_write_indirect_idx(SimOutputsSteamAudio * sim_outputs) {
    return (sim_outputs->indirect_idx.load());
}

//Should be in SteamAudioServer
struct GlobalStateSteamAudio {
    IPLContext phonon_ctx;
    IPLContextSettings phonon_ctx_settings{};
    IPLAudioSettings audio_settings{};
    IPLHRTF hrtf = nullptr;
    IPLHRTFSettings hrtf_settings{};
    IPLSimulator simulator;
    IPLSimulationSettings sim_settings{};
    IPLScene scene = nullptr;
    IPLSceneSettings scene_settings{};

    unsigned int buffer_size;    
};

struct LocalStateSteamAudio {
    float spatial_blend;
    AudioFrame * work_buffer;    
// Process controls
    bool apply_distance_atten = false;
    bool apply_air_absorption = false;
    bool apply_directivity = false;
    bool apply_occlusion = false;
    bool apply_transmission = false;
    bool apply_reflections = false;
    bool apply_pathing = false;

// Settings
    float setting_occlusion_radius = 1.0f;
    int setting_occlusion_num_samples = 16;

// Sim state
    SimOutputsSteamAudio sim_outputs;
    float distance_attenuation_cache;
    Vector3 ambisonics_direction_cache;
    IPLCoordinateSpace3 source_coordinates_cache;
    SteamAudioSource source;

// Buffers
    IPLAudioBuffer in_buffer;
    IPLAudioBuffer out_buffer;
    IPLAudioBuffer direct_buffer;
    IPLAudioBuffer mono_buffer;
    IPLAudioBuffer ambisonics_buffer;
    IPLAudioBuffer refl_buffer;
    IPLAudioBuffer spat_buffer;
};

int spatialize_steamaudio(GlobalStateSteamAudio& global_state,
                          LocalStateSteamAudio& local_state,
                          EffectSteamAudio& effect);

inline Vector3 IPLVec3toGDVec3(IPLVector3 vec_in);
inline IPLVector3 GDVec3toIPLVec3(Vector3 vec_in);

int init_global_state_steamaudio(GlobalStateSteamAudio& global_state);
int init_local_state_steamaudio(GlobalStateSteamAudio& global_state, LocalStateSteamAudio& local_state);
int init_effect_steamaudio(GlobalStateSteamAudio& global_state, EffectSteamAudio& effect);

int deinit_global_state_steamaudio(GlobalStateSteamAudio& global_state);
int deinit_local_state_steamaudio(GlobalStateSteamAudio& global_state, LocalStateSteamAudio& local_state);
int deinit_effect_steamaudio(GlobalStateSteamAudio& global_state, EffectSteamAudio& effect);
#endif // GODOT_STEAMAUDIO_H
