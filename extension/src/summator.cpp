//© Copyright 2014-2022, Juan Linietsky, Ariel Manzur and the Godot community (CC-BY 3.0)
#include "summator.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

Summator::Summator()
{
    phonon_ctx_settings.version = STEAMAUDIO_VERSION;
    IPLerror errorCode = iplContextCreate(&(phonon_ctx_settings), &(phonon_ctx));
    if (errorCode) {
        printf("Err code for iplContextCreate: %d\n",errorCode);
    }
    //float mix_rate = GLOBAL_GET("audio/driver/mix_rate");
    //int latency = GLOBAL_GET("audio/driver/output_latency"); 
    int buffer_size = 512; //buffer_size = closest_power_of_2(latency * mix_rate / 1000);
    iplAudioBufferAllocate(phonon_ctx, 2, buffer_size, &buf);
    printf("buf address is: %p\n",buf);
    count = 0;

}

Summator::~Summator()
{
}

void Summator::add(int p_value)
{
    count += p_value;
}

void Summator::reset()
{
    count = 0;
}

int Summator::get_total() const
{
    return buf.numChannels;
}

void Summator::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("add", "value"), &Summator::add, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("reset"), &Summator::reset);
    ClassDB::bind_method(D_METHOD("get_total"), &Summator::get_total);
}
