#include "register_types.h"
#include "summator.h"
#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine.hpp>
#include "audio_stream_steamaudio.h"
#include "audio_stream_player_steamaudio.h"
#include "steamaudio_listener.h"
#include "steamaudio_server.h"
#include "steamaudio_geometry.h"


using namespace godot;

static SteamAudioServer *steamaudio_server = nullptr;

void initialize_summator_types(ModuleInitializationLevel p_level)
{
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
        Engine::get_singleton()->register_singleton(StringName("SteamAudioServer"), SteamAudioServer::get_singleton());
    }
}

void uninitialize_summator_types(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE && p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
        return;
    }
    if (p_level==MODULE_INITIALIZATION_LEVEL_SERVERS) {
        if (steamaudio_server) {
            memdelete(steamaudio_server);
        }
    }
}

extern "C"
{

	// Initialization.

	GDExtensionBool GDE_EXPORT summator_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

		init_obj.register_initializer(initialize_summator_types);
		init_obj.register_terminator(uninitialize_summator_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SERVERS);

		return init_obj.init();
	}
}
