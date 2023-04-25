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

/* godot_steamaudio.cpp */

#include "godot_steamaudio.h"
#include "core/math/math_funcs.h"
#include "core/string/print_string.h"
#include "core/typedefs.h"
#include "core/config/project_settings.h"
#include <stdio.h>

#define N_CHANNELS_INOUT 2
#define N_CHANNELS_MONO 1

int spatialize_steamaudio(GlobalStateSteamAudio& global_state,
                          LocalStateSteamAudio& local_state,
                          EffectSteamAudio& effect) {
    if (local_state.work_buffer==nullptr) {
        return 0;
    }

    SimOutputsSteamAudio * sim_outputs = &(local_state.sim_outputs);

    bool direct_valid = sim_outputs->direct_valid[get_read_direct_idx(sim_outputs)].load();
    bool indirect_valid = sim_outputs->indirect_valid[get_read_indirect_idx(sim_outputs)].load();

    if (!direct_valid || !indirect_valid) {
        return 0;
    }


    iplAudioBufferDeinterleave(global_state.phonon_ctx,(float *)local_state.work_buffer, &(local_state.in_buffer));
    iplAudioBufferDownmix(global_state.phonon_ctx, &(local_state.in_buffer), &(local_state.mono_buffer));
    
    //Apply direct effect
    IPLDirectEffectParams direct_effect_params = sim_outputs->direct_outputs[get_read_direct_idx(sim_outputs)].direct_sim_outputs.direct;

    direct_effect_params.flags = static_cast<IPLDirectEffectFlags>(direct_effect_params.flags | IPL_DIRECTEFFECTFLAGS_APPLYDISTANCEATTENUATION);
    direct_effect_params.flags = static_cast<IPLDirectEffectFlags>(direct_effect_params.flags | IPL_DIRECTEFFECTFLAGS_APPLYOCCLUSION);
    direct_effect_params.flags = static_cast<IPLDirectEffectFlags>(direct_effect_params.flags | IPL_DIRECTEFFECTFLAGS_APPLYTRANSMISSION);

    direct_effect_params.distanceAttenuation = sim_outputs->direct_outputs[get_read_direct_idx(sim_outputs)].distance_attenuation;
    iplDirectEffectApply(effect.direct_effect, &direct_effect_params, &(local_state.in_buffer), &(local_state.direct_buffer));

    //Apply binaural effect
    IPLAmbisonicsEncodeEffectParams ambisonics_enc_effect_params{};
    ambisonics_enc_effect_params.order = global_state.sim_settings.maxOrder;
    ambisonics_enc_effect_params.direction = sim_outputs->direct_outputs[get_read_direct_idx(sim_outputs)].ambisonics_direction;
    iplAmbisonicsEncodeEffectApply(effect.ambisonics_enc_effect, &ambisonics_enc_effect_params, &(local_state.direct_buffer), &(local_state.ambisonics_buffer));

    IPLAmbisonicsDecodeEffectParams ambisonics_dec_effect_params{};
    ambisonics_dec_effect_params.order = global_state.sim_settings.maxOrder;
    ambisonics_dec_effect_params.hrtf = global_state.hrtf;
    ambisonics_dec_effect_params.orientation = sim_outputs->direct_outputs[get_read_direct_idx(sim_outputs)].listener_orientation;
    ambisonics_dec_effect_params.binaural = IPL_TRUE;
    iplAmbisonicsDecodeEffectApply(effect.ambisonics_dec_effect, &ambisonics_dec_effect_params, &(local_state.ambisonics_buffer), &(local_state.out_buffer)); 

    //Apply reflections and/or pathing

    //Mix

    iplAudioBufferInterleave(global_state.phonon_ctx, &(local_state.out_buffer), (float *)local_state.work_buffer);

    sim_outputs->direct_read_done.store(true);
    sim_outputs->indirect_read_done.store(true);

    return 0;
}

