// MathLib.cpp - fast float math (polynomial sin/cos/asin/atan, bit-trick exp/log), vectors, planes,
// angles, quaternions and 4x4 matrices. Angles are in degrees unless stated otherwise.
#include "MathLib.h"

// ---- constants ----
#define DEG_TO_RAD      0.0174532924f
#define RAD_TO_DEG      57.2957802f
#define HALF_PI         1.57079637f
#define PI_VALUE        3.14159274f
#define SQRT_2          1.41421354f
#define LOG2_E          1.44269502f
#define ATAN_EPSILON    1e-06f
#define RAY_EPSILON     0.001f
#define BARY_EPSILON    0.001
#define DOUBLE_MAX      1.7976931348623157e+308
#define SQRT_HALF       0.70710678118654757
#define LN2_HIGH        0.693359375
#define LN2_LOW         -0.00021219444005469057

/** IEEE double split into its fields (low word first on ARM). */
union DoubleBits {
    double value;
    struct {
        unsigned int mantissaLow : 32;
        unsigned int mantissaHigh : 20;
        unsigned int exponent : 11;
        unsigned int sign : 1;
    } parts;
};

float MathLib::InvSqrt(float x)
{
    float negativeHalf = x * -0.5f;
    int bits = *(int*)&x;
    bits = 0x5f3759df - (bits >> 1);
    float root = *(float*)&bits;
    root = root * (root * root * negativeHalf + 1.5f);
    return root;
}

float MathLib::NormAngle(float degrees)
{
    // sign bit test on the raw bits
    if (*(int*)&degrees < 0) {
        return degrees + 360.0f - (float)((int)(degrees / 360.0f) * 360);
    }
    if (degrees >= 360.0f) {
        return degrees - (float)((int)(degrees / 360.0f) * 360);
    }
    return degrees;
}

float MathLib::Sin(float degrees)
{
    float angle = NormAngle(degrees);
    // fold into [-90, 90]
    if (angle < 180.0f) {
        if (angle > 90.0f) {
            angle = 180.0f - angle;
        }
    } else if (angle > 270.0f) {
        angle = angle - 360.0f;
    } else {
        angle = 180.0f - angle;
    }
    angle = angle * DEG_TO_RAD;
    float square = angle * angle;
    float result = -2.39e-08f;
    result = result * square + 2.7526e-06f;
    result = result * square - 1.98409e-04f;
    result = result * square + 8.3333315e-03f;
    result = result * square - 1.666666664e-01f;
    result = result * square + 1.0f;
    return angle * result;
}

float MathLib::Cos(float degrees)
{
    float angle = NormAngle(degrees);
    float sign;
    // fold into [-90, 90]; the cosine changes sign in (90, 270]
    if (angle < 180.0f) {
        if (angle > 90.0f) {
            angle = 180.0f - angle;
            sign = -1.0f;
        } else {
            sign = 1.0f;
        }
    } else if (angle > 270.0f) {
        angle = angle - 360.0f;
        sign = 1.0f;
    } else {
        angle = 180.0f - angle;
        sign = -1.0f;
    }
    angle = angle * DEG_TO_RAD;
    float square = angle * angle;
    float result = -2.605e-07f;
    result = result * square + 2.47609e-05f;
    result = result * square - 1.3888397e-03f;
    result = result * square + 4.16666418e-02f;
    result = result * square - 0.5f;
    result = result * square + 1.0f;
    return sign * result;
}

void MathLib::SinCos(float degrees, float& sine, float& cosine)
{
    float angle = NormAngle(degrees);
    float sign;
    if (angle < 180.0f) {
        if (angle > 90.0f) {
            angle = 180.0f - angle;
            sign = -1.0f;
        } else {
            sign = 1.0f;
        }
    } else if (angle > 270.0f) {
        angle = angle - 360.0f;
        sign = 1.0f;
    } else {
        angle = 180.0f - angle;
        sign = -1.0f;
    }
    angle = angle * DEG_TO_RAD;
    float square = angle * angle;

    float sinResult = -2.39e-08f;
    sinResult = sinResult * square + 2.7526e-06f;
    sinResult = sinResult * square - 1.98409e-04f;
    sinResult = sinResult * square + 8.3333315e-03f;
    sinResult = sinResult * square - 1.666666664e-01f;
    sinResult = sinResult * square + 1.0f;

    float cosResult = -2.605e-07f;
    cosResult = cosResult * square + 2.47609e-05f;
    cosResult = cosResult * square - 1.3888397e-03f;
    cosResult = cosResult * square + 4.16666418e-02f;
    cosResult = cosResult * square - 0.5f;
    cosResult = cosResult * square + 1.0f;

    sine = angle * sinResult;
    cosine = sign * cosResult;
}

