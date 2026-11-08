// Original inline at 0x0f10e4: the tangent branch and normalized result are intentional.
// The box helpers are fully inlined in OcTreeSector/CCollision constructors.
inline void CCollision::AABB::Split(AABB& low, AABB& high) const
{
    vec3 extent = m_max - m_min;
    int axis = 0;
    float longest = extent.m_x;
    if (extent.m_y > longest) {
        longest = extent.m_y;
        axis = 1;
    }
    if (extent.m_z > longest) {
        axis = 2;
    }
    float middle = (&m_min.m_x)[axis] + (&extent.m_x)[axis] * 0.5f;
    low = *this;
    high = *this;
    (&low.m_max.m_x)[axis] = middle;
    (&high.m_min.m_x)[axis] = middle;
}

inline void CCollision::AABB::Add(const vec3& point)
{
    // CCollision C2 tests each minimum before the matching maximum.
    if (point.m_x < m_min.m_x) { m_min.m_x = point.m_x; }
    if (point.m_x > m_max.m_x) { m_max.m_x = point.m_x; }
    if (point.m_y < m_min.m_y) { m_min.m_y = point.m_y; }
    if (point.m_y > m_max.m_y) { m_max.m_y = point.m_y; }
    if (point.m_z < m_min.m_z) { m_min.m_z = point.m_z; }
    if (point.m_z > m_max.m_z) { m_max.m_z = point.m_z; }
}

inline bool CCollision::Intersect(const Line& line, const Sphere& sphere, vec3& point)
{
    vec3 offset = line.m_from - sphere.m_center;
    vec3 direction = line.m_to - line.m_from;
    float a = direction.Dot(direction);
    float b = 2.0f * offset.Dot(direction);
    float c = offset.Dot(offset) - sphere.m_radius * sphere.m_radius;
    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return false;
    }
    float twiceA = a + a;
    float distance = b;
    if (discriminant > 0.0f) {
        distance = (MathLib::InvSqrt(discriminant) * discriminant - b) / twiceA;
    } else if (discriminant == 0.0f) {
        distance = -(b * twiceA);
    }
    direction = direction * MathLib::InvSqrt(a);
    point = line.m_from + direction * distance;
    return true;
}

/** Two-projection separating-axis test used by the original triangle/box inline. */
static inline bool CollisionAxisOverlaps(float first, float second, float radius)
{
    float minimum = second;
    float maximum = first;
    if (first < second) {
        minimum = first;
        maximum = second;
    }
    if (minimum > radius || maximum < -radius) {
        return false;
    }
    return true;
}

