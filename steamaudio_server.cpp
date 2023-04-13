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

#include "steamaudio_server.h"
#include "audio_stream_player_steamaudio.h"

void SteamAudioServer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("is_indirect_busy"), &SteamAudioServer::is_indirect_busy);
    ClassDB::bind_method(D_METHOD("tick_indirect"), &SteamAudioServer::tick_indirect);
    ClassDB::bind_method(D_METHOD("tick_direct"), &SteamAudioServer::tick_direct);
}

bool SteamAudioServer::is_indirect_busy() {
    return indirect_thread_processing.load();
}

bool SteamAudioServer::tick_direct() {
    if (listener==nullptr) {
        return false;
    }
    iplSceneCommit(global_state.scene);
    iplSimulatorSetScene(global_state.simulator, global_state.scene);
    iplSimulatorCommit(global_state.simulator);
    Vector3 listener_pos = listener->get_global_transform().origin;
    //https://docs.godotengine.org/en/stable/classes/class_basis.html#class-basis-operator-idx-int
    //Access basis components using their index. b[0] is equivalent to b.x, b[1] is equivalent to b.y, and b[2] is equivalent to b.z.
    Vector3 listener_ahead = -listener->get_global_transform().get_basis().get_column(2); //z
    Vector3 listener_up = listener->get_global_transform().get_basis().get_column(1);     //y
    Vector3 listener_right = listener->get_global_transform().get_basis().get_column(0);  //x
    IPLCoordinateSpace3 listener_coordinates;
    listener_coordinates.ahead = GDVec3toIPLVec3(listener_ahead);
    listener_coordinates.up = GDVec3toIPLVec3(listener_up);
    listener_coordinates.right = GDVec3toIPLVec3(listener_right);
    listener_coordinates.origin = GDVec3toIPLVec3(listener_pos);

    for (LocalStateSteamAudio * local_state : local_states) {
        IPLDistanceAttenuationModel distance_attenuation_model{};
        distance_attenuation_model.type = IPL_DISTANCEATTENUATIONTYPE_DEFAULT;
        Vector3 source_pos = local_state->source.steamaudio_player->get_global_transform().origin;
        float _distance_attenuation = iplDistanceAttenuationCalculate(global_state.phonon_ctx, 
                                                                      GDVec3toIPLVec3(source_pos), 
                                                                      GDVec3toIPLVec3(listener_pos), 
                                                                      &distance_attenuation_model);
        local_state->distance_attenuation_cache = _distance_attenuation;
        local_state->ambisonics_direction_cache = source_pos - listener_pos;
 
        IPLCoordinateSpace3 source_coordinates; // the world-space position and orientation of the source
        source_coordinates.ahead = IPLVector3{0.0f,0.0f,0.0f};
        source_coordinates.up = IPLVector3{0.0f,0.0f,0.0f};
        source_coordinates.right = IPLVector3{0.0f,0.0f,0.0f};
        source_coordinates.origin = GDVec3toIPLVec3(source_pos);

        IPLSimulationInputs inputs{};
        inputs.flags = IPL_SIMULATIONFLAGS_DIRECT;
        inputs.directFlags = static_cast<IPLDirectSimulationFlags>(IPL_DIRECTSIMULATIONFLAGS_OCCLUSION | IPL_DIRECTSIMULATIONFLAGS_TRANSMISSION);
        inputs.source = source_coordinates;
        inputs.occlusionType = IPL_OCCLUSIONTYPE_VOLUMETRIC;
        inputs.occlusionRadius = local_state->setting_occlusion_radius;
        inputs.numOcclusionSamples = local_state->setting_occlusion_num_samples;
        iplSourceSetInputs(local_state->source.src, IPL_SIMULATIONFLAGS_DIRECT, &inputs);

    }


    IPLSimulationSharedInputs shared_inputs{};
    shared_inputs.listener = listener_coordinates;

    iplSimulatorSetSharedInputs(global_state.simulator, IPL_SIMULATIONFLAGS_DIRECT, &shared_inputs);
    iplSimulatorRunDirect(global_state.simulator);

    for (LocalStateSteamAudio * local_state : local_states) {
        //Write outputs
        local_state->sim_outputs_mutex.lock();
            
            local_state->sim_outputs.distance_attenuation = local_state->distance_attenuation_cache;
            local_state->sim_outputs.listener_orientation = listener_coordinates;
            local_state->sim_outputs.listener_orientation.origin = IPLVector3{0.0f,0.0f,0.0f};
            local_state->sim_outputs.ambisonics_direction = GDVec3toIPLVec3(local_state->ambisonics_direction_cache.normalized());
            iplSourceGetOutputs(local_state->source.src, IPL_SIMULATIONFLAGS_DIRECT, &(local_state->sim_outputs.direct_outputs));
            local_state->sim_outputs.direct_valid = true;
        local_state->sim_outputs_mutex.unlock();
    }
    return true; 
}