float MathLib::ArcSin(float x)
{
    float radians;
    if (*(int*)&x < 0) {
        if (x <= -1.0f) {
            return -90.0f;
        }
        x = -x;
        radians = (((-0.0187293f * x + 0.0742610f) * x - 0.2121144f) * x + 1.5707288f) * Sqrt(1.0f - x) - HALF_PI;
        return radians * RAD_TO_DEG;
    }
    if (x >= 1.0f) {
        return 90.0f;
    }
    radians = HALF_PI - (((-0.0187293f * x + 0.0742610f) * x - 0.2121144f) * x + 1.5707288f) * Sqrt(1.0f - x);
    return radians * RAD_TO_DEG;
}

float MathLib::ArcTan(float y, float x)
{
    float absX = Fabs(x);
    float absY = Fabs(y);
    if (absX < ATAN_EPSILON) {
        if (absY < ATAN_EPSILON) {
            return 0.0f;
        }
        // ?: gives the original constant table load
        return (y > 0.0f) ? 90.0f : -90.0f;
    }
    if (absY < ATAN_EPSILON) {
        return (x > 0.0f) ? 0.0f : 180.0f;
    }

    float radians;
    if (absX > absY) {
        float ratio = absY / absX;
        radians = ((0.010413f - (0.411333f - (-0.028779f * ratio + 0.21537f) * ratio) * ratio) * ratio + 1.0f) * ratio;
        if (x < 0.0f) {
            radians = PI_VALUE - radians;
        }
    } else {
        float ratio = absX / absY;
        radians = ((0.010413f - (0.411333f - (-0.028779f * ratio + 0.21537f) * ratio) * ratio) * ratio + 1.0f) * ratio;
        if (x >= 0.0f) {
            radians = HALF_PI - radians;
        } else {
            radians = HALF_PI + radians;
        }
    }
    radians = radians * RAD_TO_DEG;
    if (y >= 0.0f) {
        return radians;
    }
    return -radians;
}

float MathLib::Frexp(float x, int* exponent)
{
    DoubleBits bits;
    bits.value = x;
    int biasedExponent = bits.parts.exponent;
    int adjust;
    if (biasedExponent != 0) {
        adjust = 0;
        if (biasedExponent == 0x7ff) {
            // infinity or NaN
            *exponent = 0;
            return bits.value;
        }
    } else {
        // zero or subnormal
        if ((bits.parts.mantissaHigh | bits.parts.mantissaLow) == 0) {
            *exponent = 0;
            return bits.value;
        }
        bits.value = bits.value * 18446744073709551616.0;
        biasedExponent = bits.parts.exponent;
        adjust = 64;
    }
    *exponent = biasedExponent - 0x3fe - adjust;
    bits.parts.exponent = 0x3fe;
    return bits.value;
}

/** Split `value` into integer part (*integerPart) and fraction (returned). */
static double __modf(double value, double* integerPart);

float MathLib::Floor(float x)
{
    double integerPart;
    double fraction = __modf(x, &integerPart);
    float result = (float)integerPart;
    if (fraction < 0.0) {
        result = result - 1.0f;
    }
    return result;
}

// Dead-stripped in the shipped binary: name and body are a guess. A second caller is what keeps the
// static __modf out of line in the original (with Floor as its only caller it gets inlined).
float MathLib::Ceil(float x)
{
    double integerPart;
    double fraction = __modf(x, &integerPart);
    float result = (float)integerPart;
    if (fraction > 0.0) {
        result = result + 1.0f;
    }
    return result;
}

static double __modf(double value, double* integerPart)
{
    DoubleBits bits;
    bits.value = value;
    int exponent = bits.parts.exponent - 0x3ff;
    if (exponent < 0) {
        // |value| < 1
        *integerPart = 0.0;
        return value;
    }
    int fractionBits = 52 - exponent;
    if (fractionBits <= 0) {
        // no fraction bits left
        *integerPart = value;
        return 0.0;
    }
    if (fractionBits >= 32) {
        bits.parts.mantissaHigh = bits.parts.mantissaHigh & (0xffffffff << (fractionBits - 32));
        bits.parts.mantissaLow = 0;
    } else {
        bits.parts.mantissaLow = bits.parts.mantissaLow & (0xffffffff << fractionBits);
    }
    *integerPart = bits.value;
    return value - bits.value;
}

int MathLib::Round(float value)
{
    return (int)(value + 0.5f);
}

