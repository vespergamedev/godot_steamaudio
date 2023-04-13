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

/* register_types.cpp */

#include "register_types.h"
#include "core/object/class_db.h"
#include "audio_stream_steamaudio.h"
#include "audio_stream_player_steamaudio.h"
#include "steamaudio_listener.h"
#include "steamaudio_server.h"
#include "steamaudio_geometry.h"

static SteamAudioServer *steamaudio_server = nullptr;

void initialize_godot_steamaudio_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE && p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
        return;
    }

    if (p_level==MODULE_INITIALIZATION_LEVEL_SCENE) {
        ClassDB::register_class<AudioStreamSteamAudio>();
        ClassDB::register_class<AudioStreamPlaybackSteamAudio>();
        ClassDB::register_class<AudioStreamPlayerSteamAudio>();
        ClassDB::register_class<SteamAudioListener>();
        ClassDB::register_class<SteamAudioGeometry>();
    }

    if (p_level==MODULE_INITIALIZATION_LEVEL_SERVERS) {
        steamaudio_server = memnew(SteamAudioServer);
        steamaudio_server->init();
        GDREGISTER_CLASS(SteamAudioServer);
        Engine::get_singleton()->add_singleton(Engine::Singleton("SteamAudioServer", SteamAudioServer::get_singleton()));
    }
}

void uninitialize_godot_steamaudio_module(ModuleInitializationLevel p_level){
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE && p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
        return;
    }
    if (p_level==MODULE_INITIALIZATION_LEVEL_SERVERS) {
        if (steamaudio_server) {
            memdelete(steamaudio_server);
        }
    }
}
