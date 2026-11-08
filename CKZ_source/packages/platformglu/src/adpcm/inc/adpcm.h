// adpcm.h - IMA ADPCM decoder (after Jack Jansen's public domain adpcm.c), extended to
// interleaved stereo: every input byte holds one 4-bit code per channel.
#ifndef ADPCM_H
#define ADPCM_H

/** Decoder state, one predictor per channel. */
struct adpcm_state {
    short valprev[2];   // 0x00 previous output value per channel
    char index[2];      // 0x04 index into stepsizeTable per channel
};

/** Decode `len` input bytes (2 * len samples) from `indata` into `outdata`. */
void adpcm_decoder(signed char* indata, short* outdata, int len, int channels, adpcm_state* state);

#endif