float MathLib::Pow(float base, float exponent)
{
    if (base == 0.0f) {
        return 0.0f;
    }
    if (exponent == 0.0f) {
        return 1.0f;
    }
    return Exp(Log(base) * exponent);
}

float MathLib::Exp(float x)
{
    // e^x = 2^(x * log2(e)) = 2^integer * 2^fraction
    float power = x * LOG2_E;
    int bits = *(int*)&power;
    int biasedExponent = (bits >> 23) & 0xff;
    int mantissa = (bits & 0x7fffff) | 0x800000;
    int truncated = (mantissa >> (150 - biasedExponent)) & ~((biasedExponent - 127) >> 31);
    int integer = truncated ^ (bits >> 31);
    int scaleBits = (integer << 23) + 0x3f800000;
    float scale = *(float*)&scaleBits;
    float fraction = power - (float)integer;
    if (fraction >= 0.5f) {
        fraction = fraction - 0.5f;
        scale = scale * SQRT_2;
    }
    float square = fraction * fraction;
    float odd = fraction * (square * 0.0576900728f + 7.21528912f);
    float even = square + 20.8189239f;
    return scale * (even + odd) / (even - odd);
}

/** Natural log (cephes log algorithm, double precision inside). */
float MathLib::Log(float x)
{
    if ((*(int*)&x & 0x7f800000) == 0x7f800000) {
        // infinity or NaN
        return x;
    }
    if (x <= 0.0f) {
        return -1.0f / 0.0f;
    }
    if (x <= DOUBLE_MAX) {
        int exponent;
        double mantissa = Frexp(x, &exponent);
        double z = mantissa - 0.5;
        double y;
        if (mantissa > SQRT_HALF) {
            z = z - 0.5;
            y = 0.5 * mantissa + 0.5;
        } else {
            exponent = exponent - 1;
            y = 0.5 * z + 0.5;
        }
        double ratio = z / y;
        double square = ratio * ratio;
        double numerator = (-0.78956112887491258 * square + 16.383943563021536) * square - 64.124943423745581;
        double denominator = ((square - 35.667977739034646) * square + 312.03222091924533) * square - 769.49932108494872;
        float result = ratio + ratio * square * (numerator / denominator);
        result = result + exponent * LN2_LOW;
        result = result + exponent * LN2_HIGH;
        return result;
    }
    return x;
}

int MathLib::CeilPowerOfTwo(int value)
{
    value = value - 1;
    value = value | (value >> 1);
    value = value | (value >> 2);
    value = value | (value >> 4);
    value = value | (value >> 8);
    value = value | (value >> 16);
    return value + 1;
}

int RandomValue::Int(int range)
{
    if (range == 0) {
        return 0;
    }
    m_seed = m_seed * 1664525 + 1013904223;
    return (int)(m_seed & 0x7fffffff) % range;
}

float RandomValue::Float()
{
    m_seed = m_seed * 1664525 + 1013904223;
    // random mantissa with exponent 0: [1, 2)
    unsigned int bits = (m_seed & 0x007fffff) | 0x3f800000;
    return *(float*)&bits - 1.0f;
}

float RandomValue::CFloat()
{
    m_seed = m_seed * 1664525 + 1013904223;
    unsigned int bits = (m_seed & 0x007fffff) | 0x3f800000;
    float value = *(float*)&bits;
    return value + value - 3.0f;
}

bool vec3::FixDegenerateNormal()
{
    if (m_x == 0.0f) {
        if (m_y == 0.0f) {
            if (m_z > 0.0f) {
                if (m_z != 1.0f) {
                    m_z = 1.0f;
                    return true;
                }
            } else {
                if (m_z != -1.0f) {
                    m_z = -1.0f;
                    return true;
                }
            }
            return false;
        } else if (m_z == 0.0f) {
            if (m_y > 0.0f) {
                if (m_y != 1.0f) {
                    m_y = 1.0f;
                    return true;
                }
            } else {
                if (m_y != -1.0f) {
                    m_y = -1.0f;
                    return true;
                }
            }
            return false;
        }
    } else if (m_y == 0.0f) {
        if (m_z == 0.0f) {
            if (m_x > 0.0f) {
                if (m_x != 1.0f) {
                    m_x = 1.0f;
                    return true;
                }
            } else {
                if (m_x != -1.0f) {
                    m_x = -1.0f;
                    return true;
                }
            }
            return false;
        }
    }
    if (MathLib::Fabs(m_x) == 1.0f) {
        if (m_y != 0.0f || m_z != 0.0f) {
            m_y = m_z = 0.0f;
            return true;
        }
        return false;
    } else if (MathLib::Fabs(m_y) == 1.0f) {
        if (m_x != 0.0f || m_z != 0.0f) {
            m_x = m_z = 0.0f;
            return true;
        }
        return false;
    } else if (MathLib::Fabs(m_z) == 1.0f) {
        if (m_x != 0.0f || m_y != 0.0f) {
            m_x = m_y = 0.0f;
            return true;
        }
        return false;
    }
    return false;
}

