// MathLib.h - float math of the game layer: fast trig/exp/log, vectors, planes, angles, quaternions,
// 4x4 matrices and a small LCG random generator (created by b051x; replaces the b045x PROVISIONAL,
// MathLib::Round kept). Implemented in MathLib.cpp.
// Much of it follows the Doom 3 idLib math code (FixDegenerateNormal, RSqrt, Mat3::ToQuat, ...).
// The default constructors of vec3 / vec4 zero their members: Plane, Vectors and Mat4x4 constructors
// in the original store zeros (or call memset) before doing anything else.
#ifndef MATHLIB_H
#define MATHLIB_H

class vec3;
class Plane;
class Angles;
class Vectors;
class Quaternion;
class Mat4x4;

/** Fast float math (degrees for angles). */
class MathLib {
public:
    /** 1 / sqrt(x) (one Newton step). */
    static float InvSqrt(float x);

    /** 1 / sqrt(x), inline variant (Doom 3 idMath::RSqrt); used by the normalisations. */
    static float RSqrt(float x)
    {
        float half = x * 0.5f;
        int bits = *(int*)&x;
        bits = 0x5f3759df - (bits >> 1);
        float root = *(float*)&bits;
        root = root * (1.5f - root * root * half);
        return root;
    }

    /** |x| by clearing the sign bit. */
    static float Fabs(float x)
    {
        int bits = *(int*)&x;
        bits = bits & 0x7fffffff;
        return *(float*)&bits;
    }

    /** Angle in [0, 360). */
    static float NormAngle(float degrees);
    static float Sin(float degrees);
    static float Cos(float degrees);
    static void SinCos(float degrees, float& sine, float& cosine);
    /** Degrees. */
    static float ArcSin(float x);
    /** Degrees; angle of (x, y) with y as the first argument. */
    static float ArcTan(float y, float x);
    static float Frexp(float x, int* exponent);
    static float Floor(float x);
    /** Dead-stripped in the shipped binary (name guessed), see MathLib.cpp. */
    static float Ceil(float x);
    /** `value` rounded to the nearest integer. */
    static int Round(float value);
    static float Pow(float base, float exponent);
    static float Exp(float x);
    static float Log(float x);
    static int CeilPowerOfTwo(int value);

    static float Sqrt(float x)
    {
        return InvSqrt(x) * x;
    }

    /** Is `hit` (a point of `plane`) inside the triangle a, b, c? (body only survives inlined) */
    static bool RayTriangleIntersection(const vec3& origin, const vec3& direction, const Plane& plane,
                                        const vec3& a, const vec3& b, const vec3& c, vec3& hit, float& distance);
    static bool RayTriangleIntersection(const vec3& origin, const vec3& direction, float length, const Plane& plane,
                                        const vec3& a, const vec3& b, const vec3& c, vec3& hit, float& distance,
                                        float frontDistance, float backDistance, float minDistance, float maxDistance);
    /** Vector from the closest point of segment [a, b] to `point`; `inside` when it falls between a and b. */
    static vec3 DistanceToSegment(const vec3& point, const vec3& a, const vec3& b, bool& inside);
    static vec3 DistanceToLine(const vec3& point, const vec3& a, const vec3& b);
};

/** 3D vector. */
class vec3 {
public:
    vec3()
    {
        // Chained assignment as in the original (b057x: App/BaseDialog inline shapes depend on it).
        m_x = m_y = m_z = 0.0f;
    }

    // No user copy assignment (removed by fx2): the original copies vec3 with integer ldr/str or ldm/stm
    // (implicit trivial assignment), e.g. `m_forward = forward` in Vectors::Vectors and `m_normal = normal`
    // in Plane::Init. The float copies in CAuxRender::DrawSphere/DrawBox come from another source shape
    // (the center is loaded before the task stores), not from operator=; no unit changes its MATCH count.

    vec3(float x, float y, float z)
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    float& operator[](int index)
    {
        return (&m_x)[index];
    }

    vec3 operator+(const vec3& other) const
    {
        return vec3(m_x + other.m_x, m_y + other.m_y, m_z + other.m_z);
    }

    vec3 operator-(const vec3& other) const
    {
        return vec3(m_x - other.m_x, m_y - other.m_y, m_z - other.m_z);
    }

    vec3 operator*(float scale) const
    {
        return vec3(m_x * scale, m_y * scale, m_z * scale);
    }

    float Dot(const vec3& other) const
    {
        return m_x * other.m_x + m_y * other.m_y + m_z * other.m_z;
    }

    vec3 Cross(const vec3& other) const
    {
        return vec3(m_y * other.m_z - m_z * other.m_y, m_z * other.m_x - m_x * other.m_z,
                    m_x * other.m_y - m_y * other.m_x);
    }

    float LengthSqr() const
    {
        return m_x * m_x + m_y * m_y + m_z * m_z;
    }

    /** Scale to unit length (RSqrt). */
    void Normalize()
    {
        float inverseLength = MathLib::RSqrt(LengthSqr());
        m_x = m_x * inverseLength;
        m_y = m_y * inverseLength;
        m_z = m_z * inverseLength;
    }

    /** Snap a normal that is axis aligned up to rounding to the exact axis; true when changed. */
    bool FixDegenerateNormal();

    float m_x;  // 0x00
    float m_y;  // 0x04
    float m_z;  // 0x08
};

/** 4D vector, one row of Mat4x4. */
class vec4 {
public:
    vec4()
    {
        // CCollision C2 at 0xf1eb0 stores the four zeros in z, w, y, x order.
        m_z = 0.0f;
        m_w = 0.0f;
        m_y = 0.0f;
        m_x = 0.0f;
    }

