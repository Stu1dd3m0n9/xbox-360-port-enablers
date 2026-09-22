/* sdl360_audio.cpp: XAudio2 32kHz stereo S16 push. XDK group. Generic. */
#include <xtl.h>
#include <xaudio2.h>
#include <stdlib.h>
#include <string.h>
#include "sdl360.h"
extern "C" int DbgPrint(const char *, ...);
namespace {
IXAudio2 *g_xa; IXAudio2MasteringVoice *g_master; IXAudio2SourceVoice *g_src;
bool g_started; volatile LONG g_sub, g_done;
class CB : public IXAudio2VoiceCallback {
public: void STDMETHODCALLTYPE OnStreamEnd() {}
 void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() {}
 void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) {}
 void STDMETHODCALLTYPE OnBufferEnd(void *c) { InterlockedExchangeAdd(&g_done, (LONG)(UINT_PTR)c); }
 void STDMETHODCALLTYPE OnBufferStart(void *) {} void STDMETHODCALLTYPE OnLoopEnd(void *) {}
 void STDMETHODCALLTYPE OnVoiceError(void *, HRESULT) {}
};
CB g_cb;
}
#define BLK 64
static void *g_b[BLK]; static uint32_t g_bs[BLK], g_head, g_tail;
int sdl360_audio_init(void) {
    if (g_src) return 1;
    if (FAILED(XAudio2Create(&g_xa, 0, XAUDIO2_DEFAULT_PROCESSOR))) return 0;
    if (FAILED(g_xa->CreateMasteringVoice(&g_master, XAUDIO2_DEFAULT_CHANNELS,
        XAUDIO2_DEFAULT_SAMPLERATE, 0, 0, NULL))) return 0;
    WAVEFORMATEX w; ZeroMemory(&w, sizeof(w));
    w.wFormatTag = WAVE_FORMAT_PCM; w.nChannels = 2; w.nSamplesPerSec = 32000;
    w.wBitsPerSample = 16; w.nBlockAlign = 4; w.nAvgBytesPerSec = 128000;
    if (FAILED(g_xa->CreateSourceVoice(&g_src, &w, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &g_cb, NULL, NULL))) {
        DbgPrint("[SDL360] src voice failed\n"); return 0;
    }
    DbgPrint("[SDL360] audio 32k stereo ready\n"); return 1;
}
int sdl360_audio_buffered_frames(void) { return (int)((g_sub - g_done) / 4); }
void sdl360_audio_push(const int16_t *pcm, int frames) {
    if (!g_src || !pcm || frames <= 0) return;
    if (sdl360_audio_buffered_frames() >= 6000) return;
    LONG done = g_done; static LONG rec = 0;
    while (g_tail != g_head) {
        if (rec + (LONG)g_bs[g_tail] > done) break;
        rec += (LONG)g_bs[g_tail]; free(g_b[g_tail]); g_b[g_tail] = 0;
        g_tail = (g_tail + 1) % BLK;
    }
    uint32_t nx = (g_head + 1) % BLK; if (nx == g_tail) return;
    size_t n = (size_t)frames * 4;
    uint8_t *cp = (uint8_t *)malloc(n); if (!cp) return;
    memcpy(cp, pcm, n);
    XAUDIO2_BUFFER xb; ZeroMemory(&xb, sizeof(xb));
    xb.AudioBytes = (UINT32)n; xb.pAudioData = cp; xb.pContext = (void *)(UINT_PTR)n;
    if (FAILED(g_src->SubmitSourceBuffer(&xb, NULL))) { free(cp); return; }
    g_b[g_head] = cp; g_bs[g_head] = (uint32_t)n; g_head = nx;
    InterlockedExchangeAdd(&g_sub, (LONG)n);
    if (!g_started) { g_started = true; g_src->Start(0, XAUDIO2_COMMIT_NOW); }
}
