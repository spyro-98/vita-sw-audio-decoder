#include "vita_sw_audio_decoder.h"
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <stdlib.h>
struct VitaSwAudioDecoder {
 AVCodecContext *codec;AVFrame *frame;SwrContext *resampler;int16_t *pcm;
 AVChannelLayout input_layout;enum AVSampleFormat input_format;int input_rate;
 AVRational time_base;int codec_eof,pts_valid;int64_t origin_us;uint64_t samples;
};
#include <vita_audio_formats.h>
#include <libavutil/mathematics.h>
#include <libavutil/mem.h>
#include <string.h>

void vita_sw_audio_decoder_close(VitaSwAudioDecoder *s){
 if(!s)return;
 swr_free(&s->resampler);av_channel_layout_uninit(&s->input_layout);
 av_frame_free(&s->frame);avcodec_free_context(&s->codec);av_freep(&s->pcm);
 memset(s,0,sizeof(*s));
}
int vita_sw_audio_decoder_open(VitaSwAudioDecoder *s,const AVCodecParameters *p,AVRational tb){
 if(!s)return AVERROR(EINVAL);
 vita_sw_audio_decoder_close(s);s->time_base=tb;
 if(!vmd_audio_parameters_playable(p)||(p->codec_id!=AV_CODEC_ID_AAC&&p->codec_id!=AV_CODEC_ID_AC3&&p->codec_id!=AV_CODEC_ID_MP3)||tb.num<=0||tb.den<=0)return AVERROR_DECODER_NOT_FOUND;
 const AVCodec *codec=avcodec_find_decoder(p->codec_id);if(!codec)return AVERROR_DECODER_NOT_FOUND;
 s->codec=avcodec_alloc_context3(codec);s->frame=av_frame_alloc();
 s->pcm=av_malloc(VITA_SOFTWARE_AUDIO_FRAMES*VITA_SOFTWARE_AUDIO_CHANNELS*sizeof(*s->pcm));
 int r=AVERROR(ENOMEM);if(!s->codec||!s->frame||!s->pcm)goto fail;
 r=avcodec_parameters_to_context(s->codec,p);if(r<0)goto fail;
 s->codec->thread_count=1;s->codec->pkt_timebase=tb;
 r=avcodec_open2(s->codec,codec,NULL);if(r<0)goto fail;
 return 0;
fail:vita_sw_audio_decoder_close(s);return r;
}
int vita_sw_audio_decoder_send(VitaSwAudioDecoder *s,const AVPacket *p){
 return s&&s->codec?avcodec_send_packet(s->codec,p):AVERROR(EINVAL);
}
static int output_pcm(VitaSwAudioDecoder *s,VitaDecodedAudio *out,int64_t *pts,int frames){
 if(frames<=0)return frames;
 out->pcm=s->pcm;out->frames=(uint32_t)frames;out->channels=VITA_SOFTWARE_AUDIO_CHANNELS;out->sample_rate=VITA_SOFTWARE_AUDIO_RATE;
 *pts=s->pts_valid?s->origin_us+av_rescale_q((int64_t)s->samples,(AVRational){1,VITA_SOFTWARE_AUDIO_RATE},AV_TIME_BASE_Q):AV_NOPTS_VALUE;
 s->samples+=(unsigned)frames;return 1;
}
int vita_sw_audio_decoder_receive(VitaSwAudioDecoder *s,VitaDecodedAudio *out,int64_t *pts){
 if(!s||!s->codec||!out||!pts)return AVERROR(EINVAL);
 memset(out,0,sizeof(*out));*pts=AV_NOPTS_VALUE;
 uint8_t *destination=(uint8_t*)s->pcm;
 if(s->codec_eof){
  int n=s->resampler?swr_convert(s->resampler,&destination,VITA_SOFTWARE_AUDIO_FRAMES,NULL,0):0;
  if(n<0)return n;
  return n?output_pcm(s,out,pts,n):AVERROR_EOF;
 }
 av_frame_unref(s->frame);int r=avcodec_receive_frame(s->codec,s->frame);
 if(r==AVERROR(EAGAIN))return 0;
 if(r==AVERROR_EOF){s->codec_eof=1;return vita_sw_audio_decoder_receive(s,out,pts);}
 if(r<0)return r;
 AVFrame *frame=s->frame;
 if(frame->nb_samples<=0||frame->nb_samples>VITA_SOFTWARE_AUDIO_FRAMES||frame->sample_rate<8000||frame->sample_rate>48000||frame->ch_layout.nb_channels<1||frame->ch_layout.nb_channels>6)return AVERROR_INVALIDDATA;
 if(!s->resampler){
  AVChannelLayout layout=AV_CHANNEL_LAYOUT_STEREO;
  r=swr_alloc_set_opts2(&s->resampler,&layout,AV_SAMPLE_FMT_S16,VITA_SOFTWARE_AUDIO_RATE,&frame->ch_layout,(enum AVSampleFormat)frame->format,frame->sample_rate,0,NULL);
  if(r<0)return r;
  r=swr_init(s->resampler);if(r<0)return r;
  r=av_channel_layout_copy(&s->input_layout,&frame->ch_layout);if(r<0)return r;
  s->input_rate=frame->sample_rate;s->input_format=(enum AVSampleFormat)frame->format;
 }else if(s->input_rate!=frame->sample_rate||s->input_format!=frame->format||av_channel_layout_compare(&s->input_layout,&frame->ch_layout))return AVERROR_INVALIDDATA;
 int64_t stamp=frame->best_effort_timestamp!=AV_NOPTS_VALUE?frame->best_effort_timestamp:frame->pts;
 if(stamp!=AV_NOPTS_VALUE){
  int64_t frame_us=av_rescale_q(stamp,s->time_base,AV_TIME_BASE_Q);
  int64_t delay_us=av_rescale(swr_get_delay(s->resampler,s->input_rate),1000000,s->input_rate);
  int64_t next=s->origin_us+av_rescale_q((int64_t)s->samples,(AVRational){1,VITA_SOFTWARE_AUDIO_RATE},AV_TIME_BASE_Q);
  if(!s->pts_valid||frame_us-delay_us-next>100000||next-(frame_us-delay_us)>100000){s->origin_us=frame_us-delay_us;s->samples=0;s->pts_valid=1;}
 }
 int capacity=swr_get_out_samples(s->resampler,frame->nb_samples);
 if(capacity<0||capacity>VITA_SOFTWARE_AUDIO_FRAMES)return AVERROR_INVALIDDATA;
 r=swr_convert(s->resampler,&destination,VITA_SOFTWARE_AUDIO_FRAMES,(const uint8_t**)frame->extended_data,frame->nb_samples);
 return output_pcm(s,out,pts,r);
}

VitaSwAudioDecoder *vita_sw_audio_decoder_create(void){return calloc(1,sizeof(VitaSwAudioDecoder));}
void vita_sw_audio_decoder_destroy(VitaSwAudioDecoder *decoder){if(decoder){vita_sw_audio_decoder_close(decoder);free(decoder);}}
