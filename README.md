# vita-sw-audio-decoder

FFmpeg AAC/MP3/AC-3 decoding (AC-3 up to 5.1 input), downmixed/resampled to stereo PCM16 at 48 kHz. Audio API **1.0.0**, extracted for **VitaMediaDeck Beta 1.4.4**. The video backend remains independent.

## Use

Include `vita_sw_audio_decoder.h`, create an opaque decoder, open it with stream parameters, decode, then close or destroy it. Zero/designated-initialize configs. Returned PCM is borrowed until the next decode/receive or close; copy it before reuse. A decoder object is used by one worker at a time; join that worker before destruction. Closing twice and reopening the same object are safe.

The library does not own AudioOut, demux, volume, playback clocks or threads. VitaMediaDeck provides those once in `src/media/audio_playback.c`, including codec-specific grains (AAC 1024, MP3 576/1152, AC-3 1536), continuous PCM packing, EOF drain and cancellation.

## Build

Use VitaSDK and CMake. The CMake target is `VitaSwAudioDecoder::VitaSwAudioDecoder`.

Keep `vita-hw-audio-decoder` beside this repository for the shared headers, or set `VITA_AUDIO_DECODER_INTERFACE_ROOT`. Set `VITA_SW_AUDIO_DECODER_FFMPEG_ROOT` to the compatible FFmpeg 6 prefix with AAC, AC-3, MP3 and swresample enabled. Consumers of an installed package must also provide FFmpeg headers/libraries. Send packets until accepted, receive until more input is needed, then send NULL at EOF and receive through complete decoder/resampler drain.

Host regressions live in VitaMediaDeck `tests/multiaudio` and `tests/audio_volume`. Vita firmware calls are substituted on the host; physical audio output and A/V performance require console testing.

GPL-3.0-only. See [LICENSE](LICENSE). Development sources are published on the `master` branch; VitaMediaDeck pins an exact component commit in its dependency lock.