int init_global_state_steamaudio(GlobalStateSteamAudio& global_state) {
    global_state.phonon_ctx_settings.version = STEAMAUDIO_VERSION;
    global_state.phonon_ctx = nullptr;
    IPLerror error_code = iplContextCreate(&(global_state.phonon_ctx_settings), &(global_state.phonon_ctx));
    if (error_code) {
        printf("Err code for iplContextCreate: %d\n",error_code);
        return (int)error_code;
    }
    float mix_rate = GLOBAL_GET("audio/driver/mix_rate");
    int latency = GLOBAL_GET("audio/driver/output_latency"); 
    global_state.buffer_size = closest_power_of_2(latency * mix_rate / 1000);
    printf("mix_rate %f latency %d buffer_size %u\n", mix_rate, latency, global_state.buffer_size);   

    global_state.audio_settings.samplingRate = mix_rate;
    global_state.audio_settings.frameSize = global_state.buffer_size;
    global_state.sim_settings.maxOrder = MAX_AMBISONICS_ORDER_DEFAULT;

    global_state.hrtf_settings.type = IPL_HRTFTYPE_DEFAULT;

    error_code = iplHRTFCreate(global_state.phonon_ctx, &(global_state.audio_settings), &(global_state.hrtf_settings), &(global_state.hrtf));
    if (error_code) {
        printf("Err code for iplHRTFCreate: %d\n",error_code);
        return (int)error_code;
    }

    global_state.scene_settings.type = IPL_SCENETYPE_DEFAULT;
    global_state.scene = nullptr;
    error_code = iplSceneCreate(global_state.phonon_ctx, &(global_state.scene_settings), &(global_state.scene));
    if (error_code) {
        printf("Err code for iplSceneCreate: %d\n", error_code);
        return (int)error_code;
    }

    global_state.sim_settings.flags = IPL_SIMULATIONFLAGS_DIRECT;
    global_state.sim_settings.sceneType = IPL_SCENETYPE_DEFAULT;
    global_state.sim_settings.maxNumOcclusionSamples = MAX_OCCLUSION_NUM_SAMPLES;
    global_state.sim_settings.frameSize = global_state.buffer_size;
    global_state.simulator = nullptr;
    error_code = iplSimulatorCreate(global_state.phonon_ctx, &(global_state.sim_settings), &(global_state.simulator));
    if (error_code) {
        printf("Err code for iplSimulatorCreate: %d\n", error_code);
        return (int)error_code;
    }

    iplSimulatorSetScene(global_state.simulator, global_state.scene);
 

    return 0;
}

int init_local_state_steamaudio(GlobalStateSteamAudio& global_state, LocalStateSteamAudio& local_state) {
    local_state.spatial_blend = 1.0f;
    local_state.sim_outputs.direct_valid[0].store(false);
    local_state.sim_outputs.direct_valid[1].store(false);

    local_state.sim_outputs.indirect_valid[0].store(false);
    local_state.sim_outputs.indirect_valid[1].store(false);
    local_state.work_buffer = (AudioFrame *)memalloc(sizeof(AudioFrame)*global_state.buffer_size);
    if (local_state.work_buffer == nullptr) {
        printf("Failed to alloc mem for work buffer\n");
        return -1;
    }
    IPLerror error_code = iplAudioBufferAllocate(global_state.phonon_ctx, N_CHANNELS_INOUT, global_state.buffer_size, &(local_state.in_buffer));
    if (error_code) {
        printf("Err code for iplAudioBufferAllocate: %d\n", error_code);
        printf("Error allocating %d channels %d frames for in_buffer\n",N_CHANNELS_INOUT,global_state.buffer_size);
        return (int)error_code;
    }
    error_code = iplAudioBufferAllocate(global_state.phonon_ctx, N_CHANNELS_INOUT, global_state.buffer_size, &(local_state.out_buffer));
    if (error_code) {
        printf("Err code for iplAudioBufferAllocate: %d\n", error_code);
        printf("Error allocating %d channels %d frames for out_buffer\n",N_CHANNELS_INOUT,global_state.buffer_size);
        return (int)error_code;
    }
    error_code = iplAudioBufferAllocate(global_state.phonon_ctx, N_CHANNELS_MONO, global_state.buffer_size, &(local_state.direct_buffer));
    if (error_code) {
        printf("Err code for iplAudioBufferAllocate: %d\n", error_code);
        printf("Error allocating %d channels %d frames for direct_buffer\n",N_CHANNELS_MONO,global_state.buffer_size);
        return (int)error_code;
    }
    error_code = iplAudioBufferAllocate(global_state.phonon_ctx, N_CHANNELS_MONO, global_state.buffer_size, &(local_state.mono_buffer));
    if (error_code) {
        printf("Err code for iplAudioBufferAllocate: %d\n", error_code);
        printf("Error allocating %d channels %d frames for mono_buffer\n",N_CHANNELS_MONO,global_state.buffer_size);
        return (int)error_code;
    }
    error_code = iplAudioBufferAllocate(global_state.phonon_ctx, num_channels_for_order(global_state.sim_settings.maxOrder), global_state.buffer_size, &(local_state.ambisonics_buffer));
    if (error_code) {
        printf("Err code for iplAudioBufferAllocate: %d\n", error_code);
        printf("Error allocating %d channels %d frames for ambisonics_buffer\n",num_channels_for_order(global_state.sim_settings.maxOrder),global_state.buffer_size);
        return (int)error_code;
    }
    error_code = iplAudioBufferAllocate(global_state.phonon_ctx, num_channels_for_order(global_state.sim_settings.maxOrder), global_state.buffer_size, &(local_state.refl_buffer));
    if (error_code) {
        printf("Err code for iplAudioBufferAllocate: %d\n", error_code);
        printf("Error allocating %d channels %d frames for refl_buffer\n",num_channels_for_order(global_state.sim_settings.maxOrder),global_state.buffer_size);
        return (int)error_code;
    }
    error_code = iplAudioBufferAllocate(global_state.phonon_ctx, N_CHANNELS_INOUT, global_state.buffer_size, &(local_state.spat_buffer));
    if (error_code) {
        printf("Err code for iplAudioBufferAllocate: %d\n", error_code);
        printf("Error allocating %d channels %d frames for spat_buffer\n",N_CHANNELS_INOUT,global_state.buffer_size);
        return (int)error_code;
    }

    return 0;
}

