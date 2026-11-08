// adpcm.cpp - IMA ADPCM decoder (mono or interleaved stereo).
#include "adpcm.h"

static const int stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767,
};

// Intel ADPCM step variation table
static const int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

void adpcm_decoder(signed char* indata, short* outdata, int len, int channels, adpcm_state* state)
{
    signed char* inp;               // input buffer pointer
    short* outp;                    // output buffer pointer
    int sign;                       // current adpcm sign bit
    int delta;                      // current adpcm code
    int step[2];                    // stepsize per channel
    int valpred[2];                 // predicted output value per channel
    int vpdiff;                     // current change to valpred
    int index[2];                   // current step change index per channel
    int inputbuffer;                // current input byte (two 4-bit codes)
    int bufferstep;                 // toggle between high and low nibble
    int channel;                    // channel of the current code (set before use)

    outp = outdata;
    inp = indata;

    valpred[0] = state->valprev[0];
    valpred[1] = state->valprev[1];
    index[0] = state->index[0];
    index[1] = state->index[1];
    step[0] = stepsizeTable[index[0]];
    step[1] = stepsizeTable[index[1]];

    bufferstep = 0;

    for (len = len * 2; len > 0; len--) {
        // Step 1 - get the delta value: high nibble is channel 0, low nibble channel 1 (stereo)
        if (bufferstep) {
            delta = inputbuffer & 0xf;
            if (channels == 2) {
                channel = 1;
            }
        } else {
            inputbuffer = *inp++;
            delta = (inputbuffer >> 4) & 0xf;
            channel = 0;
        }

        // Step 2 - find new index value (for later)
        index[channel] += indexTable[delta];
        if (index[channel] < 0) {
            index[channel] = 0;
        }
        if (index[channel] > 88) {
            index[channel] = 88;
        }

        // Step 3 - separate sign and magnitude
        sign = delta & 8;
        delta = delta & 7;

        // Step 4 - compute difference and new predicted value
        vpdiff = ((delta * step[channel]) >> 2) + (step[channel] >> 3);
        if (sign) {
            valpred[channel] -= vpdiff;
        } else {
            valpred[channel] += vpdiff;
        }

        // Step 5 - update step value
        step[channel] = stepsizeTable[index[channel]];

        bufferstep = !bufferstep;

        // Step 6 - clamp output value
        if (valpred[channel] > 32767) {
            valpred[channel] = 32767;
        } else if (valpred[channel] < -32768) {
            valpred[channel] = -32768;
        }

        // Step 7 - output value
        *outp++ = valpred[channel];
    }

    state->valprev[0] = valpred[0];
    state->valprev[1] = valpred[1];
    state->index[0] = index[0];
    state->index[1] = index[1];
}
