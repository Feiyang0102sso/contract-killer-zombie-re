// malij297_glpopulatevbo.cpp - packs the vertex arrays of a Swerve (M3G) vertex buffer into one
// interleaved OpenGL ES vertex buffer object and describes where each attribute landed.
// The runtime reaches PopulateVBO_Contiguous through the environment table (malij297_glruntime.cpp).
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "SwerveApi.h"

// ---- constants ----
/** Component type in the low nibble of an array format word. */
static const int COMPONENT_TYPE_MASK = 0xf;
static const int COMPONENT_TYPE_BYTE = 1;
static const int COMPONENT_TYPE_SHORT = 2;
static const int COMPONENT_TYPE_FIXED = 3;      // 16.16 fixed point
static const int COMPONENT_TYPE_FLOAT = 4;
static const int COMPONENT_TYPE_HALF = 5;       // 16 bit float, expanded to float
/** Format bit: the components are signed (GL_BYTE / GL_SHORT instead of the unsigned types). */
static const int COMPONENT_SIGNED_FLAG = 0x10;
/** Color formats: component count in bits 8..15 plus the component type (signed flag removed). */
static const int COLOR_BYTE_RGB = 0x301;
static const int COLOR_FIXED_RGB = 0x303;
static const int COLOR_FLOAT_RGB = 0x304;
static const int COLOR_HALF_RGB = 0x305;
static const int COLOR_BYTE_RGBA = 0x401;
static const int COLOR_FIXED_RGBA = 0x403;
static const int COLOR_FLOAT_RGBA = 0x404;
static const int COLOR_HALF_RGBA = 0x405;
/** Offset stored for attributes the vertex buffer does not have. */
static const int ATTRIBUTE_ABSENT = -1;

static void packTypedData(unsigned char** cursor, unsigned char* source, malij297_VBOArrayFormat* arrayFormat,
                          int format, int components, int count);

/** Bytes of one component of an array format; -1 for unknown types. */
static inline int ComponentSize(int format)
{
    switch (format & COMPONENT_TYPE_MASK) {
    case COMPONENT_TYPE_BYTE:
        return 1;
    case COMPONENT_TYPE_SHORT:
        return 2;
    case COMPONENT_TYPE_FIXED:
        return 4;
    case COMPONENT_TYPE_FLOAT:
        return 4;
    case COMPONENT_TYPE_HALF:
        return 4;   // half floats are expanded to floats
    default:
        return -1;
    }
}

/** Component count of an array format. */
static inline int ComponentCount(int format)
{
    return (format >> 8) & 0xff;
}

/** Round a byte count up to a multiple of four. */
static inline int AlignTo4(int bytes)
{
    return (bytes + 3) / 4 * 4;
}

/** Expand a 16 bit float (zero only for +0; no denormal / infinity handling, as the original). */
static inline float HalfToFloat(short half)
{
    if (half == 0) {
        return 0.0f;
    }
    int mantissa = (half << 13) & 0x007fe000;
    int sign = (half << 16) & 0x80000000;
    int exponent = ((half << 13) & 0x0f800000) + 0x38000000;   // rebias 15 -> 127
    int bits = mantissa | sign | exponent;
    return *(float*)&bits;
}

/** 16.16 fixed point to float. */
static inline float FixedToFloat(int fixed)
{
    return (float)fixed * (1.0f / 65536.0f);
}

/** Float to 16.16 fixed point (rounded). */
static inline int FloatToFixed(float value)
{
    return (int)(value * 65536.0f + 0.5f);
}

/**
 * Pack all arrays of `data` into `buffer` (interleaved, every attribute 4 byte aligned) and fill
 * `format`. Returns the byte size of the VBO; with a NULL buffer only the size is computed.
 */