    vec4(float x, float y, float z, float w)
    {
        m_x = x;
        m_y = y;
        m_z = z;
        m_w = w;
    }

    float& operator[](int index)
    {
        return (&m_x)[index];
    }

    const float& operator[](int index) const
    {
        return (&m_x)[index];
    }

    /** Set x, y, z; w is left alone. */
    void SetXYZ(float x, float y, float z)
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    float m_x;  // 0x00
    float m_y;  // 0x04
    float m_z;  // 0x08
    float m_w;  // 0x0c
};

/** Plane n.p = d. */
class Plane {
public:
    /** Plane through three points; `fixNormal` snaps an almost axis aligned normal. */
    Plane(const vec3& a, const vec3& b, const vec3& c, bool fixNormal);

    void Init(const vec3& a, const vec3& b, const vec3& c, bool fixNormal);
    /** Dominant axis of the normal (0 x, 1 y, 2 z). */
    int PlaneAxeNum() const;
    /** `point` projected onto the plane. */
    vec3 Projection(const vec3& point) const;

    /** Signed distance of `point` (for a unit normal). */
    float Distance(const vec3& point) const
    {
        return m_normal.Dot(point) - m_distance;
    }

    vec3 m_normal;      // 0x00
    float m_distance;   // 0x0c
};

/** Linear congruential generator (Numerical Recipes constants). */
class RandomValue {
public:
    /** [0, range). */
    int Int(int range);
    /** [0, 1). */
    float Float();
    /** [-1, 1). */
    float CFloat();

    unsigned int m_seed;    // 0x00
};

/** Pitch / yaw / roll in degrees. */
class Angles {
public:
    Angles()
    {
    }

    /** Angles of the frame given by `forward` and `up`. */
    Angles(const vec3& forward, const vec3& up);

    Vectors& ToVectors(Vectors& vectors) const;
    vec3 Forward() const;
    void ToQuaternion(Quaternion& quaternion) const;

    float m_pitch;  // 0x00
    float m_yaw;    // 0x04
    float m_roll;   // 0x08
};

/** Orthonormal frame. */
class Vectors {
public:
    Vectors()
    {
    }

    Vectors(const vec3& forward, const vec3& up);

    Angles& ToAngles(Angles& angles) const;
    Angles& ToAngles_ByKashtur(Angles& angles) const;
    /** Inline (defined after Mat4x4): through a rotation matrix. */
    void ToQuaternion(Quaternion& quaternion) const;

    vec3 m_forward; // 0x00
    vec3 m_right;   // 0x0c
    vec3 m_up;      // 0x18
};

class Quaternion {
public:
    float& operator[](int index)
    {
        return (&m_x)[index];
    }

    Vectors& ToVectors(Vectors& vectors) const;
    void ToAngles_ByKashtur(Angles& angles) const;

    /** Hamilton product; grouping follows the original weak body at 0xf102c. */
    Quaternion Mul(const Quaternion& other) const
    {
        // 0xf102c computes all four components before the stores at 0xf10c6..0xf10d2.
        float x = m_w * other.m_x + m_x * other.m_w + m_y * other.m_z - m_z * other.m_y;
        float y = m_w * other.m_y + m_y * other.m_w + m_z * other.m_x - m_x * other.m_z;
        float z = m_w * other.m_z + m_z * other.m_w + m_x * other.m_y - m_y * other.m_x;
        float w = m_w * other.m_w - m_x * other.m_x - m_y * other.m_y - m_z * other.m_z;
        Quaternion result;
        result.m_x = x;
        result.m_y = y;
        result.m_z = z;
        result.m_w = w;
        return result;
    }

    float m_x;  // 0x00
    float m_y;  // 0x04
    float m_z;  // 0x08
    float m_w;  // 0x0c
};

/** Row major 4x4 matrix; the translation is the last column (m[3], m[7], m[11]). */
class Mat4x4 {
public:
    Mat4x4()
    {
    }

    /** `diagonal` on the diagonal (m[15] = 1), 0 elsewhere. */
    Mat4x4(float diagonal);
    /** Rotation columns right / forward / up, then the translation column. */
    Mat4x4(const Vectors& vectors, const vec3& translation);

    vec4& operator[](int row)
    {
        return m_rows[row];
    }

    const vec4& operator[](int row) const
    {
        return m_rows[row];
    }

    /** The 16 floats, row by row (the old `m` array). */
    float* ToFloatPtr()
    {
        return &m_rows[0].m_x;
    }

    const float* ToFloatPtr() const
    {
        return &m_rows[0].m_x;
    }

    Quaternion& ToQuaternion(Quaternion& quaternion) const;
    void Ident(float diagonal);
    Vectors& ToVectors(Vectors& vectors) const;
    void SetAngles(const Angles& angles);
    vec3 Translation() const;
    void SetTranslation(const vec3& translation);
    vec3 RotateFast(const vec3& v) const;
    vec3 TransformFast(const vec3& v) const;
    /** Gauss-Jordan inversion in place; false (matrix partly changed) when singular. */
    bool Invert();
    /** this = other * this. */
    void Multiply(const Mat4x4& other);
    /** this = this * other. */
    void PostMultiply(const Mat4x4& other);
    void PostRotate(float degrees, float x, float y, float z);
    void PostScale(float x, float y, float z);

    vec4 m_rows[4];     // 0x00
};

inline void Vectors::ToQuaternion(Quaternion& quaternion) const
{
    Mat4x4 matrix(*this, vec3(0.0f, 0.0f, 0.0f));
    matrix.ToQuaternion(quaternion);
}

#endif