inline bool CCollision::Intersect(const AABB& box, const vec3& first, const vec3& second, const vec3& third)
{
    vec3 half = (box.m_max - box.m_min) * 0.5f;
    vec3 center = box.m_min + half;
    vec3 vertices[3];
    vertices[0] = first - center;
    vertices[1] = second - center;
    vertices[2] = third - center;
    vec3 edges[3];
    edges[0] = vertices[1] - vertices[0];
    edges[1] = vertices[2] - vertices[1];
    edges[2] = vertices[0] - vertices[2];
    // Nine edge/canonical-axis tests precede the coordinate and triangle-plane tests.
    for (int index = 0; index < 3; index++) {
        const vec3& edge = edges[index];
        int firstIndex = 0;
        int secondIndex = 2;
        if (index == 2) {
            secondIndex = 1;
        }
        float firstProjection = edge.m_z * vertices[firstIndex].m_y - edge.m_y * vertices[firstIndex].m_z;
        float secondProjection = edge.m_z * vertices[secondIndex].m_y - edge.m_y * vertices[secondIndex].m_z;
        float radius = MathLib::Fabs(edge.m_z) * half.m_y + MathLib::Fabs(edge.m_y) * half.m_z;
        if (!CollisionAxisOverlaps(firstProjection, secondProjection, radius)) {
            return false;
        }
        firstProjection = edge.m_x * vertices[firstIndex].m_z - edge.m_z * vertices[firstIndex].m_x;
        secondProjection = edge.m_x * vertices[secondIndex].m_z - edge.m_z * vertices[secondIndex].m_x;
        radius = MathLib::Fabs(edge.m_z) * half.m_x + MathLib::Fabs(edge.m_x) * half.m_z;
        if (!CollisionAxisOverlaps(firstProjection, secondProjection, radius)) {
            return false;
        }
        firstIndex = 1;
        secondIndex = 2;
        if (index == 1) {
            firstIndex = 0;
            secondIndex = 1;
        }
        firstProjection = edge.m_y * vertices[firstIndex].m_x - edge.m_x * vertices[firstIndex].m_y;
        secondProjection = edge.m_y * vertices[secondIndex].m_x - edge.m_x * vertices[secondIndex].m_y;
        radius = MathLib::Fabs(edge.m_y) * half.m_x + MathLib::Fabs(edge.m_x) * half.m_y;
        if (!CollisionAxisOverlaps(firstProjection, secondProjection, radius)) {
            return false;
        }
    }
    for (int axis = 0; axis < 3; axis++) {
        float minimum = (&vertices[1].m_x)[axis];
        float maximum = (&vertices[1].m_x)[axis];
        if ((&vertices[0].m_x)[axis] < minimum) { minimum = (&vertices[0].m_x)[axis]; }
        if ((&vertices[0].m_x)[axis] > maximum) { maximum = (&vertices[0].m_x)[axis]; }
        if ((&vertices[2].m_x)[axis] < minimum) { minimum = (&vertices[2].m_x)[axis]; }
        if ((&vertices[2].m_x)[axis] > maximum) { maximum = (&vertices[2].m_x)[axis]; }
        if (minimum > (&half.m_x)[axis] || maximum < -(&half.m_x)[axis]) {
            return false;
        }
    }
    vec3 normal = edges[0].Cross(edges[1]);
    float planeDistance = normal.Dot(vertices[0]);
    vec3 low;
    vec3 high;
    for (int axis = 0; axis < 3; axis++) {
        float extent = (&half.m_x)[axis];
        (&low.m_x)[axis] = extent;
        (&high.m_x)[axis] = -extent;
        if ((&normal.m_x)[axis] > 0.0f) {
            (&low.m_x)[axis] = -extent;
            (&high.m_x)[axis] = extent;
        }
    }
    if (normal.Dot(low) - planeDistance > 0.0f) {
        return false;
    }
    return normal.Dot(high) - planeDistance >= 0.0f;
}

static inline int CollisionBoxOutcode(const vec3& point, const CCollision::AABB& box)
{
    int code = 0;
    if (point.m_x > box.m_max.m_x) { code |= 1; }
    else if (point.m_x < box.m_min.m_x) { code |= 2; }
    if (point.m_y > box.m_max.m_y) { code |= 4; }
    else if (point.m_y < box.m_min.m_y) { code |= 8; }
    if (point.m_z > box.m_max.m_z) { code |= 0x20; }
    else if (point.m_z < box.m_min.m_z) { code |= 0x10; }
    return code;
}

