/**************************************************************************/
/*  audio_stream_steamaudio.cpp                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

//Above notice retained as this is largely based on AudioStreamPolyphonic

#include "audio_stream_steamaudio.h"
#include "steamaudio_server.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <unistd.h>

Ref<AudioStreamPlayback> AudioStreamSteamAudio::instantiate_playback() {
	Ref<AudioStreamPlaybackSteamAudio> playback;
	playback.instantiate();
	playback->streams.resize(polyphony);
        for (uint32_t i = 0; i < playback->streams.size(); i++) {
            init_effect_steamaudio(*(playback->global_state),playback->streams[i].effect);
        }
	return playback;
}

String AudioStreamSteamAudio::get_stream_name() const {
	return "AudioStreamSteamAudio";
}

bool AudioStreamSteamAudio::is_monophonic() const {
	return true; // This avoids stream players to instantiate more than one of these.
}

void AudioStreamSteamAudio::set_polyphony(int p_voices) {
	ERR_FAIL_COND(p_voices < 0 || p_voices > 128);
	polyphony = p_voices;
}
int AudioStreamSteamAudio::get_polyphony() const {
	return polyphony;
}

void AudioStreamSteamAudio::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_polyphony", "voices"), &AudioStreamSteamAudio::set_polyphony);
	ClassDB::bind_method(D_METHOD("get_polyphony"), &AudioStreamSteamAudio::get_polyphony);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "polyphony", PROPERTY_HINT_RANGE, "1,128,1"), "set_polyphony", "get_polyphony");
}

AudioStreamSteamAudio::AudioStreamSteamAudio() {
}

////////////////////////



void AudioStreamPlaybackSteamAudio::start(double p_from_pos) {
	if (active) {
		stop();
	}

	active = true;
}

void AudioStreamPlaybackSteamAudio::stop() {
	if (!active) {
		return;
	}

	bool locked = false;
	for (Stream &s : streams) {
		if (s.active.is_set()) {
			// Need locking because something may still be mixing.
			locked = true;
			AudioServer::get_singleton()->lock();
		}
		s.active.clear();
		s.finish_request.clear();
		s.stream_playback.unref();
		s.stream.unref();
	}
	if (locked) {
		AudioServer::get_singleton()->unlock();
	}

	active = false;
}

bool AudioStreamPlaybackSteamAudio::is_playing() const {
	return active;
}

int AudioStreamPlaybackSteamAudio::get_loop_count() const {
	return 0;
}

double AudioStreamPlaybackSteamAudio::get_playback_position() const {
	return 0;
}
void AudioStreamPlaybackSteamAudio::seek(double p_time) {
	// Ignored.
}

void AudioStreamPlaybackSteamAudio::tag_used_streams() {
	for (Stream &s : streams) {
		if (s.active.is_set()) {
			s.stream_playback->tag_used_streams();
		}
	}
}

int AudioStreamPlaybackSteamAudio::mix(AudioFrame *p_buffer, float p_rate_scale, int p_frames) {
	if (!active) {
            return 0;
	}
        if (!local_state.source.source_initialized) {
            return 0;
        }

	// Pre-clear buffer.
	for (int i = 0; i < p_frames; i++) {
		p_buffer[i].left = 0.0f;
                p_buffer[i].right = 0.0f;
	}
        SimOutputsSteamAudio * sim_outputs = &(local_state.sim_outputs);

        //If either output is invalid, we'll skip
        bool direct_valid = sim_outputs->direct_valid[get_read_direct_idx(sim_outputs)];
        bool indirect_valid = sim_outputs->indirect_valid[get_read_indirect_idx(sim_outputs)];

        if (!direct_valid || !indirect_valid) {
            return p_frames;
        }
	for (Stream &s : streams) {
		if (!s.active.is_set()) {
			continue;
		}

		float volume_db = s.volume_db; // Copy because it can be overridden at any time.
		float volume = Math::db2linear(volume_db);

		if (s.finish_request.is_set()) {
			if (s.pending_play.is_set()) {
				// Did not get the chance to play, was finalized too soon.
				s.active.clear();
				continue;
			}
		}

		if (s.pending_play.is_set()) {
			s.stream_playback->start(s.play_offset);
			s.pending_play.clear();
		}
                memset(local_state.work_buffer,0,sizeof(AudioFrame)*global_state->buffer_size);
                int mixed = s.stream_playback->mix(local_state.work_buffer, s.pitch_scale, p_frames); 
                if (mixed==p_frames) {
                    spatialize_steamaudio(*global_state, local_state, s.effect);
                }
 
               for (int i = 0; i < p_frames; i++) {
                    p_buffer[i].left += volume*local_state.work_buffer[i].left;
                    p_buffer[i].right += volume*local_state.work_buffer[i].right;

                }
                if (mixed<p_frames) {
                    s.active.clear();
                }
                
		if (s.finish_request.is_set()) {
			s.active.clear();
		}
	}


	return p_frames;
}

AudioStreamPlaybackSteamAudio::ID AudioStreamPlaybackSteamAudio::play_stream(const Ref<AudioStream> &p_stream, float p_from_offset, float p_volume_db, float p_pitch_scale) {
        ERR_FAIL_COND_V(local_state.source.source_initialized==false, INVALID_ID);
	ERR_FAIL_COND_V(p_stream.is_null(), INVALID_ID);
	for (uint32_t i = 0; i < streams.size(); i++) {
		if (!streams[i].active.is_set()) {
			// Can use this stream, as it's not active.
			streams[i].stream = p_stream;
			streams[i].stream_playback = streams[i].stream->instantiate_playback();
			streams[i].play_offset = p_from_offset;
			streams[i].volume_db = p_volume_db;
			streams[i].prev_volume_db = p_volume_db;
			streams[i].pitch_scale = p_pitch_scale;
			streams[i].id = id_counter++;
			streams[i].finish_request.clear();
			streams[i].pending_play.set();
			streams[i].active.set();
			return (ID(i) << INDEX_SHIFT) | ID(streams[i].id);
		}
	}
	return INVALID_ID;
}

AudioStreamPlaybackSteamAudio::Stream *AudioStreamPlaybackSteamAudio::_find_stream(int64_t p_id) {
	uint32_t index = p_id >> INDEX_SHIFT;
	if (index >= streams.size()) {
		return nullptr;
	}
	if (!streams[index].active.is_set()) {
		return nullptr; // Not active, no longer exists.
	}
	int64_t id = p_id & ID_MASK;
	if (streams[index].id != id) {
		return nullptr;
	}
	return &streams[index];
}

void AudioStreamPlaybackSteamAudio::set_stream_volume(ID p_stream_id, float p_volume_db) {
	Stream *s = _find_stream(p_stream_id);
	if (!s) {
		return;
	}
	s->volume_db = p_volume_db;
}

void AudioStreamPlaybackSteamAudio::set_stream_pitch_scale(ID p_stream_id, float p_pitch_scale) {
	Stream *s = _find_stream(p_stream_id);
	if (!s) {
		return;
	}
	s->pitch_scale = p_pitch_scale;
}

bool AudioStreamPlaybackSteamAudio::is_stream_playing(ID p_stream_id) const {
	return const_cast<AudioStreamPlaybackSteamAudio *>(this)->_find_stream(p_stream_id) != nullptr;
}

void AudioStreamPlaybackSteamAudio::stop_stream(ID p_stream_id) {
	Stream *s = _find_stream(p_stream_id);
	if (!s) {
		return;
	}
	s->finish_request.set();
}

bool AudioStreamPlaybackSteamAudio::init_source_steamaudio(AudioStreamPlayerSteamAudio * player) {
    local_state.source.steamaudio_player = player;
    IPLSourceSettings source_settings{};
    source_settings.flags = static_cast<IPLSimulationFlags>(IPL_SIMULATIONFLAGS_DIRECT|IPL_SIMULATIONFLAGS_REFLECTIONS);
    
    IPLerror errorCode = iplSourceCreate(global_state->simulator, &source_settings, &(local_state.source.src));
    if (errorCode) {
        printf("Err code for iplSourceCreate: %d\n", errorCode);
        return false;
    } 
    SteamAudioServer::get_singleton()->add_source(&(local_state));
    local_state.source.source_initialized = true;
    return true;
}

void AudioStreamPlaybackSteamAudio::_bind_methods() {
	ClassDB::bind_method(D_METHOD("play_stream", "stream", "from_offset", "volume_db", "pitch_scale"), &AudioStreamPlaybackSteamAudio::play_stream, DEFVAL(0), DEFVAL(0), DEFVAL(1.0));
	ClassDB::bind_method(D_METHOD("set_stream_volume", "stream", "volume_db"), &AudioStreamPlaybackSteamAudio::set_stream_volume);
	ClassDB::bind_method(D_METHOD("set_stream_pitch_scale", "stream", "pitch_scale"), &AudioStreamPlaybackSteamAudio::set_stream_pitch_scale);
	ClassDB::bind_method(D_METHOD("is_stream_playing", "stream"), &AudioStreamPlaybackSteamAudio::is_stream_playing);
	ClassDB::bind_method(D_METHOD("stop_stream", "stream"), &AudioStreamPlaybackSteamAudio::stop_stream);
	BIND_CONSTANT(INVALID_ID);
}

AudioStreamPlaybackSteamAudio::AudioStreamPlaybackSteamAudio() {
    global_state = SteamAudioServer::get_singleton()->clone_global_state();
    init_local_state_steamaudio(*global_state,local_state);
}

AudioStreamPlaybackSteamAudio::~AudioStreamPlaybackSteamAudio() {
    SteamAudioServer::get_singleton()->remove_source(&(local_state));
    for (uint32_t i = 0; i < streams.size(); i++) {
            deinit_effect_steamaudio(*global_state,streams[i].effect);
    }
    deinit_local_state_steamaudio(*global_state,local_state);
}
