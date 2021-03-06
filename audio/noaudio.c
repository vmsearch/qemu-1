/*
 * QEMU Timer based audio emulation
 *
 * Copyright (c) 2004-2005 Vassili Karpov (malc)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/host-utils.h"
#include "qemu/module.h"
#include "audio.h"
#include "qemu/timer.h"

#define AUDIO_CAP "noaudio"
#include "audio_int.h"

typedef struct NoVoiceOut {
    HWVoiceOut hw;
    int64_t old_ticks;
} NoVoiceOut;

typedef struct NoVoiceIn {
    HWVoiceIn hw;
    int64_t old_ticks;
} NoVoiceIn;

static size_t no_run_out(HWVoiceOut *hw, size_t live)
{
    NoVoiceOut *no = (NoVoiceOut *) hw;
    size_t decr, samples;
    int64_t now;
    int64_t ticks;
    int64_t bytes;

    now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    ticks = now - no->old_ticks;
    bytes = muldiv64(ticks, hw->info.bytes_per_second, NANOSECONDS_PER_SECOND);
    bytes = MIN(bytes, SIZE_MAX);
    samples = bytes >> hw->info.shift;

    no->old_ticks = now;
    decr = MIN (live, samples);
    hw->rpos = (hw->rpos + decr) % hw->samples;
    return decr;
}

static int no_init_out(HWVoiceOut *hw, struct audsettings *as, void *drv_opaque)
{
    audio_pcm_init_info (&hw->info, as);
    hw->samples = 1024;
    return 0;
}

static void no_fini_out (HWVoiceOut *hw)
{
    (void) hw;
}

static int no_ctl_out (HWVoiceOut *hw, int cmd, ...)
{
    (void) hw;
    (void) cmd;
    return 0;
}

static int no_init_in(HWVoiceIn *hw, struct audsettings *as, void *drv_opaque)
{
    audio_pcm_init_info (&hw->info, as);
    hw->samples = 1024;
    return 0;
}

static void no_fini_in (HWVoiceIn *hw)
{
    (void) hw;
}

static size_t no_run_in(HWVoiceIn *hw)
{
    NoVoiceIn *no = (NoVoiceIn *) hw;
    size_t live = audio_pcm_hw_get_live_in(hw);
    size_t dead = hw->samples - live;
    size_t samples = 0;

    if (dead) {
        int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
        int64_t ticks = now - no->old_ticks;
        int64_t bytes =
            muldiv64(ticks, hw->info.bytes_per_second, NANOSECONDS_PER_SECOND);

        no->old_ticks = now;
        bytes = MIN (bytes, SIZE_MAX);
        samples = bytes >> hw->info.shift;
        samples = MIN (samples, dead);
    }
    return samples;
}

static int no_ctl_in (HWVoiceIn *hw, int cmd, ...)
{
    (void) hw;
    (void) cmd;
    return 0;
}

static void *no_audio_init(Audiodev *dev)
{
    return &no_audio_init;
}

static void no_audio_fini (void *opaque)
{
    (void) opaque;
}

static struct audio_pcm_ops no_pcm_ops = {
    .init_out = no_init_out,
    .fini_out = no_fini_out,
    .run_out  = no_run_out,
    .ctl_out  = no_ctl_out,

    .init_in  = no_init_in,
    .fini_in  = no_fini_in,
    .run_in   = no_run_in,
    .ctl_in   = no_ctl_in
};

static struct audio_driver no_audio_driver = {
    .name           = "none",
    .descr          = "Timer based audio emulation",
    .init           = no_audio_init,
    .fini           = no_audio_fini,
    .pcm_ops        = &no_pcm_ops,
    .can_be_default = 1,
    .max_voices_out = INT_MAX,
    .max_voices_in  = INT_MAX,
    .voice_size_out = sizeof (NoVoiceOut),
    .voice_size_in  = sizeof (NoVoiceIn)
};

static void register_audio_none(void)
{
    audio_driver_register(&no_audio_driver);
}
type_init(register_audio_none);