inline bool CCollision::Intersect(const Line& line, const AABB& box, vec3& point)
{
    int fromCode = CollisionBoxOutcode(line.m_from, box);
    if (fromCode == 0) {
        point = line.m_from;
        return true;
    }
    int toCode = CollisionBoxOutcode(line.m_to, box);
    if (toCode == 0) {
        point = line.m_to;
        return true;
    }
    if ((fromCode & toCode) != 0) {
        return false;
    }
    if ((fromCode & 3) != 0) {
        float bound = box.m_min.m_x;
        if ((fromCode & 1) != 0) { bound = box.m_max.m_x; }
        point.m_x = bound;
        point.m_y = line.m_from.m_y + (bound - line.m_from.m_x) * (line.m_to.m_y - line.m_from.m_y) / (line.m_to.m_x - line.m_from.m_x);
        point.m_z = line.m_from.m_z + (bound - line.m_from.m_x) * (line.m_to.m_z - line.m_from.m_z) / (line.m_to.m_x - line.m_from.m_x);
        if (point.m_y <= box.m_max.m_y && point.m_y >= box.m_min.m_y &&
            point.m_z <= box.m_max.m_z && point.m_z >= box.m_min.m_z) {
            return true;
        }
    }
    if ((fromCode & 0xc) != 0) {
        float bound = box.m_min.m_y;
        if ((fromCode & 4) != 0) { bound = box.m_max.m_y; }
        point.m_y = bound;
        point.m_x = line.m_from.m_x + (bound - line.m_from.m_y) * (line.m_to.m_x - line.m_from.m_x) / (line.m_to.m_y - line.m_from.m_y);
        point.m_z = line.m_from.m_z + (bound - line.m_from.m_y) * (line.m_to.m_z - line.m_from.m_z) / (line.m_to.m_y - line.m_from.m_y);
        if (point.m_x <= box.m_max.m_x && point.m_x >= box.m_min.m_x &&
            point.m_z <= box.m_max.m_z && point.m_z >= box.m_min.m_z) {
            return true;
        }
    }
    if ((fromCode & 0x30) != 0) {
        float bound = box.m_min.m_z;
        if ((fromCode & 0x20) != 0) { bound = box.m_max.m_z; }
        point.m_z = bound;
        point.m_x = line.m_from.m_x + (bound - line.m_from.m_z) * (line.m_to.m_x - line.m_from.m_x) / (line.m_to.m_z - line.m_from.m_z);
        point.m_y = line.m_from.m_y + (bound - line.m_from.m_z) * (line.m_to.m_y - line.m_from.m_y) / (line.m_to.m_z - line.m_from.m_z);
        if (point.m_x <= box.m_max.m_x && point.m_x >= box.m_min.m_x &&
            point.m_y <= box.m_max.m_y && point.m_y >= box.m_min.m_y) {
            return true;
        }
    }
    return false;
}

inline bool CCollision::IsPointWithinTriangle(const vec3& point, const vec3& normal,
                                             const vec3& first, const vec3& second, const vec3& third)
{
    int projection = 0xf00;
    float x = MathLib::Fabs(normal.m_x);
    float y = MathLib::Fabs(normal.m_y);
    float z = MathLib::Fabs(normal.m_z);
    if (z > y) {
        if (z > x) {
            projection = 0xf;
        }
    } else if (y > x) {
        projection = 0xf0;
    }
    const vec3* vertices[3] = { &first, &second, &third };
    bool inside = false;
    int previous = 2;
    // The original retains all three projected loops (0xf5c92, 0xf5d28, 0xf5dbc).
    if (projection == 0xf00) {
        for (int index = 0; index < 3; index++) {
            const vec3& current = *vertices[index];
            const vec3& before = *vertices[previous];
            if ((current.m_y <= point.m_y && point.m_y < before.m_y) ||
                (before.m_y <= point.m_y && point.m_y < current.m_y)) {
                float crossing = current.m_z + (before.m_z - current.m_z) * (point.m_y - current.m_y) / (before.m_y - current.m_y);
                if (point.m_z < crossing) {
                    inside = !inside;
                }
            }
            previous = index;
        }
    } else if (projection == 0xf0) {
        for (int index = 0; index < 3; index++) {
            const vec3& current = *vertices[index];
            const vec3& before = *vertices[previous];
            if ((current.m_z <= point.m_z && point.m_z < before.m_z) ||
                (before.m_z <= point.m_z && point.m_z < current.m_z)) {
                float crossing = current.m_x + (before.m_x - current.m_x) * (point.m_z - current.m_z) / (before.m_z - current.m_z);
                if (point.m_x < crossing) {
                    inside = !inside;
                }
            }
            previous = index;
        }
    } else if (projection == 0xf) {
        for (int index = 0; index < 3; index++) {
            const vec3& current = *vertices[index];
            const vec3& before = *vertices[previous];
            if ((current.m_y <= point.m_y && point.m_y < before.m_y) ||
                (before.m_y <= point.m_y && point.m_y < current.m_y)) {
                float crossing = current.m_x + (before.m_x - current.m_x) * (point.m_y - current.m_y) / (before.m_y - current.m_y);
                if (point.m_x < crossing) {
                    inside = !inside;
                }
            }
            previous = index;
        }
    }
    return inside;
}

