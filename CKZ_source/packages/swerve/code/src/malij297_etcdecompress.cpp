// malij297_etcdecompress.cpp - software ETC1 (Ericsson Texture Compression) decoder of the Swerve
// porting layer, used when the GPU cannot take ETC1 textures directly. The block decoder follows the
// Ericsson reference decoder (etcdec: decompressBlockDiffFlip) with 16 bit modifier tables.
// Output is RGB888 or RGB565; images whose size is not a multiple of 4 are decoded into a padded
// RGB work buffer first and cropped.

#include "SwerveApi.h"

// ---- constants ----
/** Output pixel format IMAGE_FORMAT_RGB565 of the Swerve runtime (anything else gives RGB888). */
static const int ETC_OUTPUT_RGB565 = 101;
/** Bytes of one decoded RGB pixel in the work image. */
static const int ETC_CHANNELS = 3;

/** Pixel index (MSB << 1 | LSB) to modifier column. */
static const unsigned char unscramble[4] = {2, 3, 1, 0};

/** Intensity modifiers per codeword table; each row appears twice (indexed with table << 1). */
static const short compressParams[16][4] = {
    {-8, -2, 2, 8}, {-8, -2, 2, 8},
    {-17, -5, 5, 17}, {-17, -5, 5, 17},
    {-29, -9, 9, 29}, {-29, -9, 9, 29},
    {-42, -13, 13, 42}, {-42, -13, 13, 42},
    {-60, -18, 18, 60}, {-60, -18, 18, 60},
    {-80, -24, 24, 80}, {-80, -24, 24, 80},
    {-106, -33, 33, 106}, {-106, -33, 33, 106},
    {-183, -47, 47, 183}, {-183, -47, 47, 183}
};

/** Clamp to [low, high]. */
#define CLAMP(low, x, high) (((x) < (low)) ? (low) : (((x) > (high)) ? (high) : (x)))
/** `size` bits of the 64 bit block ending at bit `startpos` (63..32 live in the high word). */
#define GETBITS(source, size, startpos) (((source) >> ((startpos) - (size) + 1)) & ((1 << (size)) - 1))
#define GETBITSHIGH(source, size, startpos) (((source) >> (((startpos) - 32) - (size) + 1)) & ((1 << (size)) - 1))
/** Channels of pixel (x, y) in an RGB image `width` pixels wide. */
#define RED_CHANNEL(img, width, x, y) img[ETC_CHANNELS * ((y) * (width) + (x)) + 0]
#define GREEN_CHANNEL(img, width, x, y) img[ETC_CHANNELS * ((y) * (width) + (x)) + 1]
#define BLUE_CHANNEL(img, width, x, y) img[ETC_CHANNELS * ((y) * (width) + (x)) + 2]

/**
 * Decode one 4x4 block (high word `blockPart1`, low word `blockPart2`) into the RGB image `img`
 * at (startX, startY). Handles individual / differential base colors and both flip orientations.
 */