int init_effect_steamaudio(GlobalStateSteamAudio& global_state, EffectSteamAudio& effect) {
    effect.binaural_settings.hrtf = global_state.hrtf;
    IPLerror error_code = iplBinauralEffectCreate(global_state.phonon_ctx, &(global_state.audio_settings), &(effect.binaural_settings), &(effect.binaural_effect));
    if (error_code) {
        printf("Err code for iplBinauralEffectCreate: %d\n", error_code);
        return (int)error_code;
    }
    effect.direct_settings.numChannels = N_CHANNELS_MONO;
    error_code = iplDirectEffectCreate(global_state.phonon_ctx, &(global_state.audio_settings), &(effect.direct_settings), &(effect.direct_effect));
    if (error_code) {
        printf("Err code for iplDirectEffectCreate: %d\n", error_code);
        return (int)error_code;
    }

    effect.path_settings.maxOrder = global_state.sim_settings.maxOrder;
    error_code = iplPathEffectCreate(global_state.phonon_ctx, &(global_state.audio_settings), &(effect.path_settings), &(effect.path_effect));
    if (error_code) {
        printf("Err code for iplPathEffectCreate: %d\n", error_code);
        return (int)error_code;
    }

    effect.ambisonics_dec_settings.hrtf = global_state.hrtf;
    effect.ambisonics_dec_settings.maxOrder = global_state.sim_settings.maxOrder;
    IPLSpeakerLayout speaker_layout{};
    speaker_layout.type = IPL_SPEAKERLAYOUTTYPE_STEREO;
    effect.ambisonics_dec_settings.speakerLayout = speaker_layout;
    error_code = iplAmbisonicsDecodeEffectCreate(global_state.phonon_ctx, &(global_state.audio_settings), &(effect.ambisonics_dec_settings), &(effect.ambisonics_dec_effect));
    if (error_code) {
        printf("Err code for iplAmbisonicsDecodeEffectCreate: %d\n", error_code);
        return (int)error_code;
    }

    effect.ambisonics_enc_settings.maxOrder = global_state.sim_settings.maxOrder;
    error_code = iplAmbisonicsEncodeEffectCreate(global_state.phonon_ctx, &(global_state.audio_settings), &(effect.ambisonics_enc_settings), &(effect.ambisonics_enc_effect));
    if (error_code) {
        printf("Err code for iplAmbisonicsEncodeEffectCreate: %d\n", error_code);
        return (int)error_code;
    }

    return 0;
}

int deinit_global_state_steamaudio(GlobalStateSteamAudio& global_state) { 
    iplSimulatorRelease(&(global_state.simulator));
    iplSceneRelease(&(global_state.scene));
    iplHRTFRelease(&(global_state.hrtf));
    iplContextRelease(&(global_state.phonon_ctx));
    return 0;
}

int deinit_local_state_steamaudio(GlobalStateSteamAudio& global_state, LocalStateSteamAudio& local_state) { 
    iplAudioBufferFree(global_state.phonon_ctx, &(local_state.in_buffer));
    iplAudioBufferFree(global_state.phonon_ctx, &(local_state.out_buffer));
    iplAudioBufferFree(global_state.phonon_ctx, &(local_state.direct_buffer));
    iplAudioBufferFree(global_state.phonon_ctx, &(local_state.mono_buffer));
    iplAudioBufferFree(global_state.phonon_ctx, &(local_state.ambisonics_buffer));
    iplAudioBufferFree(global_state.phonon_ctx, &(local_state.refl_buffer));
    iplAudioBufferFree(global_state.phonon_ctx, &(local_state.spat_buffer));
    if (local_state.work_buffer!=nullptr)
        memfree(local_state.work_buffer);
    return 0;
}

int deinit_effect_steamaudio(GlobalStateSteamAudio& global_state, EffectSteamAudio& effect) {
    iplBinauralEffectRelease(&(effect.binaural_effect));
    iplDirectEffectRelease(&(effect.direct_effect));
    iplPathEffectRelease(&(effect.path_effect));
    iplAmbisonicsDecodeEffectRelease(&(effect.ambisonics_dec_effect));
    iplAmbisonicsEncodeEffectRelease(&(effect.ambisonics_enc_effect));

    return 0;
}
