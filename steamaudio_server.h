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

#ifndef STEAMAUDIO_SERVER_H
#define STEAMAUDIO_SERVER_H

#include "core/object/object.h"
#include "core/os/thread.h"
#include "godot_steamaudio.h"
#include "steamaudio_listener.h"
#include <mutex>
#include <atomic>
#include <condition_variable>

class SteamAudioServer : public Object {
    GDCLASS(SteamAudioServer, Object);
    static SteamAudioServer * singleton;
    static void indirect_worker(void *p_udata);
private:
    GlobalStateSteamAudio global_state;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> running;
    std::atomic<bool> indirect_thread_processing;
    Thread indirect_thread;
    std::atomic<bool> global_state_initialized;
    SteamAudioListener * listener = nullptr;
    Vector<LocalStateSteamAudio*> local_states;
//Shared Data: SteamAudio Simulator Inputs
    
protected:
    static void _bind_methods();

public:
    static SteamAudioServer * get_singleton();
    Error init();
    void finish();
    bool is_indirect_busy();
    bool tick_indirect();
    bool tick_direct();
    bool register_listener(SteamAudioListener * rx);
    bool deregister_listener();
    bool add_source(LocalStateSteamAudio * local_state);
    bool remove_source(LocalStateSteamAudio * local_state);
    GlobalStateSteamAudio* clone_global_state();    
    
    SteamAudioServer();
    ~SteamAudioServer();
};

#endif //STEAMAUDIO_SERVER_H