static void decompressBlockDiffFlip(unsigned int blockPart1, unsigned int blockPart2, unsigned char* img, int width,
                                    int startX, int startY)
{
    unsigned char avgColor[3];
    unsigned char encColor2[3];
    signed char diff[3];
    int table;
    int index;
    int shift;
    int diffBit;
    int flipBit;
    unsigned int pixelIndicesMSB;
    unsigned int pixelIndicesLSB;

    diffBit = GETBITSHIGH(blockPart1, 1, 33);
    flipBit = GETBITSHIGH(blockPart1, 1, 32);

    if (!diffBit) {
        // Individual mode: two 4 bit base colors. First the left (top) half of the block.
        avgColor[0] = GETBITSHIGH(blockPart1, 4, 63);
        avgColor[1] = GETBITSHIGH(blockPart1, 4, 55);
        avgColor[2] = GETBITSHIGH(blockPart1, 4, 47);

        // Multiply by 17: copy the four bits into the upper nibble.
        avgColor[0] |= (avgColor[0] << 4);
        avgColor[1] |= (avgColor[1] << 4);
        avgColor[2] |= (avgColor[2] << 4);

        table = GETBITSHIGH(blockPart1, 3, 39) << 1;

        pixelIndicesMSB = GETBITS(blockPart2, 16, 31);
        pixelIndicesLSB = GETBITS(blockPart2, 16, 15);

        if (flipBit == 0) {
            // Not flipped: 2x4 sub-block on the left.
            shift = 0;
            for (int x = startX; x < startX + 2; x++) {
                for (int y = startY; y < startY + 4; y++) {
                    index = ((pixelIndicesMSB >> shift) & 1) << 1;
                    index |= ((pixelIndicesLSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[2] + compressParams[table][index], 255);
                }
            }
        } else {
            // Flipped: 4x2 sub-block on top.
            shift = 0;
            for (int x = startX; x < startX + 4; x++) {
                for (int y = startY; y < startY + 2; y++) {
                    index = ((pixelIndicesMSB >> shift) & 1) << 1;
                    index |= ((pixelIndicesLSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[2] + compressParams[table][index], 255);
                }
                shift += 2;
            }
        }

        // Now the other half of the block.
        avgColor[0] = GETBITSHIGH(blockPart1, 4, 59);
        avgColor[1] = GETBITSHIGH(blockPart1, 4, 51);
        avgColor[2] = GETBITSHIGH(blockPart1, 4, 43);

        avgColor[0] |= (avgColor[0] << 4);
        avgColor[1] |= (avgColor[1] << 4);
        avgColor[2] |= (avgColor[2] << 4);

        table = GETBITSHIGH(blockPart1, 3, 36) << 1;
        pixelIndicesMSB = GETBITS(blockPart2, 16, 31);
        pixelIndicesLSB = GETBITS(blockPart2, 16, 15);

        if (flipBit == 0) {
            shift = 8;
            for (int x = startX + 2; x < startX + 4; x++) {
                for (int y = startY; y < startY + 4; y++) {
                    index = ((pixelIndicesMSB >> shift) & 1) << 1;
                    index |= ((pixelIndicesLSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[2] + compressParams[table][index], 255);
                }
            }
        } else {
            shift = 2;
            for (int x = startX; x < startX + 4; x++) {
                for (int y = startY + 2; y < startY + 4; y++) {
                    index = ((pixelIndicesMSB >> shift) & 1) << 1;
                    index |= ((pixelIndicesLSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[2] + compressParams[table][index], 255);
                }
                shift += 2;
            }
        }
    } else {
        // Differential mode: 5 bit base color plus a signed 3 bit difference for the second half.
        avgColor[0] = GETBITSHIGH(blockPart1, 5, 63);
        avgColor[1] = GETBITSHIGH(blockPart1, 5, 55);
        avgColor[2] = GETBITSHIGH(blockPart1, 5, 47);
        diff[0] = GETBITSHIGH(blockPart1, 3, 58);
        diff[1] = GETBITSHIGH(blockPart1, 3, 50);
        diff[2] = GETBITSHIGH(blockPart1, 3, 42);

        // Extend the sign bit to the entire byte.
        diff[0] = (diff[0] << 5);
        diff[1] = (diff[1] << 5);
        diff[2] = (diff[2] << 5);
        diff[0] = diff[0] >> 5;
        diff[1] = diff[1] >> 5;
        diff[2] = diff[2] >> 5;

        // Second color.
        encColor2[0] = avgColor[0] + diff[0];
        encColor2[1] = avgColor[1] + diff[1];
        encColor2[2] = avgColor[2] + diff[2];

        // Extend both colors to 8 bits.
        avgColor[0] = (avgColor[0] << 3) | (avgColor[0] >> 2);
        avgColor[1] = (avgColor[1] << 3) | (avgColor[1] >> 2);
        avgColor[2] = (avgColor[2] << 3) | (avgColor[2] >> 2);
        encColor2[0] = (encColor2[0] << 3) | (encColor2[0] >> 2);
        encColor2[1] = (encColor2[1] << 3) | (encColor2[1] >> 2);
        encColor2[2] = (encColor2[2] << 3) | (encColor2[2] >> 2);

        // First half of the block.
        table = GETBITSHIGH(blockPart1, 3, 39) << 1;

        pixelIndicesMSB = GETBITS(blockPart2, 16, 31);
        pixelIndicesLSB = GETBITS(blockPart2, 16, 15);

        if (flipBit == 0) {
            shift = 0;
            for (int x = startX; x < startX + 2; x++) {
                for (int y = startY; y < startY + 4; y++) {
                    index = ((pixelIndicesMSB >> shift) & 1) << 1;
                    index |= ((pixelIndicesLSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[2] + compressParams[table][index], 255);
                }
            }
        } else {
            shift = 0;
            for (int x = startX; x < startX + 4; x++) {
                for (int y = startY; y < startY + 2; y++) {
                    index = ((pixelIndicesMSB >> shift) & 1) << 1;
                    index |= ((pixelIndicesLSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y) = CLAMP(0, avgColor[2] + compressParams[table][index], 255);
                }
                shift += 2;
            }
        }

        // Second half of the block.
        table = GETBITSHIGH(blockPart1, 3, 36) << 1;
        pixelIndicesMSB = GETBITS(blockPart2, 16, 31);
        pixelIndicesLSB = GETBITS(blockPart2, 16, 15);

        if (flipBit == 0) {
            shift = 8;
            for (int x = startX + 2; x < startX + 4; x++) {
                for (int y = startY; y < startY + 4; y++) {
                    index = ((pixelIndicesMSB >> shift) & 1) << 1;
                    index |= ((pixelIndicesLSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y) = CLAMP(0, encColor2[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y) = CLAMP(0, encColor2[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y) = CLAMP(0, encColor2[2] + compressParams[table][index], 255);
                }
            }
        } else {
            shift = 2;
            for (int x = startX; x < startX + 4; x++) {
                for (int y = startY + 2; y < startY + 4; y++) {
                    index = ((pixelIndicesMSB >> shift) & 1) << 1;
                    index |= ((pixelIndicesLSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y) = CLAMP(0, encColor2[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y) = CLAMP(0, encColor2[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y) = CLAMP(0, encColor2[2] + compressParams[table][index], 255);
                }
                shift += 2;
            }
        }
    }
}

extern "C" {

/**
 * Decode a `width` x `height` ETC1 image from `data` into `output` (RGB888, or RGB565 when
 * `format` is ETC_OUTPUT_RGB565). Returns 0 when the work buffer cannot be allocated, else 1.
 */
int malij297_DecompressRGB_ETC(int width, int height, const unsigned char* data, unsigned char* output, int format)
{
    int blocksX = (width + 3) / 4;
    int paddedWidth = blocksX * 4;
    int blocksY = (height + 3) / 4;
    int paddedHeight = blocksY * 4;

    // Decode straight into the output when no padding or conversion is needed.
    unsigned char* rgb = output;
    if (paddedWidth != width || paddedHeight != height || format == ETC_OUTPUT_RGB565) {
        rgb = (unsigned char*)malij297_Allocate(paddedWidth * paddedHeight * ETC_CHANNELS);
        if (rgb == NULL) {
            return 0;
        }
    }

    // Blocks are stored row by row, 8 big-endian bytes each.
    const unsigned char* block = data;
    for (int blockY = 0; blockY < blocksY; blockY++) {
        for (int blockX = 0; blockX < blocksX; blockX++) {
            unsigned int blockPart1 = block[0];
            blockPart1 = (blockPart1 << 8) | block[1];
            blockPart1 = (blockPart1 << 8) | block[2];
            blockPart1 = (blockPart1 << 8) | block[3];
            unsigned int blockPart2 = block[4];
            blockPart2 = (blockPart2 << 8) | block[5];
            blockPart2 = (blockPart2 << 8) | block[6];
            blockPart2 = (blockPart2 << 8) | block[7];
            decompressBlockDiffFlip(blockPart1, blockPart2, rgb, paddedWidth, blockX * 4, blockY * 4);
            block += 8;
        }
    }

    if (rgb != output) {
        if (format != ETC_OUTPUT_RGB565) {
            // Crop the padded RGB image.
            for (int y = 0; y < height; y++) {
                const unsigned char* source = rgb + y * paddedWidth * ETC_CHANNELS;
                unsigned char* destination = output + y * width * ETC_CHANNELS;
                for (int x = 0; x < width; x++) {
                    destination[x * ETC_CHANNELS + 0] = source[x * ETC_CHANNELS + 0];
                    destination[x * ETC_CHANNELS + 1] = source[x * ETC_CHANNELS + 1];
                    destination[x * ETC_CHANNELS + 2] = source[x * ETC_CHANNELS + 2];
                }
            }
        } else {
            // Convert to RGB565.
            unsigned short* destination = (unsigned short*)output;
            for (int y = 0; y < height; y++) {
                const unsigned char* source = rgb + y * paddedWidth * ETC_CHANNELS;
                for (int x = 0; x < width; x++) {
                    destination[x] = ((source[0] << 8) & 0xf800) | ((source[1] << 3) & 0x07e0) | (source[2] >> 3);
                    source += ETC_CHANNELS;
                }
                destination += width;
            }
        }
        malij297_Free(rgb);
    }
    return 1;
}

}
