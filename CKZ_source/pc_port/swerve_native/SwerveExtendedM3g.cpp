// Swerve's extended M3G files ("\xbbSWERVE\xab" identifier) for m3gcore.
//
// The game ships 50 such files. Their sections and objects are standard M3G 1.0 except one private
// object type 25: a compressed (PVRTC) texture image. m3gcore only reads standard files, so the file
// is rewritten before decoding: the standard identifier, every type 25 object decoded into a plain
// RGB / RGBA Image2D (type 10), all sections stored uncompressed with fresh Adler32 checksums and the
// header's total file size updated. Object order is kept, so references by index stay valid.
// Desktop GL has no PVRTC either: the emulated backend decodes the same data with the same decoder.
#include "NativeRuntime.h"
#include "PVRTDecompress.h"
#include <zlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

namespace ckz::native {
namespace {

const uint8_t SwerveIdentifier[12] = { 0xbb, 0x53, 0x57, 0x45, 0x52, 0x56, 0x45, 0xab, 0x0d, 0x0a, 0x1a, 0x0a };
const uint8_t M3gIdentifier[12] = { 0xab, 0x4a, 0x53, 0x52, 0x31, 0x38, 0x34, 0xbb, 0x0d, 0x0a, 0x1a, 0x0a };
constexpr size_t IdentifierSize = 12;

constexpr uint8_t ObjectHeader = 0;
constexpr uint8_t ObjectImage2D = 10;
constexpr uint8_t ObjectCompressedImage = 25; // Swerve private
// Header object: version (2 bytes), hasExternalReferences (1), TotalFileSize (4), ...
constexpr size_t HeaderTotalSizeOffset = 3;

// GL formats of the compressed images (GL_IMG_texture_compression_pvrtc)
constexpr uint32_t RgbPvrtc4 = 0x8c00;
constexpr uint32_t RgbPvrtc2 = 0x8c01;
constexpr uint32_t RgbaPvrtc4 = 0x8c02;
constexpr uint32_t RgbaPvrtc2 = 0x8c03;
constexpr uint8_t FormatRGB = 0x63;
constexpr uint8_t FormatRGBA = 0x64;

uint32_t ReadU32(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0]) | static_cast<uint32_t>(data[1]) << 8 |
        static_cast<uint32_t>(data[2]) << 16 | static_cast<uint32_t>(data[3]) << 24;
}

void AppendU32(std::vector<uint8_t>& out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 24));
}

void WriteU32(uint8_t* data, uint32_t value)
{
    data[0] = static_cast<uint8_t>(value);
    data[1] = static_cast<uint8_t>(value >> 8);
    data[2] = static_cast<uint8_t>(value >> 16);
    data[3] = static_cast<uint8_t>(value >> 24);
}

/** Inflate a zlib section payload of known size. */
bool Inflate(const uint8_t* source, size_t sourceSize, std::vector<uint8_t>& out, size_t size)
{
    out.assign(size, 0);
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = const_cast<Bytef*>(source);
    stream.avail_in = static_cast<uInt>(sourceSize);
    stream.next_out = out.data();
    stream.avail_out = static_cast<uInt>(size);
    if (inflateInit(&stream) != Z_OK) {
        return false;
    }
    const int status = inflate(&stream, Z_FINISH);
    const bool complete = status == Z_STREAM_END && stream.total_out == size;
    inflateEnd(&stream);
    return complete;
}

/**
 * Length of the Object3D part at the start of an object: userID, animation track references and
 * user parameters (id and a byte array each). Returns 0 when it runs past `size`.
 */
size_t Object3DLength(const uint8_t* data, size_t size)
{
    size_t offset = 4; // userID
    if (offset + 4 > size) {
        return 0;
    }
    const uint32_t trackCount = ReadU32(data + offset);
    offset += 4 + static_cast<size_t>(trackCount) * 4;
    if (offset + 4 > size) {
        return 0;
    }
    const uint32_t parameterCount = ReadU32(data + offset);
    offset += 4;
    for (uint32_t parameter = 0; parameter < parameterCount; parameter++) {
        if (offset + 8 > size) {
            return 0;
        }
        const uint32_t length = ReadU32(data + offset + 4);
        offset += 8 + length;
    }
    if (offset > size) {
        return 0;
    }
    return offset;
}

