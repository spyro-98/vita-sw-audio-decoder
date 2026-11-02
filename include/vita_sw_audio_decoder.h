#ifndef VITA_SW_AUDIO_DECODER_H
#define VITA_SW_AUDIO_DECODER_H

#include <libavutil/rational.h>
#include <vita_audio_types.h>
struct AVCodecParameters;
struct AVPacket;
#ifdef __cplusplus
extern "C" {
#endif

#define VITA_SOFTWARE_AUDIO_RATE 48000
#define VITA_SOFTWARE_AUDIO_CHANNELS 2
#define VITA_SOFTWARE_AUDIO_FRAMES 8192
typedef struct VitaSwAudioDecoder VitaSwAudioDecoder;

VitaSwAudioDecoder *vita_sw_audio_decoder_create(void);
/* Copies stream configuration; the caller retains codec parameters/packets.
 * Opens AAC/AC-3/MP3, with bounded stereo PCM16 output at 48 kHz. */
int vita_sw_audio_decoder_open(VitaSwAudioDecoder *, const struct AVCodecParameters *, AVRational);
/* FFmpeg send semantics: receive pending output on EAGAIN; NULL drains EOF. */
int vita_sw_audio_decoder_send(VitaSwAudioDecoder *, const struct AVPacket *);
/* 1: PCM; 0: needs input; negative AVERROR, including EOF after resampler drain.
 * PCM is borrowed until the next receive/close; pts_us uses stream timestamps. */
int vita_sw_audio_decoder_receive(VitaSwAudioDecoder *, VitaDecodedAudio *, int64_t *pts_us);
void vita_sw_audio_decoder_close(VitaSwAudioDecoder *);
void vita_sw_audio_decoder_destroy(VitaSwAudioDecoder *);

#ifdef __cplusplus
}
#endif
#endif