Plane::Plane(const vec3& a, const vec3& b, const vec3& c, bool fixNormal)
{
    Init(a, b, c, fixNormal);
}

void Plane::Init(const vec3& a, const vec3& b, const vec3& c, bool fixNormal)
{
    vec3 normal = (a - b).Cross(c - b);
    float inverseLength = MathLib::InvSqrt(normal.LengthSqr());
    normal = normal * inverseLength;
    if (fixNormal) {
        normal.FixDegenerateNormal();
    }
    m_normal = normal;
    m_distance = normal.Dot(b);
}

int Plane::PlaneAxeNum() const
{
    float absX = MathLib::Fabs(m_normal.m_x);
    float absY = MathLib::Fabs(m_normal.m_y);
    float absZ = MathLib::Fabs(m_normal.m_z);
    if (absX > absY) {
        if (absX > absZ) {
            return 0;
        }
        return 2;
    }
    if (absY > absZ) {
        return 1;
    }
    return 2;
}

vec3 Plane::Projection(const vec3& point) const
{
    vec3 result = point;
    float lengthSqr = m_normal.LengthSqr();
    if (lengthSqr != 0.0f) {
        float t = (m_distance - m_normal.Dot(point)) / lengthSqr;
        result = point + m_normal * t;
    }
    return result;
}

bool MathLib::RayTriangleIntersection(const vec3& origin, const vec3& direction, const Plane& plane,
                                      const vec3& a, const vec3& b, const vec3& c, vec3& hit, float& distance)
{
    // projection axes for the dominant normal axis
    static const int minorAxes[3][2] = { { 1, 2 }, { 0, 2 }, { 0, 1 } };

    vec3 edge1 = b - a;
    vec3 edge2 = c - a;
    vec3 offset = hit - a;
    int axis = plane.PlaneAxeNum();
    int first = minorAxes[axis][0];
    int second = minorAxes[axis][1];

    float u;
    float v;
    if (Fabs(edge2[second]) < RAY_EPSILON) {
        u = (offset[second] * edge2[first] - offset[first] * edge2[second]) /
            (edge1[second] * edge2[first] - edge1[first] * edge2[second]);
        v = (offset[first] - u * edge1[first]) / edge2[first];
    } else {
        u = (offset[first] * edge2[second] - offset[second] * edge2[first]) /
            (edge1[first] * edge2[second] - edge1[second] * edge2[first]);
        v = (offset[second] - u * edge1[second]) / edge2[second];
    }
    if (u >= -BARY_EPSILON && v >= -BARY_EPSILON && u + v >= -BARY_EPSILON && u + v <= 1.0 + BARY_EPSILON) {
        return true;
    }
    return false;
}

bool MathLib::RayTriangleIntersection(const vec3& origin, const vec3& direction, float length, const Plane& plane,
                                      const vec3& a, const vec3& b, const vec3& c, vec3& hit, float& distance,
                                      float frontDistance, float backDistance, float minDistance, float maxDistance)
{
    float denominator = plane.m_normal.Dot(direction);
    float originDistance = plane.Distance(origin);
    bool closeToPlane = false;
    if (originDistance <= frontDistance && originDistance >= -backDistance) {
        closeToPlane = true;
    } else if (originDistance >= minDistance && originDistance <= maxDistance) {
        closeToPlane = true;
    }

    if (closeToPlane) {
        if (Fabs(denominator) > RAY_EPSILON) {
            hit = origin - plane.m_normal * originDistance;
            distance = originDistance;
        } else {
            hit = origin + direction * length;
            hit = hit - plane.m_normal * plane.Distance(hit);
            distance = length;
        }
    } else {
        if (denominator == 0.0f) {
            return false;
        }
        float t = -originDistance / denominator;
        if (t < -RAY_EPSILON) {
            return false;
        }
        if (t > length + RAY_EPSILON) {
            return false;
        }
        hit = origin + direction * t;
        distance = t;
    }
    return RayTriangleIntersection(origin, direction, plane, a, b, c, hit, distance);
}

vec3 MathLib::DistanceToSegment(const vec3& point, const vec3& a, const vec3& b, bool& inside)
{
    vec3 toPoint = point - a;
    vec3 segment = b - a;
    inside = false;
    float projection = segment.Dot(toPoint);
    float lengthSqr = segment.LengthSqr();
    if (projection <= 0.0f) {
        return point - a;
    }
    if (lengthSqr <= projection) {
        return point - b;
    }
    float t = projection / lengthSqr;
    inside = true;
    return point - (a + segment * t);
}

