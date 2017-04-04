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

#ifndef DISTRHO_PLUGIN_KARS_HPP_INCLUDED
#define DISTRHO_PLUGIN_KARS_HPP_INCLUDED

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

struct SunvoxLibInstance {
    bool _l1, _l2, ok;

    uint cur_frame;
    uint song_frames;

    SunvoxLibInstance(const double sampleRate);
    ~SunvoxLibInstance();

    bool loadSong(const char* filename);
    void processSong(float* buffer, uint32_t frames);
};

// -----------------------------------------------------------------------

class DistrhoPluginSunbox : public Plugin
{
public:
    enum Parameters
    {
        paramCount = 0
    };

    DistrhoPluginSunbox();
    ~DistrhoPluginSunbox() override;

protected:
    // -------------------------------------------------------------------
    // Information

    const char* getLabel() const noexcept override
    {
        return "Sunbox";
    }

    const char* getDescription() const override
    {
        return "Test plugin";
    }

    const char* getMaker() const noexcept override
    {
        return "falkTX";
    }

    const char* getHomePage() const override
    {
        return "https://github.com/DISTRHO/Sunbox";
    }

    const char* getLicense() const noexcept override
    {
        return "ISC";
    }

    uint32_t getVersion() const noexcept override
    {
        return d_version(1, 0, 0);
    }

    int64_t getUniqueId() const noexcept override
    {
        return d_cconst('D', 'S', 'b', 'x');
    }

    // -------------------------------------------------------------------
    // Init

    void initParameter(uint32_t index, Parameter& parameter) override;

    // -------------------------------------------------------------------
    // Internal data

    float getParameterValue(uint32_t index) const override;
    void  setParameterValue(uint32_t index, float value) override;

    // -------------------------------------------------------------------
    // Process

    void activate() override;
    void deactivate() override;
    void run(const float**, float** outputs, uint32_t frames) override;

    // -------------------------------------------------------------------

private:
    SunvoxLibInstance sunbox;

    //int mod_num;
    int old_pos;
    uint cur_frame, song_frames;

    float* interleavedBuffer;
    uint interleavedBufferSize;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistrhoPluginSunbox)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // DISTRHO_PLUGIN_KARS_HPP_INCLUDED
