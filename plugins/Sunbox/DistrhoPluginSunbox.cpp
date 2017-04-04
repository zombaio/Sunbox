/*
 * DISTRHO Sunbox Plugin
 * Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <dlfcn.h>

#include "DistrhoPluginSunbox.hpp"

static const char* get_sunvox_libname_()
{
    static const char* const kLocalFilename = "./sunvox.so";

    Dl_info exeInfo;
    void* localSymbol = (void*)get_sunvox_libname_;

    if (dladdr(localSymbol, &exeInfo) == 0)
        return nullptr;

    const char* filename = exeInfo.dli_fname;
    if (filename == nullptr || filename[0] == '\0')
        return nullptr;

    if (filename[0] == '.')
        return kLocalFilename;

    if (const char* const lastsepstr = strrchr(filename, '/'))
    {
        const ssize_t diff = lastsepstr - filename + 1;

        if (char* const rfilename = (char*)malloc(diff+10))
        {
            memcpy(rfilename, filename, diff);
            memcpy(rfilename+diff, kLocalFilename+2, 10);

            printf("rfilename: %s\n", rfilename);
            return rfilename;
        }
    }

    printf("filename: %s\n", filename);
    return nullptr;
}

static const char* get_sunvox_libname()
{
    static const char* const ret(get_sunvox_libname_());
    return ret;
}

#define SUNVOX_MAIN
#include "sunvox.h"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

SunvoxLibInstance::SunvoxLibInstance(const double sampleRate)
    : _l1(false),
      _l2(false),
      ok(false),
      cur_frame(0),
      song_frames(0)
{
    if (sv_load_dll() == 0)
    {
        _l1 = true;

        if (sv_init(nullptr, sampleRate, 2, SV_INIT_FLAG_NO_DEBUG_OUTPUT |
                                            SV_INIT_FLAG_USER_AUDIO_CALLBACK |
                                            SV_INIT_FLAG_AUDIO_FLOAT32 |
                                            SV_INIT_FLAG_ONE_THREAD) == 0 || 1)
        {
            _l2 = true;

            if (sv_open_slot(0) == 0)
            {
                sv_volume(0, 256);
                ok = true;
            }
        }
    }
}

SunvoxLibInstance::~SunvoxLibInstance()
{
    if (_l1)
    {
        if (_l2)
        {
            if (ok)
                sv_close_slot(0);

            sv_deinit();
        }

        sv_unload_dll();
    }
}

bool SunvoxLibInstance::loadSong(const char* filename)
{
    printf( "Loading SunVox song from file...\n" );

    if (sv_load(0, filename) != 0)
    {
        song_frames = 0;
        return false;
    }

    sv_play_from_beginning(0);
    sv_set_autostop(0, 0);

    cur_frame = 0;
    song_frames = sv_get_song_length_frames(0);
    return true;
}

void SunvoxLibInstance::processSong(float* buffer, uint32_t frames)
{
    const unsigned int ticks = sv_get_ticks();

    if (cur_frame + frames >= song_frames)
    {
        const uint diff = (song_frames - cur_frame);
        sv_audio_callback(buffer, diff, 0, ticks);

        printf("Reached max frames, diff =: %d\n", diff);

        cur_frame = frames - diff;
        sv_audio_callback(buffer, cur_frame, 0, ticks);
    }
    else
    {
        sv_audio_callback(buffer, frames, 0, ticks);
        cur_frame += frames;
    }
}

// -----------------------------------------------------------------------

DistrhoPluginSunbox::DistrhoPluginSunbox()
    : Plugin(paramCount, 0, 0), // 0 programs, 0 states
      sunbox(getSampleRate()),
      //mod_num(-1),
      old_pos(-1),
      interleavedBuffer(nullptr),
      interleavedBufferSize(0)
{
    if (! sunbox.ok)
    {
        printf("not ok\n");
        return;
    }

#if 0
    sv_lock_slot(0);
    mod_num = sv_new_module(0, "Sampler", "Sampler", 0, 0, 0 );
    sv_unlock_slot(0);

    if (mod_num >= 0)
    {
        sv_lock_slot(0);
        sv_connect_module(0, mod_num, 0);
        sv_unlock_slot(0);

        //Load a sample:
        sv_sampler_load(0, mod_num, "/tmp/flute.xi", -1);
    }
    else
    {
        fprintf(stderr, "mod num failed\n");
        return;
    }
#else
    sunbox.loadSong("/tmp/test.sunvox");
#endif
}

DistrhoPluginSunbox::~DistrhoPluginSunbox()
{
    if (! sunbox.ok)
        return;

    sv_stop(0);
}

// -----------------------------------------------------------------------
// Init

void DistrhoPluginSunbox::initParameter(uint32_t index, Parameter& parameter)
{
}

// -----------------------------------------------------------------------
// Internal data

float DistrhoPluginSunbox::getParameterValue(uint32_t index) const
{
    return 0.0f;
}

void DistrhoPluginSunbox::setParameterValue(uint32_t index, float value)
{
}

// -----------------------------------------------------------------------
// Process

void DistrhoPluginSunbox::activate()
{
    interleavedBufferSize = getBufferSize()*2;

    interleavedBuffer = new float[interleavedBufferSize];
    memset(interleavedBuffer, 0, sizeof(float)*interleavedBufferSize);
}

void DistrhoPluginSunbox::deactivate()
{
    delete[] interleavedBuffer;
    interleavedBuffer = nullptr;
}

void DistrhoPluginSunbox::run(const float**, float** outputs, uint32_t frames)
{
    float* out1 = outputs[0];
    float* out2 = outputs[1];

    if (! sunbox.ok)
    {
        memset(out1, 0, sizeof(float)*frames);
        memset(out2, 0, sizeof(float)*frames);
        return;
    }

    if (interleavedBufferSize < frames*2)
    {
        delete[] interleavedBuffer;
        interleavedBufferSize = frames*2;

        interleavedBuffer = new float[interleavedBufferSize];
        memset(interleavedBuffer, 0, sizeof(float)*interleavedBufferSize);
    }

    sunbox.processSong(interleavedBuffer, frames);

#if 0
    //Print some info:
    int new_pos = (int)( ( (float)cur_frame / (float)song_frames ) * 100 );
    if( old_pos != new_pos )
    {
        printf( "Playing position: %d %%\n", old_pos );
        old_pos = new_pos;
    }
#endif

    float* _buf = interleavedBuffer;

    for (uint i=0; i<frames; ++i)
    {
        out1[i] = *_buf++;
        out2[i] = *_buf++;
    }
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new DistrhoPluginSunbox();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