/** Rewrite a type 25 object as a standard Image2D object body (decoded level 0). */
bool ConvertCompressedImage(const uint8_t* data, size_t size, std::vector<uint8_t>& image)
{
    const size_t object3D = Object3DLength(data, size);
    // glFormat (4), flag (1), width (4), height (4), first level size (4)
    if (object3D == 0 || object3D + 17 > size) {
        return false;
    }
    const uint8_t* fields = data + object3D;
    const uint32_t glFormat = ReadU32(fields);
    const uint32_t width = ReadU32(fields + 5);
    const uint32_t height = ReadU32(fields + 9);
    const uint32_t levelSize = ReadU32(fields + 13);
    const uint8_t* level = fields + 17;
    if (level + levelSize > data + size || width == 0 || height == 0) {
        return false;
    }
    bool twoBits = false;
    bool alpha = false;
    if (glFormat == RgbPvrtc2 || glFormat == RgbaPvrtc2) {
        twoBits = true;
    }
    if (glFormat == RgbaPvrtc4 || glFormat == RgbaPvrtc2) {
        alpha = true;
    }
    if (glFormat != RgbPvrtc4 && glFormat != RgbPvrtc2 && glFormat != RgbaPvrtc4 && glFormat != RgbaPvrtc2) {
        fprintf(stderr, "Swerve native: compressed image format 0x%x is not supported\n", glFormat);
        return false;
    }
    std::vector<uint8_t> rgba(static_cast<size_t>(width) * height * 4);
    pvr::PVRTDecompressPVRTC(level, twoBits, static_cast<int>(width), static_cast<int>(height), rgba.data());

    image.assign(data, data + object3D);
    std::vector<uint8_t> pixels;
    if (alpha) {
        image.push_back(FormatRGBA);
        pixels = rgba;
    } else {
        image.push_back(FormatRGB);
        pixels.reserve(static_cast<size_t>(width) * height * 3);
        for (size_t pixel = 0; pixel < static_cast<size_t>(width) * height; pixel++) {
            pixels.push_back(rgba[pixel * 4]);
            pixels.push_back(rgba[pixel * 4 + 1]);
            pixels.push_back(rgba[pixel * 4 + 2]);
        }
    }
    image.push_back(0); // isMutable = false
    AppendU32(image, width);
    AppendU32(image, height);
    AppendU32(image, 0); // no palette
    AppendU32(image, static_cast<uint32_t>(pixels.size()));
    image.insert(image.end(), pixels.begin(), pixels.end());
    return true;
}

} // namespace

bool IsSwerveExtendedM3g(const std::vector<uint8_t>& data)
{
    return data.size() >= IdentifierSize && memcmp(data.data(), SwerveIdentifier, IdentifierSize) == 0;
}

bool ConvertSwerveExtendedM3g(const std::vector<uint8_t>& data, std::vector<uint8_t>& out)
{
    out.assign(M3gIdentifier, M3gIdentifier + IdentifierSize);
    size_t headerSizeField = 0; // position in `out` of the header's TotalFileSize
    size_t position = IdentifierSize;
    int converted = 0;
    while (position + 13 <= data.size()) {
        const uint8_t compression = data[position];
        const uint32_t total = ReadU32(data.data() + position + 1);
        const uint32_t uncompressed = ReadU32(data.data() + position + 5);
        if (compression > 1 || total < 13 || position + total > data.size()) {
            fprintf(stderr, "Swerve native: bad section at %zu of an extended M3G file\n", position);
            return false;
        }
        const uint8_t* payload = data.data() + position + 9;
        const size_t payloadSize = total - 13;
        std::vector<uint8_t> objects;
        if (compression == 1) {
            if (!Inflate(payload, payloadSize, objects, uncompressed)) {
                fprintf(stderr, "Swerve native: could not inflate a section of an extended M3G file\n");
                return false;
            }
        } else {
            objects.assign(payload, payload + payloadSize);
        }

        // Rebuild the section uncompressed: compression, total, uncompressed, objects, checksum.
        const size_t sectionStart = out.size();
        out.push_back(0);
        AppendU32(out, 0);
        AppendU32(out, 0);
        size_t offset = 0;
        while (offset + 5 <= objects.size()) {
            const uint8_t type = objects[offset];
            const uint32_t length = ReadU32(objects.data() + offset + 1);
            const uint8_t* body = objects.data() + offset + 5;
            if (offset + 5 + length > objects.size()) {
                return false;
            }
            if (type == ObjectCompressedImage) {
                std::vector<uint8_t> image;
                if (!ConvertCompressedImage(body, length, image)) {
                    return false;
                }
                out.push_back(ObjectImage2D);
                AppendU32(out, static_cast<uint32_t>(image.size()));
                out.insert(out.end(), image.begin(), image.end());
                converted++;
            } else {
                out.push_back(type);
                AppendU32(out, length);
                if (type == ObjectHeader) {
                    headerSizeField = out.size() + HeaderTotalSizeOffset;
                }
                out.insert(out.end(), body, body + length);
            }
            offset += 5 + length;
        }
        const uint32_t sectionObjects = static_cast<uint32_t>(out.size() - sectionStart - 9);
        WriteU32(out.data() + sectionStart + 1, sectionObjects + 13);
        WriteU32(out.data() + sectionStart + 5, sectionObjects);
        AppendU32(out, 0); // checksum, filled in below once the file size is known
        position += total;
    }
    if (headerSizeField == 0) {
        return false;
    }
    WriteU32(out.data() + headerSizeField, static_cast<uint32_t>(out.size()));

    // Checksums last: the header section contains the file size.
    size_t section = IdentifierSize;
    while (section < out.size()) {
        const uint32_t total = ReadU32(out.data() + section + 1);
        const uLong checksum = adler32(adler32(0, nullptr, 0), out.data() + section, total - 4);
        WriteU32(out.data() + section + total - 4, static_cast<uint32_t>(checksum));
        section += total;
    }
    static int loggedFiles = 0;
    if (loggedFiles < 3) {
        loggedFiles++;
        fprintf(stderr, "Swerve native: extended M3G file converted (%d compressed images decoded)\n", converted);
    }
    return true;
}

} // namespace ckz::native