inline void CCollision::GetDistance(const Sphere& sphere, const vec3& normal, const vec3& first,
                                    const vec3& second, const vec3& third, vec3& point, float& distance)
{
    float planeOffset = first.Dot(normal) - sphere.m_center.Dot(normal);
    vec3 projected = normal * planeOffset + sphere.m_center;
    if (IsPointWithinTriangle(projected, normal, first, second, third) == true) {
        distance = MathLib::Fabs(planeOffset);
        point = projected;
        return;
    }
    // The original is fully unrolled over the three edges (three InvSqrt calls);
    // the first edge always sets the result, the other two only when closer.
    vec3 candidate = ClosestPointOnEdge(projected, first, second);
    vec3 difference = sphere.m_center - candidate;
    float squaredLength = difference.LengthSqr();
    distance = MathLib::InvSqrt(squaredLength) * squaredLength;
    point = candidate;

    candidate = ClosestPointOnEdge(projected, second, third);
    difference = sphere.m_center - candidate;
    squaredLength = difference.LengthSqr();
    float edgeDistance = MathLib::InvSqrt(squaredLength) * squaredLength;
    if (distance > edgeDistance) {
        distance = edgeDistance;
        point = candidate;
    }

    candidate = ClosestPointOnEdge(projected, first, third);
    difference = sphere.m_center - candidate;
    squaredLength = difference.LengthSqr();
    edgeDistance = MathLib::InvSqrt(squaredLength) * squaredLength;
    if (distance > edgeDistance) {
        distance = edgeDistance;
        point = candidate;
    }
}

/** Closest point of segment [start, end] to `point` (helper name is a guess; only seen inlined). */
inline vec3 CCollision::ClosestPointOnEdge(const vec3& point, const vec3& start, const vec3& end)
{
    vec3 edge = end - start;
    float projection = point.Dot(edge) - start.Dot(edge);
    vec3 candidate = end;
    if (projection <= 0.0f) {
        candidate = start;
    } else {
        float squaredLength = edge.LengthSqr();
        if (projection < squaredLength) {
            candidate = edge * (projection / squaredLength) + start;
        }
    }
    return candidate;
}

