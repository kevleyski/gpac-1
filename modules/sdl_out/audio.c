/*
 *			GPAC - Multimedia Framework C SDK
 *
 *			Copyright (c) Jean Le Feuvre 2000-2005
 *					All rights reserved
 *
 *  This file is part of GPAC / SDL audio and video module
 *
 *  GPAC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  GPAC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *   
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *		
 */

#ifdef GPAC_IPHONE
#include "libgpac_symbols.h"
#endif

#include "sdl_out.h"

#define SDLAUD()	SDLAudCtx *ctx = (SDLAudCtx *)dr->opaque

static void sdl_close_audio(){
  SDL_CloseAudio();
}


void sdl_fill_audio(void *udata, Uint8 *stream, int len)
{
	GF_AudioOutput *dr = (GF_AudioOutput *)udata;
	SDLAUD();
	if (ctx->volume != SDL_MIX_MAXVOLUME){
		ctx->audioBuff = gf_realloc( ctx->audioBuff, sizeof(Uint8) * len);
		dr->FillBuffer(dr->audio_renderer, ctx->audioBuff, len);	
		SDL_MixAudio(stream, ctx->audioBuff, len, ctx->volume);
	} else {
		dr->FillBuffer(dr->audio_renderer, stream, len);	
	}
}


static GF_Err SDLAud_Setup(GF_AudioOutput *dr, void *os_handle, u32 num_buffers, u32 total_duration)
{
	u32 flags;
	SDL_AudioSpec want_format, got_format;
	SDLAUD();

	/*init sdl*/
	if (!SDLOUT_InitSDL()) return GF_IO_ERR;

	flags = SDL_WasInit(SDL_INIT_AUDIO);
	if (!(flags & SDL_INIT_AUDIO)) {
		if (SDL_InitSubSystem(SDL_INIT_AUDIO)<0) {
			SDLOUT_CloseSDL();
			return GF_IO_ERR;
		}
	}
	/*check we can open the device*/
	memset(&want_format, 0, sizeof(SDL_AudioSpec));
	want_format.freq = 44100;
	want_format.format = AUDIO_S16SYS;
	want_format.channels = 2;
	want_format.samples = 1024;
	want_format.callback = sdl_fill_audio;
	want_format.userdata = dr;
	if ( SDL_OpenAudio(&want_format, &got_format) < 0 ) {
		sdl_close_audio();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		SDLOUT_CloseSDL();
		return GF_IO_ERR;
	}
	sdl_close_audio();
	ctx->is_init = 1;
	ctx->num_buffers = num_buffers;
	ctx->total_duration = total_duration;
	return GF_OK;
}

static void SDLAud_Shutdown(GF_AudioOutput *dr)
{
	SDLAUD();
	sdl_close_audio();
	if (ctx->is_init) {
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		SDLOUT_CloseSDL();
		ctx->is_init = 0;
	}
}

void SDL_DeleteAudio(void *ifce){
	SDLAudCtx *ctx;
	GF_AudioOutput * dr;
	if (!ifce)
		return;
	dr = ( GF_AudioOutput *) ifce;
	ctx = (SDLAudCtx *)dr->opaque;
	if (!ctx)
		return;
	if (ctx->audioBuff)
		gf_free(ctx->audioBuff);
	ctx->audioBuff = NULL;
	gf_free( ctx );
	dr->opaque = NULL;
	gf_free( dr );
}