int malij297_PopulateVBO_Contiguous(const malij297_VertexBufferData* data, void* buffer, malij297_VBOFormat* format)
{
    int vertexSize = 0;
    int vertexCount = data->m_vertexCount;

    // Bytes of one vertex.
    if (data->m_positions.m_data != NULL) {
        vertexSize = AlignTo4(ComponentSize(data->m_positions.m_format) * ComponentCount(data->m_positions.m_format));
    }
    if (data->m_colors.m_data != NULL) {
        vertexSize += AlignTo4(ComponentSize(data->m_colors.m_format) * 4);
    }
    if (data->m_normals.m_data != NULL) {
        vertexSize += AlignTo4(ComponentSize(data->m_normals.m_format) * ComponentCount(data->m_normals.m_format));
    }
    // The original counts one component for the next three arrays (and four bytes per texture
    // coordinate) whatever their formats say; kept as is.
    if (data->m_pointSizes.m_data != NULL) {
        vertexSize += AlignTo4(ComponentSize(data->m_pointSizes.m_format));
    }
    if (data->m_matrixIndices.m_data != NULL) {
        vertexSize += AlignTo4(ComponentSize(data->m_matrixIndices.m_format));
    }
    if (data->m_weights.m_data != NULL) {
        vertexSize += AlignTo4(ComponentSize(data->m_weights.m_format));
    }
    for (int index = 0; index < data->m_texCoordCount; index++) {
        if (data->m_texCoords[index].m_data != NULL) {
            vertexSize += ComponentCount(data->m_texCoords[index].m_format) * 4;
        }
    }

    if (buffer != NULL) {
        unsigned char* start = (unsigned char*)buffer;
        unsigned char* cursor = start;
        int offset;

        // Positions come first, unaligned.
        if (data->m_positions.m_data != NULL) {
            int components = ComponentCount(data->m_positions.m_format);
            format->m_positions.m_offset = 0;
            packTypedData(&cursor, data->m_positions.m_data, &format->m_positions, data->m_positions.m_format,
                          components, vertexCount);
        } else {
            format->m_positions.m_offset = ATTRIBUTE_ABSENT;
        }

        // Colors always become four components; RGB gets m_alpha, RGBA alpha is scaled by m_alpha.
        if (data->m_colors.m_data != NULL) {
            format->m_colors.m_components = 4;
            offset = AlignTo4(cursor - start);
            cursor = start + offset;
            format->m_colors.m_offset = offset;
            switch (data->m_colors.m_format & ~COMPONENT_SIGNED_FLAG) {
            case COLOR_BYTE_RGB: {
                float alpha = data->m_alpha * 255.0f;
                unsigned char* source = data->m_colors.m_data;
                format->m_colors.m_size = 4;
                format->m_colors.m_type = GL_UNSIGNED_BYTE;
                for (int vertex = 0; vertex < vertexCount; vertex++) {
                    cursor[0] = source[0];
                    cursor[1] = source[1];
                    cursor[2] = source[2];
                    cursor[3] = (int)alpha;
                    source += 3;
                    cursor += format->m_colors.m_size;
                }
                break;
            }
            case COLOR_BYTE_RGBA: {
                unsigned char* source = data->m_colors.m_data;
                format->m_colors.m_size = 4;
                format->m_colors.m_type = GL_UNSIGNED_BYTE;
                for (int vertex = 0; vertex < vertexCount; vertex++) {
                    cursor[0] = source[0];
                    cursor[1] = source[1];
                    cursor[2] = source[2];
                    cursor[3] = (int)((float)(int)source[3] * data->m_alpha);
                    source += 4;
                    cursor += format->m_colors.m_size;
                }
                break;
            }
            case COLOR_FLOAT_RGB: {
                float* source = (float*)data->m_colors.m_data;
                float alpha = data->m_alpha;
                format->m_colors.m_size = 16;
                format->m_colors.m_type = GL_FLOAT;
                for (int vertex = 0; vertex < vertexCount; vertex++) {
                    float* destination = (float*)cursor;
                    destination[0] = source[0];
                    destination[1] = source[1];
                    destination[2] = source[2];
                    destination[3] = alpha;
                    source += 3;
                    cursor += format->m_colors.m_size;
                }
                break;
            }
            case COLOR_FLOAT_RGBA: {
                float* source = (float*)data->m_colors.m_data;
                float alpha = data->m_alpha;
                format->m_colors.m_size = 16;
                format->m_colors.m_type = GL_FLOAT;
                for (int vertex = 0; vertex < vertexCount; vertex++) {
                    float* destination = (float*)cursor;
                    destination[0] = source[0];
                    destination[1] = source[1];
                    destination[2] = source[2];
                    destination[3] = source[3] * alpha;
                    source += 4;
                    cursor += format->m_colors.m_size;
                }
                break;
            }
            case COLOR_FIXED_RGB: {
                int* source = (int*)data->m_colors.m_data;
                int alpha = FloatToFixed(data->m_alpha);
                format->m_colors.m_size = 16;
                format->m_colors.m_type = GL_FIXED;
                for (int vertex = 0; vertex < vertexCount; vertex++) {
                    int* destination = (int*)cursor;
                    destination[0] = source[0];
                    destination[1] = source[1];
                    destination[2] = source[2];
                    destination[3] = alpha;
                    source += 3;
                    cursor += format->m_colors.m_size;
                }
                break;
            }
            case COLOR_FIXED_RGBA: {
                int* source = (int*)data->m_colors.m_data;
                float alpha = data->m_alpha;
                format->m_colors.m_size = 16;
                format->m_colors.m_type = GL_FIXED;
                for (int vertex = 0; vertex < vertexCount; vertex++) {
                    int* destination = (int*)cursor;
                    destination[0] = source[0];
                    destination[1] = source[1];
                    destination[2] = source[2];
                    destination[3] = FloatToFixed(FixedToFloat(source[3]) * alpha);
                    source += 4;
                    cursor += format->m_colors.m_size;
                }
                break;
            }
            case COLOR_HALF_RGB: {
                unsigned short* source = (unsigned short*)data->m_colors.m_data;
                float alpha = data->m_alpha;
                format->m_colors.m_size = 16;
                format->m_colors.m_type = GL_FLOAT;
                for (int vertex = 0; vertex < vertexCount; vertex++) {
                    float* destination = (float*)cursor;
                    destination[0] = HalfToFloat(source[0]);
                    destination[1] = HalfToFloat(source[1]);
                    destination[2] = HalfToFloat(source[2]);
                    destination[3] = alpha;
                    source += 3;
                    cursor += format->m_colors.m_size;
                }
                break;
            }
            case COLOR_HALF_RGBA: {
                unsigned short* source = (unsigned short*)data->m_colors.m_data;
                float alpha = data->m_alpha;
                format->m_colors.m_size = 16;
                format->m_colors.m_type = GL_FLOAT;
                for (int vertex = 0; vertex < vertexCount; vertex++) {
                    float* destination = (float*)cursor;
                    destination[0] = HalfToFloat(source[0]);
                    destination[1] = HalfToFloat(source[1]);
                    destination[2] = HalfToFloat(source[2]);
                    destination[3] = HalfToFloat(source[3]) * alpha;
                    source += 4;
                    cursor += format->m_colors.m_size;
                }
                break;
            }
            }
        } else {
            format->m_colors.m_offset = ATTRIBUTE_ABSENT;
        }

        if (data->m_normals.m_data != NULL) {
            int components = ComponentCount(data->m_normals.m_format);
            offset = AlignTo4(cursor - start);
            cursor = start + offset;
            format->m_normals.m_offset = offset;
            packTypedData(&cursor, data->m_normals.m_data, &format->m_normals, data->m_normals.m_format, components,
                          vertexCount);
        } else {
            format->m_normals.m_offset = ATTRIBUTE_ABSENT;
        }

        if (data->m_pointSizes.m_data != NULL) {
            offset = AlignTo4(cursor - start);
            cursor = start + offset;
            format->m_pointSizes.m_offset = offset;
            packTypedData(&cursor, data->m_pointSizes.m_data, &format->m_pointSizes, data->m_pointSizes.m_format, 1,
                          vertexCount);
        } else {
            format->m_pointSizes.m_offset = ATTRIBUTE_ABSENT;
        }

        // Matrix indices are copied as unsigned bytes whatever the format type says.
        if (data->m_matrixIndices.m_data != NULL) {
            int components = ComponentCount(data->m_matrixIndices.m_format);
            offset = AlignTo4(cursor - start);
            cursor = start + offset;
            format->m_matrixIndices.m_offset = offset;
            format->m_matrixIndices.m_size = components;
            format->m_matrixIndices.m_type = GL_UNSIGNED_BYTE;
            format->m_matrixIndices.m_components = components;
            malij297_MemCpy(cursor, data->m_matrixIndices.m_data, components * vertexCount);
            cursor += components * vertexCount;
        } else {
            format->m_matrixIndices.m_offset = ATTRIBUTE_ABSENT;
        }

        if (data->m_weights.m_data != NULL) {
            int components = ComponentCount(data->m_weights.m_format);
            offset = AlignTo4(cursor - start);
            cursor = start + offset;
            format->m_weights.m_offset = offset;
            packTypedData(&cursor, data->m_weights.m_data, &format->m_weights, data->m_weights.m_format, components,
                          vertexCount);
        } else {
            format->m_weights.m_offset = ATTRIBUTE_ABSENT;
        }

        for (int index = 0; index < data->m_texCoordCount; index++) {
            if (data->m_texCoords[index].m_data != NULL) {
                int texCoordFormat = data->m_texCoords[index].m_format;
                offset = AlignTo4(cursor - start);
                cursor = start + offset;
                format->m_texCoords[index].m_offset = offset;
                packTypedData(&cursor, data->m_texCoords[index].m_data, &format->m_texCoords[index],
                              data->m_texCoords[index].m_format, ComponentCount(texCoordFormat), vertexCount);
            } else {
                format->m_texCoords[index].m_offset = ATTRIBUTE_ABSENT;
            }
        }
    }
    return vertexSize * vertexCount;
}

