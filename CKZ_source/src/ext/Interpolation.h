// Interpolation.h - Robert Penner style easing equations (created by b051x).
// Every equation takes (t, b, c, d): time, start value, change, duration; t <= 0 gives b and
// t >= d gives b + c. The simple In/Out forms are inline (Run expands them); the rest live in
// Interpolation.cpp.
#ifndef INTERPOLATION_H
#define INTERPOLATION_H

class EasyLib {
public:
    /** Direction of an easing run: DIR_BACKWARD runs from b + c back to b. Names 0/1 are guesses. */
    enum EDirType {
        DIR_NONE = 0,
        DIR_FORWARD = 1,
        DIR_BACKWARD = 2
    };

    /** Easing curve; every family has In, Out, InOut and OutIn in this order. */
    enum EMethod {
        LINEAR = 0,
        IN_QUAD = 1,
        OUT_QUAD = 2,
        IN_OUT_QUAD = 3,
        OUT_IN_QUAD = 4,
        IN_CUBIC = 5,
        OUT_CUBIC = 6,
        IN_OUT_CUBIC = 7,
        OUT_IN_CUBIC = 8,
        IN_QUART = 9,
        OUT_QUART = 10,
        IN_OUT_QUART = 11,
        OUT_IN_QUART = 12,
        IN_QUINT = 13,
        OUT_QUINT = 14,
        IN_OUT_QUINT = 15,
        OUT_IN_QUINT = 16,
        IN_EXPO = 17,
        OUT_EXPO = 18,
        IN_OUT_EXPO = 19,
        OUT_IN_EXPO = 20,
        IN_CIRC = 21,
        OUT_CIRC = 22,
        IN_OUT_CIRC = 23,
        OUT_IN_CIRC = 24,
        IN_BACK = 25,
        OUT_BACK = 26,
        IN_OUT_BACK = 27,
        OUT_IN_BACK = 28,
        IN_BOUNCE = 29,
        OUT_BOUNCE = 30,
        IN_OUT_BOUNCE = 31,
        OUT_IN_BOUNCE = 32
    };

    static float InQuad(float t, float b, float c, float d)
    {
        if (t <= 0) {
            return b;
        }
        if (t >= d) {
            return b + c;
        }
        t /= d;
        return c * t * t + b;
    }

    static float OutQuad(float t, float b, float c, float d)
    {
        if (t <= 0) {
            return b;
        }
        if (t >= d) {
            return b + c;
        }
        t /= d;
        return (0 - c) * t * (t - 2) + b;
    }

    static float InOutQuad(float t, float b, float c, float d);
    static float OutInQuad(float t, float b, float c, float d);

    static float InCubic(float t, float b, float c, float d)
    {
        if (t <= 0) {
            return b;
        }
        if (t >= d) {
            return b + c;
        }
        t /= d;
        return c * t * t * t + b;
    }

    static float OutCubic(float t, float b, float c, float d)
    {
        if (t <= 0) {
            return b;
        }
        if (t >= d) {
            return b + c;
        }
        t = t / d - 1;
        return c * (t * t * t + 1) + b;
    }

    static float InOutCubic(float t, float b, float c, float d);
    static float OutInCubic(float t, float b, float c, float d);

    static float InQuart(float t, float b, float c, float d)
    {
        if (t <= 0) {
            return b;
        }
        if (t >= d) {
            return b + c;
        }
        t /= d;
        return c * t * t * t * t + b;
    }

    static float OutQuart(float t, float b, float c, float d)
    {
        if (t <= 0) {
            return b;
        }
        if (t >= d) {
            return b + c;
        }
        t = t / d - 1;
        return (0 - c) * (t * t * t * t - 1) + b;
    }

    static float InOutQuart(float t, float b, float c, float d);
    static float OutInQuart(float t, float b, float c, float d);

    static float InQuint(float t, float b, float c, float d)
    {
        if (t <= 0) {
            return b;
        }
        if (t >= d) {
            return b + c;
        }
        t /= d;
        return c * t * t * t * t * t + b;
    }

    static float OutQuint(float t, float b, float c, float d)
    {
        if (t <= 0) {
            return b;
        }
        if (t >= d) {
            return b + c;
        }
        t = t / d - 1;
        return c * (t * t * t * t * t + 1) + b;
    }

    static float InOutQuint(float t, float b, float c, float d);
    static float OutInQuint(float t, float b, float c, float d);

    static float InExpo(float t, float b, float c, float d);
    static float OutExpo(float t, float b, float c, float d);
    static float InOutExpo(float t, float b, float c, float d);
    static float OutInExpo(float t, float b, float c, float d);

    static float InCirc(float t, float b, float c, float d);
    static float OutCirc(float t, float b, float c, float d);
    static float InOutCirc(float t, float b, float c, float d);
    static float OutInCirc(float t, float b, float c, float d);

    /** `s` is the overshoot (1.70158 gives 10%). */
    static float InBack(float t, float b, float c, float d, float s)
    {
        if (t <= 0) {
            return b;
        }
        if (t >= d) {
            return b + c;
        }
        t /= d;
        return c * t * t * ((s + 1) * t - s) + b;
    }

    static float OutBack(float t, float b, float c, float d, float s)
    {
        if (t <= 0) {
            return b;
        }
        if (t >= d) {
            return b + c;
        }
        t = t / d - 1;
        return c * (t * t * ((s + 1) * t + s) + 1) + b;
    }

    static float InOutBack(float t, float b, float c, float d, float s);
    static float OutInBack(float t, float b, float c, float d, float s);

    static float InBounce(float t, float b, float c, float d);
    static float OutBounce(float t, float b, float c, float d);
    static float InOutBounce(float t, float b, float c, float d);
    static float OutInBounce(float t, float b, float c, float d);

    /** Evaluate `method` at time `t`; `overshoot` is only used by the Back family. */
    static float Run(EDirType direction, EMethod method, float t, float b, float c, float d, float overshoot);
    /** The curve that plays `method` backwards (In <-> Out, InOut <-> OutIn). */
    static EMethod ReverseMethod(EMethod method);
};

#endif
