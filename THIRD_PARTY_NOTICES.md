# Third-party notices

This module is extracted from VitaMediaDeck's former `vita-sw-decoder` package and retains GPL-3.0-only licensing. See LICENSE. The PlayStation Vita firmware API is called through VitaSDK; no Sony firmware binary is distributed here.

Software audio uses FFmpeg n6.0 `libavcodec`, `libswresample` and `libavutil`. VitaMediaDeck shares its reviewed, patched FFmpeg installation between audio/video modules. Preserve that prefix's license notices and corresponding source (including any GPL-covered patches) when distributing linked binaries. No third-party source or binary is vendored in this repository.