vec3 MathLib::DistanceToLine(const vec3& point, const vec3& a, const vec3& b)
{
    vec3 line = b - a;
    vec3 toPoint = point - a;
    float t = line.Dot(toPoint) / line.LengthSqr();
    return point - (a + line * t);
}

Vectors::Vectors(const vec3& forward, const vec3& up)
{
    m_forward = forward;
    // a helper axis that is not parallel to forward
    vec3 helper;
    if (MathLib::Fabs(forward.Dot(up)) < 0.99f) {
        helper = up;
    } else {
        helper = vec3(0.0f, 1.0f, 0.0f);
    }
    m_right = forward.Cross(helper);
    m_right.Normalize();
    m_up = m_right.Cross(m_forward);
    m_up.Normalize();
}

Vectors& Angles::ToVectors(Vectors& vectors) const
{
    float sinPitch;
    float cosPitch;
    float sinYaw;
    float cosYaw;
    float sinRoll;
    float cosRoll;
    MathLib::SinCos(m_pitch, sinPitch, cosPitch);
    MathLib::SinCos(m_yaw, sinYaw, cosYaw);
    MathLib::SinCos(m_roll, sinRoll, cosRoll);
    vectors.m_forward = vec3(-(cosYaw * sinPitch), cosYaw * cosPitch, -sinYaw);
    vectors.m_right = vec3(sinYaw * sinRoll * sinPitch + cosRoll * cosPitch, cosRoll * sinPitch - sinYaw * sinRoll * cosPitch,
                           -(sinRoll * cosYaw));
    vectors.m_up = vec3(sinRoll * cosPitch - cosRoll * sinYaw * sinPitch, cosRoll * sinYaw * cosPitch + sinRoll * sinPitch,
                        cosRoll * cosYaw);
    vectors.m_forward.FixDegenerateNormal();
    vectors.m_right.FixDegenerateNormal();
    vectors.m_up.FixDegenerateNormal();
    return vectors;
}

/** NormAngle mapped to (-180, 180]. */
static inline float NormAngle180(float degrees)
{
    float angle = MathLib::NormAngle(degrees);
    if (angle > 180.0f) {
        angle = angle - 360.0f;
    }
    return angle;
}

Angles& Vectors::ToAngles(Angles& angles) const
{
    float z = m_forward.m_z;
    if (z > 1.0f) {
        z = 1.0f;
    } else if (z < -1.0f) {
        z = -1.0f;
    }
    angles.m_yaw = NormAngle180(-MathLib::ArcSin(z));
    if (angles.m_yaw >= -89.0f && angles.m_yaw <= 89.0f) {
        angles.m_pitch = NormAngle180(MathLib::ArcTan(-m_forward.m_x, m_forward.m_y));
        angles.m_roll = NormAngle180(-MathLib::ArcTan(m_right.m_z, m_up.m_z));
    } else {
        // looking straight up or down: roll is folded into pitch
        angles.m_pitch = NormAngle180(-MathLib::ArcTan(m_right.m_x, m_right.m_y));
        angles.m_roll = 0.0f;
    }
    return angles;
}

Angles& Vectors::ToAngles_ByKashtur(Angles& angles) const
{
    float z = m_forward.m_z;
    if (z > 1.0f) {
        z = 1.0f;
    } else if (z < -1.0f) {
        z = -1.0f;
    }
    angles.m_yaw = NormAngle180(-MathLib::ArcSin(z));
    angles.m_pitch = NormAngle180(MathLib::ArcTan(-m_forward.m_x, m_forward.m_y));
    angles.m_roll = NormAngle180(-MathLib::ArcTan(m_right.m_z, m_up.m_z));
    return angles;
}

Quaternion& Mat4x4::ToQuaternion(Quaternion& quaternion) const
{
    static int next[3] = { 1, 2, 0 };

    const Mat4x4& mat = *this;
    float trace = mat[0][0] + mat[1][1] + mat[2][2];
    if (trace > 0.0f) {
        float t = trace + 1.0f;
        float s = MathLib::RSqrt(t) * 0.5f;
        quaternion[3] = s * t;
        quaternion[0] = (mat[1][2] - mat[2][1]) * s;
        quaternion[1] = (mat[2][0] - mat[0][2]) * s;
        quaternion[2] = (mat[0][1] - mat[1][0]) * s;
    } else {
        int i = 0;
        if (mat[1][1] > mat[0][0]) {
            i = 1;
        }
        if (mat[2][2] > mat[i][i]) {
            i = 2;
        }
        int j = next[i];
        int k = next[j];

        float t = (mat[i][i] - (mat[j][j] + mat[k][k])) + 1.0f;
        float s = MathLib::RSqrt(t) * 0.5f;
        quaternion[i] = s * t;
        quaternion[3] = (mat[j][k] - mat[k][j]) * s;
        quaternion[j] = (mat[i][j] + mat[j][i]) * s;
        quaternion[k] = (mat[i][k] + mat[k][i]) * s;
    }
    return quaternion;
}