/**
 * Copy `count` vertices of `components` values from `source` to `*cursor`, advance the cursor and
 * describe the attribute in `arrayFormat`. Half floats are expanded to floats; unknown types are skipped.
 */
static void packTypedData(unsigned char** cursor, unsigned char* source, malij297_VBOArrayFormat* arrayFormat,
                          int format, int components, int count)
{
    switch (format & COMPONENT_TYPE_MASK) {
    case COMPONENT_TYPE_BYTE: {
        int vertexBytes = components;
        arrayFormat->m_size = vertexBytes;
        arrayFormat->m_type = GL_UNSIGNED_BYTE ^ ((format >> 4) & 1);
        arrayFormat->m_components = components;
        malij297_MemCpy(*cursor, source, vertexBytes * count);
        *cursor += vertexBytes * count;
        break;
    }
    case COMPONENT_TYPE_SHORT: {
        int vertexBytes = components * 2;
        arrayFormat->m_size = vertexBytes;
        arrayFormat->m_type = GL_UNSIGNED_SHORT ^ ((format >> 4) & 1);
        arrayFormat->m_components = components;
        malij297_MemCpy(*cursor, source, vertexBytes * count);
        *cursor += vertexBytes * count;
        break;
    }
    case COMPONENT_TYPE_FLOAT: {
        int vertexBytes = components * 4;
        arrayFormat->m_size = vertexBytes;
        arrayFormat->m_type = GL_FLOAT;
        arrayFormat->m_components = components;
        malij297_MemCpy(*cursor, source, vertexBytes * count);
        *cursor += vertexBytes * count;
        break;
    }
    case COMPONENT_TYPE_FIXED: {
        int vertexBytes = components * 4;
        arrayFormat->m_size = vertexBytes;
        arrayFormat->m_type = GL_FIXED;
        arrayFormat->m_components = components;
        malij297_MemCpy(*cursor, source, vertexBytes * count);
        *cursor += vertexBytes * count;
        break;
    }
    case COMPONENT_TYPE_HALF: {
        int vertexBytes = components * 4;
        unsigned short* halves = (unsigned short*)source;
        arrayFormat->m_size = vertexBytes;
        arrayFormat->m_type = GL_FLOAT;
        arrayFormat->m_components = components;
        for (int vertex = 0; vertex < count; vertex++) {
            float* destination = (float*)*cursor;
            for (int component = 0; component < components; component++) {
                destination[component] = HalfToFloat(halves[component]);
            }
            halves += components;
            *cursor += vertexBytes;
        }
        break;
    }
    }
}
