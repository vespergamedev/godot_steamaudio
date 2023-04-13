/**************************************************************************/
/*  audio_stream_steamaudio.h                                             */
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

#ifndef AUDIO_STREAM_STEAMAUDIO_H
#define AUDIO_STREAM_STEAMAUDIO_H

#include "core/templates/local_vector.h"
#include "servers/audio/audio_stream.h"
#include "godot_steamaudio.h"

class AudioStreamSteamAudio : public AudioStream {
	GDCLASS(AudioStreamSteamAudio, AudioStream)
	int polyphony = 32;

	static void _bind_methods();

public:
	virtual Ref<AudioStreamPlayback> instantiate_playback() override;
	virtual String get_stream_name() const override;
	virtual bool is_monophonic() const override;

	void set_polyphony(int p_voices);
	int get_polyphony() const;

	AudioStreamSteamAudio();
};

class AudioStreamPlaybackSteamAudio : public AudioStreamPlayback {
	GDCLASS(AudioStreamPlaybackSteamAudio, AudioStreamPlayback)
	friend class AudioStreamSteamAudio;

        float mix_rate;
        int latency;
protected:
	enum {
		INTERNAL_BUFFER_LEN = 128,
		ID_MASK = 0xFFFFFFFF,
		INDEX_SHIFT = 32
	};
	struct Stream {
		SafeFlag active;
		SafeFlag pending_play;
		SafeFlag finish_request;
		float play_offset = 0;
		float pitch_scale = 1.0;
		Ref<AudioStream> stream;
		Ref<AudioStreamPlayback> stream_playback;
		float prev_volume_db = 0;
		float volume_db = 0;
		uint32_t id = 0;
                EffectSteamAudio effect;
		Stream() :
				active(false), pending_play(false), finish_request(false) {}
	};

        GlobalStateSteamAudio* global_state;
        LocalStateSteamAudio local_state;
	LocalVector<Stream> streams;
	bool active = false;
	uint32_t id_counter = 1;

	_FORCE_INLINE_ Stream *_find_stream(int64_t p_id);
	static void _bind_methods();

public:
	typedef int64_t ID;
	enum {
		INVALID_ID = -1
	};

	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override; //times it looped

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	virtual void tag_used_streams() override;

	virtual int mix(AudioFrame *p_buffer, float p_rate_scale, int p_frames) override;

	ID play_stream(const Ref<AudioStream> &p_stream, float p_from_offset = 0, float p_volume_db = 0, float p_pitch_scale = 1.0);
	void set_stream_volume(ID p_stream_id, float p_volume_db);
	void set_stream_pitch_scale(ID p_stream_id, float p_pitch_scale);
	bool is_stream_playing(ID p_stream_id) const;
	void stop_stream(ID p_stream_id);

        bool init_source_steamaudio(AudioStreamPlayerSteamAudio * player);


	AudioStreamPlaybackSteamAudio();
        ~AudioStreamPlaybackSteamAudio();
};

#endif // AUDIO_STREAM_STEAMAUDIO_H