Angles::Angles(const vec3& forward, const vec3& up)
{
    Vectors vectors(forward, up);
    vectors.ToAngles(*this);
}

vec3 Angles::Forward() const
{
    float sinPitch;
    float cosPitch;
    float sinYaw;
    float cosYaw;
    MathLib::SinCos(m_pitch, sinPitch, cosPitch);
    MathLib::SinCos(m_yaw, sinYaw, cosYaw);
    vec3 forward(-(cosYaw * sinPitch), cosYaw * cosPitch, -sinYaw);
    forward.FixDegenerateNormal();
    return forward;
}

void Angles::ToQuaternion(Quaternion& quaternion) const
{
    Vectors vectors;
    ToVectors(vectors).ToQuaternion(quaternion);
}

Vectors& Quaternion::ToVectors(Vectors& vectors) const
{
    float lengthSqr = m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w;
    float scale;
    if (lengthSqr > 0.0f) {
        scale = 2.0f / lengthSqr;
    } else {
        scale = 0.0f;
    }
    float xs = m_x * scale;
    float ys = m_y * scale;
    float zs = m_z * scale;
    float wx = m_w * xs;
    float wy = m_w * ys;
    float wz = m_w * zs;
    float xx = m_x * xs;
    float xy = m_x * ys;
    float xz = m_x * zs;
    float yy = m_y * ys;
    float yz = m_y * zs;
    float zz = m_z * zs;
    vectors.m_right = vec3(1.0f - (yy + zz), xy - wz, xz + wy);
    vectors.m_forward = vec3(xy + wz, 1.0f - (xx + zz), yz - wx);
    vectors.m_up = vec3(xz - wy, yz + wx, 1.0f - (xx + yy));
    return vectors;
}

void Quaternion::ToAngles_ByKashtur(Angles& angles) const
{
    Vectors vectors;
    ToVectors(vectors).ToAngles_ByKashtur(angles);
}

Mat4x4::Mat4x4(float diagonal)
{
    memset(this, 0, sizeof(Mat4x4));
    m_rows[0][0] = diagonal;
    m_rows[1][1] = diagonal;
    m_rows[2][2] = diagonal;
    m_rows[3][3] = 1.0f;
}

void Mat4x4::Ident(float diagonal)
{
    memset(this, 0, sizeof(Mat4x4));
    m_rows[0][0] = diagonal;
    m_rows[1][1] = diagonal;
    m_rows[2][2] = diagonal;
    m_rows[3][3] = 1.0f;
}

