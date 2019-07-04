/*
 *  khean.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <math.h>
#include <signal.h>
#include "pcm.h"
#include "press.h"
#include "khaen.h"
#include "log.h"
#include "rec.h"

extern int g_daemon;
static char strbuf[512];

//static char *device = "plughw:0,0";                     /* playback device */
static char *device = "default";                     /* playback device */
static snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;    /* sample format */
static unsigned int rate = 48000;                       /* stream rate */
/* HiFiBerry driver does'nt support monoral. */
static unsigned int channels = 2;                       /* count of channels */
static unsigned int buffer_time = 25000;               /* ring buffer length in us */
static unsigned int period_time = 5000;               /* period time in us */
static double freq = 440;                               /* sinusoidal wave frequency in Hz */
static int verbose = 0;                                 /* verbose flag */
static int resample = 1;                                /* enable alsa-lib resampling */
static int period_event = 0;                            /* produce poll event after each period */
static snd_pcm_sframes_t buffer_size;
snd_pcm_sframes_t period_size;
static snd_output_t *output = NULL;
static int sigTermArrived = 0;

void sigint(int);
void sigkill(int);
void sigterm(int);
void SetAlsaMasterVolume(long volume);

static void generate_sine(const snd_pcm_channel_area_t *areas, 
                          snd_pcm_uframes_t offset,
                          int count, double *_phase)
{
        static double max_phase = 2. * M_PI;
        double phase = *_phase;
        double step = max_phase*freq/(double)rate;
        unsigned char *samples[channels];
        int steps[channels];
        unsigned int chn;
        int format_bits = snd_pcm_format_width(format);
        //unsigned int maxval = (1 << (format_bits - 1)) - 1;
        int bps = format_bits / 8;  /* bytes per sample */
        int phys_bps = snd_pcm_format_physical_width(format) / 8;
        int big_endian = snd_pcm_format_big_endian(format) == 1;
        int to_unsigned = snd_pcm_format_unsigned(format) == 1;
        int is_float = (format == SND_PCM_FORMAT_FLOAT_LE ||
                        format == SND_PCM_FORMAT_FLOAT_BE);
	int called = 0;

        /* verify and prepare the contents of areas */
        for (chn = 0; chn < channels; chn++) {
                if ((areas[chn].first % 8) != 0) {
                        sprintf(strbuf, "areas[%i].first == %i, aborting...\n", chn, areas[chn].first);
			log_(strbuf);
                        exit(EXIT_FAILURE);
                }
                samples[chn] = /*(signed short *)*/(((unsigned char *)areas[chn].addr) + (areas[chn].first / 8));
                if ((areas[chn].step % 16) != 0) {
                        sprintf(strbuf, "areas[%i].step == %i, aborting...\n", chn, areas[chn].step);
			log_(strbuf);
                        exit(EXIT_FAILURE);
                }
                steps[chn] = areas[chn].step / 8;
                samples[chn] += offset * steps[chn];
        }
        /* fill the channel areas */
        while (count-- > 0) {
                union {
                        float f;
                        int i;
                } fval;
                int res, i;
                if (is_float) {
                        fval.f = sin(phase);
                        res = fval.i;
                } else {
                        //res = sin(phase) * maxval;
                        res = pcm_read(called, count);
			called = 1;
		}
                if (to_unsigned)
                        res ^= 1U << (format_bits - 1);
                for (chn = 0; chn < channels; chn++) {
                        /* Generate data in native endian format */
                        if (big_endian) {
                                for (i = 0; i < bps; i++)
                                        *(samples[chn] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
                        } else {
                                for (i = 0; i < bps; i++)
                                        *(samples[chn] + i) = (res >>  i * 8) & 0xff;
                        }
                        samples[chn] += steps[chn];
                }
                phase += step;
                if (phase >= max_phase)
                        phase -= max_phase;
        }
        *_phase = phase;
}
static int set_hwparams(snd_pcm_t *handle,
                        snd_pcm_hw_params_t *params,
                        snd_pcm_access_t access)
{
        unsigned int rrate;
        snd_pcm_uframes_t size;
        int err, dir;
        /* choose all parameters */
        err = snd_pcm_hw_params_any(handle, params);
        if (err < 0) {
                log_str("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
                return err;
        }
        /* set hardware resampling */
        err = snd_pcm_hw_params_set_rate_resample(handle, params, resample);
        if (err < 0) {
                log_str("Resampling setup failed for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* set the interleaved read/write format */
        err = snd_pcm_hw_params_set_access(handle, params, access);
        if (err < 0) {
                log_str("Access type not available for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* set the sample format */
        err = snd_pcm_hw_params_set_format(handle, params, format);
        if (err < 0) {
                log_str("Sample format not available for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* set the count of channels */
        err = snd_pcm_hw_params_set_channels(handle, params, channels);
        if (err < 0) {
                sprintf(strbuf, "Channels count (%i) not available for playbacks: %s\n", channels, snd_strerror(err));
		log_(strbuf);
                return err;
        }
        /* set the stream rate */
        rrate = rate;
        err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
        if (err < 0) {
                sprintf(strbuf, "Rate %iHz not available for playback: %s\n", rate, snd_strerror(err));
		log_(strbuf);
                return err;
        }
        if (rrate != rate) {
                sprintf(strbuf, "Rate doesn't match (requested %iHz, get %iHz)\n", rate, err);
		log_(strbuf);
                return -EINVAL;
        }
        /* set the buffer time */
        err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, &dir);
        if (err < 0) {
                sprintf(strbuf, "Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
		log_(strbuf);
                return err;
        }
        err = snd_pcm_hw_params_get_buffer_size(params, &size);
        if (err < 0) {
                log_str("Unable to get buffer size for playback: %s\n", snd_strerror(err));
                return err;
        }
        buffer_size = size;
        /* set the period time */
        err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, &dir);
        if (err < 0) {
                sprintf(strbuf, "Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
		log_(strbuf);
                return err;
        }
        err = snd_pcm_hw_params_get_period_size(params, &size, &dir);
        if (err < 0) {
                log_str("Unable to get period size for playback: %s\n", snd_strerror(err));
                return err;
        }
        period_size = size;
        /* write the parameters to device */
        err = snd_pcm_hw_params(handle, params);
        if (err < 0) {
                log_str("Unable to set hw params for playback: %s\n", snd_strerror(err));
                return err;
        }
        return 0;
}
static int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams)
{
        int err;
        /* get the current swparams */
        err = snd_pcm_sw_params_current(handle, swparams);
        if (err < 0) {
                log_str("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* start the transfer when the buffer is almost full: */
        /* (buffer_size / avail_min) * avail_min */
        err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);
        if (err < 0) {
                log_str("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* allow the transfer when at least period_size samples can be processed */
        /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
        err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_event ? buffer_size : period_size);
        if (err < 0) {
                log_str("Unable to set avail min for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* enable period events when requested */
        if (period_event) {
                err = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
                if (err < 0) {
                        log_str("Unable to set period event: %s\n", snd_strerror(err));
                        return err;
                }
        }
        /* write the parameters to the playback device */
        err = snd_pcm_sw_params(handle, swparams);
        if (err < 0) {
                log_str("Unable to set sw params for playback: %s\n", snd_strerror(err));
                return err;
        }
        return 0;
}
/*
 *   Underrun and suspend recovery
 */
 
static int xrun_recovery(snd_pcm_t *handle, int err)
{
        if (verbose)
                log_("stream recovery\n");
        if (err == -EPIPE) {    /* under-run */
                err = snd_pcm_prepare(handle);
                if (err < 0)
                        log_str("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
                return 0;
        } else if (err == -ESTRPIPE) {
                while ((err = snd_pcm_resume(handle)) == -EAGAIN)
                        sleep(1);       /* wait until the suspend flag is released */
                if (err < 0) {
                        err = snd_pcm_prepare(handle);
                        if (err < 0)
                                log_str("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
                }
                return 0;
        }
        return err;
}
/*
 *   Transfer method - write only
 */
static int write_loop(snd_pcm_t *handle,
                      signed short *samples,
                      snd_pcm_channel_area_t *areas)
{
        double phase = 0;
        signed short *ptr;
        int err, cptr;
        while (1) {
                generate_sine(areas, 0, period_size, &phase);
                ptr = samples;
                cptr = period_size;
                while (cptr > 0) {
                        err = snd_pcm_writei(handle, ptr, cptr);
                        if (err == -EAGAIN)
                                continue;
                        if (err < 0) {
                                if (xrun_recovery(handle, err) < 0) {
                                        log_str("Write error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                                break;  /* skip one period */
                        }
                        ptr += err * channels;
                        cptr -= err;
                }
        }
}
 
/*
 *   Transfer method - write and wait for room in buffer using poll
 */
static int wait_for_poll(snd_pcm_t *handle, struct pollfd *ufds, unsigned int count)
{
        unsigned short revents;
        while (1) {
                poll(ufds, count, -1);
                snd_pcm_poll_descriptors_revents(handle, ufds, count, &revents);
                if (revents & POLLERR)
                        return -EIO;
                if (revents & POLLOUT)
                        return 0;
        }
}
static int write_and_poll_loop(snd_pcm_t *handle,
                               signed short *samples,
                               snd_pcm_channel_area_t *areas)
{
        struct pollfd *ufds;
        double phase = 0;
        signed short *ptr;
        int err, count, cptr, init;

        count = snd_pcm_poll_descriptors_count (handle);
        if (count <= 0) {
                log_("Invalid poll descriptors count\n");
                return count;
        }
        ufds = malloc(sizeof(struct pollfd) * count);
        if (ufds == NULL) {
                log_("No enough memory\n");
                return -ENOMEM;
        }
        if ((err = snd_pcm_poll_descriptors(handle, ufds, count)) < 0) {
                log_str("Unable to obtain poll descriptors for playback: %s\n", snd_strerror(err));
                return err;
        }
        init = 1;
        while (1) {
                if (!init) {
			if (sigTermArrived == 1){
				snd_pcm_drop(handle);
				log_("\nTerminated\n");
				//free_pcm();
				return 0;
			}
                        err = wait_for_poll(handle, ufds, count);
                        if (err < 0) {
                                if (snd_pcm_state(handle) == SND_PCM_STATE_XRUN ||
                                    snd_pcm_state(handle) == SND_PCM_STATE_SUSPENDED) {
                                        err = snd_pcm_state(handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                                        if (xrun_recovery(handle, err) < 0) {
                                                log_str("Write error: %s\n", snd_strerror(err));
                                                exit(EXIT_FAILURE);
                                        }
                                        init = 1;
                                } else {
                                        log_("Wait for poll failed\n");
                                        return err;
                                }
                        }
                }
		/* read pressure and key */
		//rtn_routine();
		/* get pcm data */
                generate_sine(areas, 0, period_size, &phase);
                ptr = samples;
                cptr = period_size;
                while (cptr > 0) {
                        err = snd_pcm_writei(handle, ptr, cptr);
                        if (err < 0) {
                                if (xrun_recovery(handle, err) < 0) {
                                        log_str("Write error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                                init = 1;
                                break;  /* skip one period */
                        }
                        if (snd_pcm_state(handle) == SND_PCM_STATE_RUNNING)
                                init = 0;
                        ptr += err * channels;
                        cptr -= err;
                        if (cptr == 0)
                                break;
                        /* it is possible, that the initial buffer cannot store */
                        /* all data from the last period, so wait awhile */
                        err = wait_for_poll(handle, ufds, count);
                        if (err < 0) {
                                if (snd_pcm_state(handle) == SND_PCM_STATE_XRUN ||
                                    snd_pcm_state(handle) == SND_PCM_STATE_SUSPENDED) {
                                        err = snd_pcm_state(handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                                        if (xrun_recovery(handle, err) < 0) {
                                                log_str("Write error: %s\n", snd_strerror(err));
                                                exit(EXIT_FAILURE);
                                        }
                                        init = 1;
                                } else {
                                        log_("Wait for poll failed\n");
                                        return err;
                                }
                        }
                }
        }
}
/*
 *   Transfer method - asynchronous notification
 */
struct async_private_data {
        signed short *samples;
        snd_pcm_channel_area_t *areas;
        double phase;
};
static void async_callback(snd_async_handler_t *ahandler)
{
        snd_pcm_t *handle = snd_async_handler_get_pcm(ahandler);
        struct async_private_data *data = snd_async_handler_get_callback_private(ahandler);
        signed short *samples = data->samples;
        snd_pcm_channel_area_t *areas = data->areas;
        snd_pcm_sframes_t avail;
        int err;
        
        avail = snd_pcm_avail_update(handle);
#if 0
        while (avail >= period_size) {
                generate_sine(areas, 0, period_size, &data->phase);
                err = snd_pcm_writei(handle, samples, period_size);
                if (err < 0) {
                        log_str("Write error: %s\n", snd_strerror(err));
                        exit(EXIT_FAILURE);
                }
                if (err != period_size) {
                        sprintf(strbuf, "Write error: written %i expected %li\n", err, period_size);
			log_(strbuf);
                        exit(EXIT_FAILURE);
                }
                avail = snd_pcm_avail_update(handle);
        }
#else
        while (avail >= period_size) {
                generate_sine(areas, 0, period_size, &data->phase);
                err = snd_pcm_writei(handle, samples, period_size);
                if (err < 0) {
                        log_str("Write error: %s\n", snd_strerror(err));
                        exit(EXIT_FAILURE);
                }
                if (err != period_size) {
                        sprintf(strbuf, "Write error: written %i expected %li\n", err, period_size);
			log_(strbuf);
                        exit(EXIT_FAILURE);
                }
                avail = snd_pcm_avail_update(handle);
        }
#endif
}
static int async_loop(snd_pcm_t *handle,
                      signed short *samples,
                      snd_pcm_channel_area_t *areas)
{
        struct async_private_data data;
        snd_async_handler_t *ahandler;
        int err, count;
        data.samples = samples;
        data.areas = areas;
        data.phase = 0;
        err = snd_async_add_pcm_handler(&ahandler, handle, async_callback, &data);
        if (err < 0) {
                log_("Unable to register async handler\n");
                exit(EXIT_FAILURE);
        }
        for (count = 0; count < 2; count++) {
                generate_sine(areas, 0, period_size, &data.phase);
                err = snd_pcm_writei(handle, samples, period_size);
                if (err < 0) {
                        log_str("Initial write error: %s\n", snd_strerror(err));
                        exit(EXIT_FAILURE);
                }
                if (err != period_size) {
                        sprintf(strbuf, "Initial write error: written %i expected %li\n", err, period_size);
			log_(strbuf);
                        exit(EXIT_FAILURE);
                }
        }
        if (snd_pcm_state(handle) == SND_PCM_STATE_PREPARED) {
                err = snd_pcm_start(handle);
                if (err < 0) {
                        log_str("Start error: %s\n", snd_strerror(err));
                        exit(EXIT_FAILURE);
                }
        }
        /* because all other work is done in the signal handler,
           suspend the process */
        while (1) {
                sleep(1);
		if (sigTermArrived){
			//log_("\nTerminated\n");
			pcm_free();
			rec_free();
			break;
		}
        }
	return 0;
}
/*
 *   Transfer method - asynchronous notification + direct write
 */
static void async_direct_callback(snd_async_handler_t *ahandler)
{
        snd_pcm_t *handle = snd_async_handler_get_pcm(ahandler);
        struct async_private_data *data = snd_async_handler_get_callback_private(ahandler);
        const snd_pcm_channel_area_t *my_areas;
        snd_pcm_uframes_t offset, frames, size;
        snd_pcm_sframes_t avail, commitres;
        snd_pcm_state_t state;
        int first = 0, err;
        
        while (1) {
                state = snd_pcm_state(handle);
                if (state == SND_PCM_STATE_XRUN) {
                        err = xrun_recovery(handle, -EPIPE);
                        if (err < 0) {
                                log_str("XRUN recovery failed: %s\n", snd_strerror(err));
                                exit(EXIT_FAILURE);
                        }
                        first = 1;
                } else if (state == SND_PCM_STATE_SUSPENDED) {
                        err = xrun_recovery(handle, -ESTRPIPE);
                        if (err < 0) {
                                log_str("SUSPEND recovery failed: %s\n", snd_strerror(err));
                                exit(EXIT_FAILURE);
                        }
                }
                avail = snd_pcm_avail_update(handle);
                if (avail < 0) {
                        err = xrun_recovery(handle, avail);
                        if (err < 0) {
                                log_str("avail update failed: %s\n", snd_strerror(err));
                                exit(EXIT_FAILURE);
                        }
                        first = 1;
                        continue;
                }
                if (avail < period_size) {
                        if (first) {
                                first = 0;
                                err = snd_pcm_start(handle);
                                if (err < 0) {
                                        log_str("Start error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                        } else {
                                break;
                        }
                        continue;
                }
                size = period_size;
                while (size > 0) {
                        frames = size;
                        err = snd_pcm_mmap_begin(handle, &my_areas, &offset, &frames);
                        if (err < 0) {
                                if ((err = xrun_recovery(handle, err)) < 0) {
                                        log_str("MMAP begin avail error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                                first = 1;
                        }
                        generate_sine(my_areas, offset, frames, &data->phase);
                        commitres = snd_pcm_mmap_commit(handle, offset, frames);
                        if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
                                if ((err = xrun_recovery(handle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
                                        log_str("MMAP commit error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                                first = 1;
                        }
                        size -= frames;
                }
        }
}
static int async_direct_loop(snd_pcm_t *handle,
                             signed short *samples ATTRIBUTE_UNUSED,
                             snd_pcm_channel_area_t *areas ATTRIBUTE_UNUSED)
{
        struct async_private_data data;
        snd_async_handler_t *ahandler;
        const snd_pcm_channel_area_t *my_areas;
        snd_pcm_uframes_t offset, frames, size;
        snd_pcm_sframes_t commitres;
        int err, count;
        data.samples = NULL;    /* we do not require the global sample area for direct write */
        data.areas = NULL;      /* we do not require the global areas for direct write */
        data.phase = 0;
        err = snd_async_add_pcm_handler(&ahandler, handle, async_direct_callback, &data);
        if (err < 0) {
                log_("Unable to register async handler\n");
                exit(EXIT_FAILURE);
        }
        for (count = 0; count < 2; count++) {
                size = period_size;
                while (size > 0) {
                        frames = size;
                        err = snd_pcm_mmap_begin(handle, &my_areas, &offset, &frames);
                        if (err < 0) {
                                if ((err = xrun_recovery(handle, err)) < 0) {
                                        log_str("MMAP begin avail error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                        }
                        generate_sine(my_areas, offset, frames, &data.phase);
                        commitres = snd_pcm_mmap_commit(handle, offset, frames);
                        if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
                                if ((err = xrun_recovery(handle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
                                        log_str("MMAP commit error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                        }
                        size -= frames;
                }
        }
        err = snd_pcm_start(handle);
        if (err < 0) {
                log_str("Start error: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        /* because all other work is done in the signal handler,
           suspend the process */
        while (1) {
                sleep(1);
        }
}
/*
 *   Transfer method - direct write only
 */
static int direct_loop(snd_pcm_t *handle,
                       signed short *samples ATTRIBUTE_UNUSED,
                       snd_pcm_channel_area_t *areas ATTRIBUTE_UNUSED)
{
        double phase = 0;
        const snd_pcm_channel_area_t *my_areas;
        snd_pcm_uframes_t offset, frames, size;
        snd_pcm_sframes_t avail, commitres;
        snd_pcm_state_t state;
        int err, first = 1;
        while (1) {
                state = snd_pcm_state(handle);
                if (state == SND_PCM_STATE_XRUN) {
                        err = xrun_recovery(handle, -EPIPE);
                        if (err < 0) {
                                log_str("XRUN recovery failed: %s\n", snd_strerror(err));
                                return err;
                        }
                        first = 1;
                } else if (state == SND_PCM_STATE_SUSPENDED) {
                        err = xrun_recovery(handle, -ESTRPIPE);
                        if (err < 0) {
                                log_str("SUSPEND recovery failed: %s\n", snd_strerror(err));
                                return err;
                        }
                }
                avail = snd_pcm_avail_update(handle);
                if (avail < 0) {
                        err = xrun_recovery(handle, avail);
                        if (err < 0) {
                                log_str("avail update failed: %s\n", snd_strerror(err));
                                return err;
                        }
                        first = 1;
                        continue;
                }
                if (avail < period_size) {
                        if (first) {
                                first = 0;
                                err = snd_pcm_start(handle);
                                if (err < 0) {
                                        log_str("Start error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                        } else {
                                err = snd_pcm_wait(handle, -1);
                                if (err < 0) {
                                        if ((err = xrun_recovery(handle, err)) < 0) {
                                                log_str("snd_pcm_wait error: %s\n", snd_strerror(err));
                                                exit(EXIT_FAILURE);
                                        }
                                        first = 1;
                                }
                        }
                        continue;
                }
                size = period_size;
                while (size > 0) {
                        frames = size;
                        err = snd_pcm_mmap_begin(handle, &my_areas, &offset, &frames);
                        if (err < 0) {
                                if ((err = xrun_recovery(handle, err)) < 0) {
                                        log_str("MMAP begin avail error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                                first = 1;
                        }
                        generate_sine(my_areas, offset, frames, &phase);
                        commitres = snd_pcm_mmap_commit(handle, offset, frames);
                        if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
                                if ((err = xrun_recovery(handle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
                                        log_str("MMAP commit error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                                first = 1;
                        }
                        size -= frames;
                }
        }
}
 
/*
 *   Transfer method - direct write only using mmap_write functions
 */
static int direct_write_loop(snd_pcm_t *handle,
                             signed short *samples,
                             snd_pcm_channel_area_t *areas)
{
        double phase = 0;
        signed short *ptr;
        int err, cptr;
        while (1) {
                generate_sine(areas, 0, period_size, &phase);
                ptr = samples;
                cptr = period_size;
                while (cptr > 0) {
                        err = snd_pcm_mmap_writei(handle, ptr, cptr);
                        if (err == -EAGAIN)
                                continue;
                        if (err < 0) {
                                if (xrun_recovery(handle, err) < 0) {
                                        log_str("Write error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                                break;  /* skip one period */
                        }
                        ptr += err * channels;
                        cptr -= err;
                }
        }
}
 
/*
 *
 */
struct transfer_method {
        const char *name;
        snd_pcm_access_t access;
        int (*transfer_loop)(snd_pcm_t *handle,
                             signed short *samples,
                             snd_pcm_channel_area_t *areas);
};
static struct transfer_method transfer_methods[] = {
        { "write", SND_PCM_ACCESS_RW_INTERLEAVED, write_loop },
        { "write_and_poll", SND_PCM_ACCESS_RW_INTERLEAVED, write_and_poll_loop },
        { "async", SND_PCM_ACCESS_RW_INTERLEAVED, async_loop },
        { "async_direct", SND_PCM_ACCESS_MMAP_INTERLEAVED, async_direct_loop },
        { "direct_interleaved", SND_PCM_ACCESS_MMAP_INTERLEAVED, direct_loop },
        { "direct_noninterleaved", SND_PCM_ACCESS_MMAP_NONINTERLEAVED, direct_loop },
        { "direct_write", SND_PCM_ACCESS_MMAP_INTERLEAVED, direct_write_loop },
        { NULL, SND_PCM_ACCESS_RW_INTERLEAVED, NULL }
};

//main
int khean(void)
{
        int method = 1; // write_and_poll

	//khean_vol(0);
	press_init();
	log_("khean: init.done\n");

  if (!g_daemon){
	if (SIG_ERR == signal(SIGINT, sigint)) { /* CTRL+C */
		log_("failed to set signal handler SIGINT\n");
		exit(1);
	}
  }else{
	/* ERROR !
	if (SIG_ERR == signal(SIGKILL, sigkill)) {
		log_("failed to set signal handler:SIGLKILL\n");
		exit(1);
	}
	*/
  }

        snd_pcm_t *handle;
        int err;
        snd_pcm_hw_params_t *hwparams;
        snd_pcm_sw_params_t *swparams;
        signed short *samples;
        unsigned int chn;
        snd_pcm_channel_area_t *areas;
        snd_pcm_hw_params_alloca(&hwparams);
        snd_pcm_sw_params_alloca(&swparams);
        
        err = snd_output_stdio_attach(&output, stdout, 0);
        if (err < 0) {
                log_str("Output failed: %s\n", snd_strerror(err));
                return 0;
        }
        //log_str("Playback device is %s\n", device);
        //sprintf(strbuf, "Stream parameters are %iHz, %s, %i channels\n", rate, snd_pcm_format_name(format), channels);
	//log_(strbuf);
        //printf("Sine wave rate is %.4fHz\n", freq);
        //log_str("Using transfer method: %s\n", transfer_methods[method].name);
        if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
                log_str("Playback open error: %s\n", snd_strerror(err));
                return 0;
        }
        
        if ((err = set_hwparams(handle, hwparams, transfer_methods[method].access)) < 0) {
                log_str("Setting of hwparams failed: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        if ((err = set_swparams(handle, swparams)) < 0) {
                log_str("Setting of swparams failed: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        if (verbose > 0)
                snd_pcm_dump(handle, output);
        samples = malloc((period_size * channels * snd_pcm_format_physical_width(format)) / 8);
        if (samples == NULL) {
                log_("No enough memory\n");
                exit(EXIT_FAILURE);
        }
        
        areas = calloc(channels, sizeof(snd_pcm_channel_area_t));
        if (areas == NULL) {
                log_("No enough memory\n");
                exit(EXIT_FAILURE);
        }
        for (chn = 0; chn < channels; chn++) {
                areas[chn].addr = samples;
                areas[chn].first = chn * snd_pcm_format_physical_width(format);
                areas[chn].step = channels * snd_pcm_format_physical_width(format);
        }
        err = transfer_methods[method].transfer_loop(handle, samples, areas);
        if (err < 0){
                log_str("Transfer failed: %s\n", snd_strerror(err));
	}else{
                //log_("stop transfer\n" );
	}
        free(areas);
        free(samples);
        snd_pcm_close(handle);
        return 0;
}

void sigkill(int sig) {
	sigint(sig);
}
void sigterm(int sig) {
	sigint(sig);
}
void sigint(int sig) {
	sigTermArrived = 1;
}

/* 0 to 100 */
void SetAlsaMasterVolume(long volume)
{
    long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";
    //const char *selem_name = "PCM";

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
    if (elem == NULL){
	log_("snd_mixer_find_selem() returns:NULL\n");
    }
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

    snd_mixer_close(handle);
}
void khean_vol(int act)
{
	static int vol = 50;
	static int first = 0;

	if (first == 0){
		first++;
		SetAlsaMasterVolume(vol);
	}
	switch(act){
	case 0:
		break;
	case -1:
		if (vol < 5){
			vol = 0;
		}else{
			vol -= 5;
		}
		break;
	case 1:
		if (vol >= 100){
			vol = 100;
		}else{
			vol += 5;
		}
		break;
	default:
		return;
	}
	SetAlsaMasterVolume(vol);
}