static GF_Err SDLAud_ConfigureOutput(GF_AudioOutput *dr, u32 *SampleRate, u32 *NbChannels, u32 *nbBitsPerSample, u32 channel_cfg)
{
	s32 nb_samples;
	SDL_AudioSpec want_format, got_format;
	SDLAUD();

	sdl_close_audio();
	ctx->is_running = 0;

	memset(&want_format, 0, sizeof(SDL_AudioSpec));
	want_format.freq = *SampleRate;
	want_format.format = (*nbBitsPerSample==16) ? AUDIO_S16SYS : AUDIO_S8;
	want_format.channels = *NbChannels;
	want_format.callback = sdl_fill_audio;
	want_format.userdata = dr;

	if (ctx->num_buffers && ctx->total_duration) {
		nb_samples = want_format.freq * ctx->total_duration;
		nb_samples /= (1000 * ctx->num_buffers);
		if (nb_samples % 2) nb_samples++;
	} else {
		nb_samples = 1024;
	}

	/*respect SDL need for power of 2*/
	want_format.samples = 1;
	while (want_format.samples*2<nb_samples) want_format.samples *= 2;

	if ( SDL_OpenAudio(&want_format, &got_format) < 0 ) return GF_IO_ERR;
	ctx->is_running = 1;
	ctx->delay_ms = (got_format.samples * 1000) / got_format.freq;
	ctx->total_size = got_format.samples;
	*SampleRate = got_format.freq;
	*NbChannels = got_format.channels;

	switch (got_format.format) {
	case AUDIO_S8:
	case AUDIO_U8:
		*nbBitsPerSample = 8;
		break;
	default:
		*nbBitsPerSample = 16;
		break;
	}
	/*and play*/
	SDL_PauseAudio(0);
	return GF_OK;
}

static u32 SDLAud_GetAudioDelay(GF_AudioOutput *dr)
{
	SDLAUD();
	return ctx->delay_ms;
}

static u32 SDLAud_GetTotalBufferTime(GF_AudioOutput *dr)
{
	SDLAUD();
	return ctx->delay_ms;
}

static void SDLAud_SetVolume(GF_AudioOutput *dr, u32 Volume)
{
	SDLAUD();
	if (Volume > 98)
		ctx->volume = SDL_MIX_MAXVOLUME;
	else
		ctx->volume = Volume * SDL_MIX_MAXVOLUME / 100;
}

static void SDLAud_SetPan(GF_AudioOutput *dr, u32 pan)
{
	/*not supported by SDL*/
}

static void SDLAud_Play(GF_AudioOutput *dr, u32 PlayType)
{
	SDL_PauseAudio(PlayType ? 0 : 1);
}

static void SDLAud_SetPriority(GF_AudioOutput *dr, u32 priority)
{
	/*not supported by SDL*/
}

static GF_Err SDLAud_QueryOutputSampleRate(GF_AudioOutput *dr, u32 *desired_samplerate, u32 *NbChannels, u32 *nbBitsPerSample)
{
	/*cannot query supported formats in SDL...*/
	return GF_OK;
}

void *SDL_NewAudio()
{
	SDLAudCtx *ctx;
	GF_AudioOutput *dr;


	ctx = gf_malloc(sizeof(SDLAudCtx));
	memset(ctx, 0, sizeof(SDLAudCtx));

	dr = gf_malloc(sizeof(GF_AudioOutput));
	memset(dr, 0, sizeof(GF_AudioOutput));
	GF_REGISTER_MODULE_INTERFACE(dr, GF_AUDIO_OUTPUT_INTERFACE, "SDL Audio Output", "gpac distribution");

	dr->opaque = ctx;

	dr->Setup = SDLAud_Setup;
	dr->Shutdown = SDLAud_Shutdown;
	dr->ConfigureOutput = SDLAud_ConfigureOutput;
	dr->SetVolume = SDLAud_SetVolume;
	dr->SetPan = SDLAud_SetPan;
	dr->Play = SDLAud_Play;
	dr->SetPriority = SDLAud_SetPriority;
	dr->GetAudioDelay = SDLAud_GetAudioDelay;
	dr->GetTotalBufferTime = SDLAud_GetTotalBufferTime;

	dr->QueryOutputSampleRate = SDLAud_QueryOutputSampleRate;
	/*always threaded*/
	dr->SelfThreaded = 1;
	ctx->audioBuff = NULL;
	ctx->volume = SDL_MIX_MAXVOLUME;
	return dr;
}