Mat4x4::Mat4x4(const Vectors& vectors, const vec3& translation)
{
    m_rows[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    m_rows[0] = vec4(vectors.m_right.m_x, vectors.m_forward.m_x, vectors.m_up.m_x, translation.m_x);
    m_rows[1] = vec4(vectors.m_right.m_y, vectors.m_forward.m_y, vectors.m_up.m_y, translation.m_y);
    m_rows[2] = vec4(vectors.m_right.m_z, vectors.m_forward.m_z, vectors.m_up.m_z, translation.m_z);
}

Vectors& Mat4x4::ToVectors(Vectors& vectors) const
{
    vectors.m_forward = vec3(m_rows[0][1], m_rows[1][1], m_rows[2][1]);
    vectors.m_right = vec3(m_rows[0][0], m_rows[1][0], m_rows[2][0]);
    vectors.m_up = vec3(m_rows[0][2], m_rows[1][2], m_rows[2][2]);
    return vectors;
}

void Mat4x4::SetAngles(const Angles& angles)
{
    // rotation columns right / forward / up; the translation column is kept
    Vectors vectors;
    const Vectors& rotation = angles.ToVectors(vectors);
    m_rows[0].SetXYZ(rotation.m_right.m_x, rotation.m_forward.m_x, rotation.m_up.m_x);
    m_rows[1].SetXYZ(rotation.m_right.m_y, rotation.m_forward.m_y, rotation.m_up.m_y);
    m_rows[2].SetXYZ(rotation.m_right.m_z, rotation.m_forward.m_z, rotation.m_up.m_z);
}

vec3 Mat4x4::Translation() const
{
    return vec3(m_rows[0][3], m_rows[1][3], m_rows[2][3]);
}

void Mat4x4::SetTranslation(const vec3& translation)
{
    m_rows[0][3] = translation.m_x;
    m_rows[1][3] = translation.m_y;
    m_rows[2][3] = translation.m_z;
}

vec3 Mat4x4::RotateFast(const vec3& v) const
{
    return vec3(m_rows[0][0] * v.m_x + m_rows[0][1] * v.m_y + m_rows[0][2] * v.m_z,
                m_rows[1][0] * v.m_x + m_rows[1][1] * v.m_y + m_rows[1][2] * v.m_z,
                m_rows[2][0] * v.m_x + m_rows[2][1] * v.m_y + m_rows[2][2] * v.m_z);
}

vec3 Mat4x4::TransformFast(const vec3& v) const
{
    return vec3(m_rows[0][0] * v.m_x + m_rows[0][1] * v.m_y + m_rows[0][2] * v.m_z + m_rows[0][3],
                m_rows[1][0] * v.m_x + m_rows[1][1] * v.m_y + m_rows[1][2] * v.m_z + m_rows[1][3],
                m_rows[2][0] * v.m_x + m_rows[2][1] * v.m_y + m_rows[2][2] * v.m_z + m_rows[2][3]);
}

/** |x| written as a compare (evaluated twice by the pivot search, as in the original). */
#define PIVOT_ABS(x) ((x) < 0.0f ? -(x) : (x))

bool Mat4x4::Invert()
{
    // Numerical Recipes gaussj on the transposed matrix
    int indexColumn[4];
    int indexRow[4];
    int pivotUsed[4];
    float a[4][4];
    int row = 0;
    int column = 0;

    for (int j = 0; j < 4; j++) {
        pivotUsed[j] = 0;
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            a[i][j] = m_rows[j][i];
        }
    }

    for (int i = 0; i < 4; i++) {
        float big = 0.0f;
        for (int j = 0; j < 4; j++) {
            if (pivotUsed[j] != 1) {
                for (int k = 0; k < 4; k++) {
                    if (pivotUsed[k] == 0) {
                        if (PIVOT_ABS(a[j][k]) >= big) {
                            big = PIVOT_ABS(a[j][k]);
                            row = j;
                            column = k;
                        }
                    } else if (pivotUsed[k] > 1) {
                        return false;
                    }
                }
            }
        }
        pivotUsed[column] = pivotUsed[column] + 1;
        if (row != column) {
            for (int l = 0; l < 4; l++) {
                float temp = a[row][l];
                a[row][l] = a[column][l];
                a[column][l] = temp;
            }
        }
        indexRow[i] = row;
        indexColumn[i] = column;
        if (a[column][column] == 0.0f) {
            return false;
        }
        float pivotInverse = 1.0f / a[column][column];
        a[column][column] = 1.0f;
        for (int l = 0; l < 4; l++) {
            a[column][l] = a[column][l] * pivotInverse;
        }
        for (int ll = 0; ll < 4; ll++) {
            if (ll != column) {
                float factor = a[ll][column];
                a[ll][column] = 0.0f;
                for (int l = 0; l < 4; l++) {
                    a[ll][l] = a[ll][l] - a[column][l] * factor;
                }
            }
        }
    }

    for (int l = 3; l >= 0; l--) {
        if (indexRow[l] != indexColumn[l]) {
            for (int k = 0; k < 4; k++) {
                float temp = a[k][indexRow[l]];
                a[k][indexRow[l]] = a[k][indexColumn[l]];
                a[k][indexColumn[l]] = temp;
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m_rows[j][i] = a[i][j];
        }
    }
    return true;
}

/** Row `row` of `a` times column `column` of `b`. */
static inline float MultiplyRowColumn(const Mat4x4& a, int row, const Mat4x4& b, int column)
{
    return b[0][column] * a[row][0] + b[1][column] * a[row][1] + b[2][column] * a[row][2] + b[3][column] * a[row][3];
}

void Mat4x4::Multiply(const Mat4x4& other)
{
    // this = other * this; every element is read before the first store
    float r00 = MultiplyRowColumn(other, 0, *this, 0);
    float r01 = MultiplyRowColumn(other, 0, *this, 1);
    float r02 = MultiplyRowColumn(other, 0, *this, 2);
    float r03 = MultiplyRowColumn(other, 0, *this, 3);
    float r10 = MultiplyRowColumn(other, 1, *this, 0);
    float r11 = MultiplyRowColumn(other, 1, *this, 1);
    float r12 = MultiplyRowColumn(other, 1, *this, 2);
    float r13 = MultiplyRowColumn(other, 1, *this, 3);
    float r20 = MultiplyRowColumn(other, 2, *this, 0);
    float r21 = MultiplyRowColumn(other, 2, *this, 1);
    float r22 = MultiplyRowColumn(other, 2, *this, 2);
    float r23 = MultiplyRowColumn(other, 2, *this, 3);
    float r30 = MultiplyRowColumn(other, 3, *this, 0);
    float r31 = MultiplyRowColumn(other, 3, *this, 1);
    float r32 = MultiplyRowColumn(other, 3, *this, 2);
    float r33 = MultiplyRowColumn(other, 3, *this, 3);
    m_rows[0] = vec4(r00, r01, r02, r03);
    m_rows[1] = vec4(r10, r11, r12, r13);
    m_rows[2] = vec4(r20, r21, r22, r23);
    m_rows[3] = vec4(r30, r31, r32, r33);
}

void Mat4x4::PostMultiply(const Mat4x4& other)
{
    // this = this * other
    float r00 = MultiplyRowColumn(*this, 0, other, 0);
    float r01 = MultiplyRowColumn(*this, 0, other, 1);
    float r02 = MultiplyRowColumn(*this, 0, other, 2);
    float r03 = MultiplyRowColumn(*this, 0, other, 3);
    float r10 = MultiplyRowColumn(*this, 1, other, 0);
    float r11 = MultiplyRowColumn(*this, 1, other, 1);
    float r12 = MultiplyRowColumn(*this, 1, other, 2);
    float r13 = MultiplyRowColumn(*this, 1, other, 3);
    float r20 = MultiplyRowColumn(*this, 2, other, 0);
    float r21 = MultiplyRowColumn(*this, 2, other, 1);
    float r22 = MultiplyRowColumn(*this, 2, other, 2);
    float r23 = MultiplyRowColumn(*this, 2, other, 3);
    float r30 = MultiplyRowColumn(*this, 3, other, 0);
    float r31 = MultiplyRowColumn(*this, 3, other, 1);
    float r32 = MultiplyRowColumn(*this, 3, other, 2);
    float r33 = MultiplyRowColumn(*this, 3, other, 3);
    m_rows[0] = vec4(r00, r01, r02, r03);
    m_rows[1] = vec4(r10, r11, r12, r13);
    m_rows[2] = vec4(r20, r21, r22, r23);
    m_rows[3] = vec4(r30, r31, r32, r33);
}

void Mat4x4::PostRotate(float degrees, float x, float y, float z)
{
    // note: the radians are passed on to the degree based Sin / Cos, as in the original
    float radians = degrees * PI_VALUE / 180.0f;
    float lengthSqr = x * x + y * y + z * z;
    if (radians == 0.0) {
        return;
    }
    if (lengthSqr == 0.0) {
        return;
    }
    float sine = MathLib::Sin(radians);
    float cosine = MathLib::Cos(radians);
    Mat4x4 rotation(1.0f);
    float length = MathLib::Sqrt(lengthSqr);
    x = x / length;
    y = y / length;
    z = z / length;
    float xx = x * x;
    float yy = y * y;
    float zz = z * z;
    float xy = x * y;
    float yz = y * z;
    float zx = z * x;
    float xs = x * sine;
    float ys = y * sine;
    float zs = z * sine;
    float oneMinusCos = 1.0f - cosine;
    rotation[0][0] = oneMinusCos * xx + cosine;
    rotation[1][0] = oneMinusCos * xy + zs;
    rotation[2][0] = oneMinusCos * zx - ys;
    rotation[3][0] = 0.0f;
    rotation[0][1] = oneMinusCos * xy - zs;
    rotation[1][1] = oneMinusCos * yy + cosine;
    rotation[2][1] = oneMinusCos * yz + xs;
    rotation[3][1] = 0.0f;
    rotation[0][2] = oneMinusCos * zx + ys;
    rotation[1][2] = oneMinusCos * yz - xs;
    rotation[2][2] = oneMinusCos * zz + cosine;
    rotation[3][2] = 0.0f;
    rotation[2][3] = 0.0f;
    rotation[1][3] = 0.0f;
    rotation[0][3] = 0.0f;
    rotation[3][3] = 1.0f;
    PostMultiply(rotation);
}

void Mat4x4::PostScale(float x, float y, float z)
{
    Mat4x4 scale(1.0f);
    scale.Ident(1.0f);
    scale[0][0] = x;
    scale[1][1] = y;
    scale[2][2] = z;
    PostMultiply(scale);
}