inline bool CCollision::Intersect(const Sphere& sphere, const vec3& first, const vec3& second,
                                  const vec3& third, vec3& point, float& distance)
{
    float squaredRadius = sphere.m_radius * sphere.m_radius;
    // The original handles each vertex-inside case with its own early return
    // (normal recomputed per case; the GetDistance tails are merged by the compiler).
    vec3 difference = third - sphere.m_center;
    if (difference.LengthSqr() <= squaredRadius) {
        vec3 firstEdge = second - first;
        vec3 secondEdge = third - first;
        vec3 normal = firstEdge.Cross(secondEdge);
        normal = normal * MathLib::InvSqrt(normal.LengthSqr());
        GetDistance(sphere, normal, first, second, third, point, distance);
        return true;
    }
    difference = second - sphere.m_center;
    if (difference.LengthSqr() <= squaredRadius) {
        vec3 firstEdge = second - first;
        vec3 secondEdge = third - first;
        vec3 normal = firstEdge.Cross(secondEdge);
        normal = normal * MathLib::InvSqrt(normal.LengthSqr());
        GetDistance(sphere, normal, first, second, third, point, distance);
        return true;
    }
    difference = first - sphere.m_center;
    if (difference.LengthSqr() <= squaredRadius) {
        vec3 firstEdge = second - first;
        vec3 secondEdge = third - first;
        vec3 normal = firstEdge.Cross(secondEdge);
        normal = normal * MathLib::InvSqrt(normal.LengthSqr());
        GetDistance(sphere, normal, first, second, third, point, distance);
        return true;
    }
    vec3 firstEdge = second - first;
    vec3 secondEdge = third - first;
    {
        // Seven point/triangle regions, with the original region-specific float expressions.
        float a = firstEdge.Dot(firstEdge);
        float b = firstEdge.Dot(secondEdge);
        float c = secondEdge.Dot(secondEdge);
        float d = difference.Dot(firstEdge);
        float e = difference.Dot(secondEdge);
        float squaredDistance = difference.Dot(difference);
        float determinant = MathLib::Fabs(a * c - b * b);
        float s = b * e - c * d;
        float t = b * d - a * e;
        bool evaluatePair = false;
        if (s + t <= determinant) {
            if (s < 0.0f) {
                if (t < 0.0f && d < 0.0f) {
                    float numerator = -d;
                    if (a <= numerator) {
                        squaredDistance = a + (d + d) + squaredDistance;
                    } else {
                        squaredDistance = d * (numerator / a) + squaredDistance;
                    }
                } else if (e < 0.0f) {
                    float numerator = -e;
                    if (c <= numerator) {
                        squaredDistance = c + (e + e) + squaredDistance;
                    } else {
                        squaredDistance = e * (numerator / c) + squaredDistance;
                    }
                }
            } else if (t < 0.0f) {
                if (d < 0.0f) {
                    float numerator = -d;
                    if (a <= numerator) {
                        squaredDistance = a + (d + d) + squaredDistance;
                    } else {
                        squaredDistance = d * (numerator / a) + squaredDistance;
                    }
                }
            } else if (determinant != 0.0f) {
                float inverse = 1.0f / determinant;
                s = s * inverse;
                t = t * inverse;
                evaluatePair = true;
            } else {
                squaredDistance = 3.4e38f; // exact original literal 0x7f7fc99e
            }
        } else if (s < 0.0f) {
            float firstTemporary = b + d;
            float secondTemporary = c + e;
            if (secondTemporary > firstTemporary) {
                float numerator = secondTemporary - firstTemporary;
                float denominator = a + b * -2.0f + c;
                if (numerator < denominator) {
                    s = numerator / denominator;
                    t = 1.0f - s;
                    evaluatePair = true;
                } else {
                    squaredDistance = a + (d + d) + squaredDistance;
                }
            } else if (secondTemporary <= 0.0f) {
                squaredDistance = c + (e + e) + squaredDistance;
            } else if (e < 0.0f) {
                squaredDistance = squaredDistance - e * (e / c);
            }
        } else if (t < 0.0f) {
            float firstTemporary = b + e;
            float secondTemporary = a + d;
            if (secondTemporary > firstTemporary) {
                float numerator = secondTemporary - firstTemporary;
                float denominator = a + b * -2.0f + c;
                if (numerator < denominator) {
                    t = numerator / denominator;
                    s = 1.0f - t;
                    evaluatePair = true;
                } else {
                    squaredDistance = c + (e + e) + squaredDistance;
                }
            } else if (secondTemporary <= 0.0f) {
                squaredDistance = a + (d + d) + squaredDistance;
            } else if (d < 0.0f) {
                squaredDistance = squaredDistance - d * (d / a);
            }
        } else {
            float numerator = c + e - b - d;
            if (numerator <= 0.0f) {
                squaredDistance = c + (e + e) + squaredDistance;
            } else {
                float denominator = a + b * -2.0f + c;
                if (numerator < denominator) {
                    s = numerator / denominator;
                    t = 1.0f - s;
                    evaluatePair = true;
                } else {
                    squaredDistance = a + (d + d) + squaredDistance;
                }
            }
        }
        if (evaluatePair) {
            float secondTerm = b * s + c * t + (e + e);
            float firstTerm = a * s + b * t + (d + d);
            squaredDistance = s * firstTerm + t * secondTerm + squaredDistance;
        }
        if (!(MathLib::Fabs(squaredDistance) < squaredRadius)) {
            return false;
        }
    }
    vec3 normal = firstEdge.Cross(secondEdge);
    normal = normal * MathLib::InvSqrt(normal.LengthSqr());
    GetDistance(sphere, normal, first, second, third, point, distance);
    return true;
}