bool SteamAudioServer::tick_indirect() {

    if (indirect_thread_processing.load())
        return false;
//Should display an error for debugging here
    if (global_state_initialized.load()==false)
        return false;
    {
        std::unique_lock<std::mutex> lock(mtx);
        indirect_thread_processing.store(true);
        cv.notify_one();
        if (listener!=nullptr) {
            Vector3 sample = listener->get_global_transform().origin;
            printf("Listener in steamaudiosrv pos: %f %f %f\n",sample.x,sample.y,sample.z);
        }
    }
    return true;
}

GlobalStateSteamAudio* SteamAudioServer::clone_global_state() {
    //The parameters required to finalize global state are not available when the SteamAudioServer is 
    //initialized, so setup the global state when the first reference to the global state is obtained by
    //a node
    if (global_state_initialized.load()==false) {
        init_global_state_steamaudio(global_state);
        global_state_initialized.store(true);
    }
    return &global_state;
}

void SteamAudioServer::indirect_worker(void *p_udata) {
    SteamAudioServer* srv = (SteamAudioServer *)p_udata;
    uint64_t msdelay = 1000;
    while (srv->running.load()) {
        {
            std::unique_lock<std::mutex> lock(srv->mtx);
            srv->cv.wait(lock, [&]{ return srv->indirect_thread_processing.load() or not srv->running.load(); });
            if (srv->running.load()==false)
                continue;
            OS::get_singleton()->delay_usec(msdelay * 1000);
            srv->indirect_thread_processing.store(false);
        }
    }
}

SteamAudioServer::SteamAudioServer() {
    singleton = this;
}

SteamAudioServer::~SteamAudioServer() {
    SteamAudioServer::finish();
    if (global_state_initialized.load()==true) {
        deinit_global_state_steamaudio(global_state);
    }
}

SteamAudioServer* SteamAudioServer::singleton = nullptr;
 
SteamAudioServer* SteamAudioServer::get_singleton() {
    return singleton;
}

Error SteamAudioServer::init() {
    global_state_initialized.store(false);
    indirect_thread_processing.store(false);
    running.store(true);
    indirect_thread.start(SteamAudioServer::indirect_worker, this);
    return OK;
}

void SteamAudioServer::finish() {
    running.store(false);
    cv.notify_one();
    indirect_thread.wait_to_finish();
    return;
}

bool SteamAudioServer::register_listener(SteamAudioListener * rx) {
    if (rx==nullptr) {
        return false;
    }
    listener = rx;
    return true;
}

bool SteamAudioServer::deregister_listener() {
    listener = nullptr;
    return true;
}

bool SteamAudioServer::add_source(LocalStateSteamAudio * local_state) {
    for (LocalStateSteamAudio * source_state : local_states) {
        if (source_state==local_state) {
            return false;
        }
    }
    iplSourceAdd(local_state->source.src, global_state.simulator);
    local_states.push_back(local_state);
    
    return true;
}

bool SteamAudioServer::remove_source(LocalStateSteamAudio * local_state) {
    if (local_states.has(local_state)) {
        iplSourceRemove(local_state->source.src, global_state.simulator);
        local_states.erase(local_state);
        return true;
    }
    return false;
}
