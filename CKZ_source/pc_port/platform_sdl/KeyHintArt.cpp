// Key hint badges drawn in memory: a procedural blood splat, the key name through Windows GDI
// (white with a black outline) or a small mouse icon. The game only gets the finished RGBA pixels.
#include "KeyHintArt.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <random>

namespace ckz {
namespace {

// ---- Look of the badge (tune here) ----
constexpr int Supersample = 4;            // samples per pixel edge (anti-aliasing)
constexpr float Pi = 3.14159265f;
// Splat: a wobbly blob in the middle, a few streaks flung outwards and loose drops around it.
constexpr float BlobRadius = 0.27f;       // part of the badge edge
constexpr int FirstHarmonic = 3;
constexpr int HarmonicCount = 5;          // harmonics 3..7 wobble the blob edge
constexpr int MaxStreaks = 4;
constexpr int MaxDrops = 9;
constexpr float SplatOpacity = 0.92f;
// Blood colour: the rim is bright red, the core this much darker; the darkening fades out
// CoreReach blob radii from the centre.
constexpr float RimRed = 170.0f;
constexpr float CoreRedDrop = 90.0f;
constexpr float RimGreen = 8.0f;
constexpr float RimBlue = 6.0f;
constexpr float CoreReach = 1.3f;
// Label: Arial Black, height in pixels by the label length, with a black outline.
const wchar_t* const LabelFontName = L"Arial Black";
constexpr int SingleLetterHeight = 22;
constexpr int ShortLabelHeight = 14;      // two to four letters
constexpr int LongLabelHeight = 11;       // five letters or more (SHIFT)
constexpr int OutlineWidth = 2;
// Mouse icon: body size as parts of the badge edge; the buttons take the top part of the body.
constexpr float MouseWidth = 0.40f;
constexpr float MouseHeight = 0.56f;
constexpr float MouseButtonPart = 0.42f;
constexpr float MouseLineWidth = 2.3f;    // black outline and button dividers, in pixels
constexpr float MouseBodyGrey = 30.0f;
constexpr float MouseBodyAlpha = 0.9f;

struct Streak {
    float angle;
    float length;
    float width;  // at the blob centre; the streak narrows to nothing at its end
};

struct Drop {
    float x;
    float y;
    float radius;
};

/** Random shape of one splat, in badge pixels. */
struct SplatShape {
    float centre;
    float radius;
    float amplitudes[HarmonicCount];
    float phases[HarmonicCount];
    Streak streaks[MaxStreaks];
    int streakCount;
    Drop drops[MaxDrops];
    int dropCount;
};

float Uniform(std::mt19937& random, float low, float high)
{
    std::uniform_real_distribution<float> distribution(low, high);
    return distribution(random);
}

int UniformInt(std::mt19937& random, int low, int high)
{
    std::uniform_int_distribution<int> distribution(low, high);
    return distribution(random);
}

unsigned char ToByte(float value)
{
    if (value <= 0.0f) {
        return 0;
    }
    if (value >= 255.0f) {
        return 255;
    }
    return static_cast<unsigned char>(value + 0.5f);
}

/** Composite a straight-alpha colour (alpha 0..1) over pixel `index` of an RGBA picture. */
void BlendOver(std::vector<unsigned char>& pixels, size_t index, float red, float green, float blue, float alpha)
{
    if (alpha <= 0.0f) {
        return;
    }
    unsigned char* pixel = &pixels[index * 4];
    const float below = pixel[3] / 255.0f;
    const float belowWeight = below * (1.0f - alpha);
    const float outAlpha = alpha + belowWeight;
    pixel[0] = ToByte((red * alpha + pixel[0] * belowWeight) / outAlpha);
    pixel[1] = ToByte((green * alpha + pixel[1] * belowWeight) / outAlpha);
    pixel[2] = ToByte((blue * alpha + pixel[2] * belowWeight) / outAlpha);
    pixel[3] = ToByte(outAlpha * 255.0f);
}

SplatShape MakeSplatShape(int seed)
{
    std::mt19937 random(static_cast<unsigned int>(seed));
    SplatShape shape = {};
    shape.centre = KeyBadgeSize / 2.0f;
    shape.radius = KeyBadgeSize * BlobRadius;
    for (int harmonic = 0; harmonic < HarmonicCount; harmonic++) {
        shape.amplitudes[harmonic] = Uniform(random, 0.02f, 0.09f);
        shape.phases[harmonic] = Uniform(random, 0.0f, 2.0f * Pi);
    }
    shape.streakCount = UniformInt(random, 2, MaxStreaks);
    for (int index = 0; index < shape.streakCount; index++) {
        Streak& streak = shape.streaks[index];
        streak.angle = Uniform(random, 0.0f, 2.0f * Pi);
        streak.length = shape.radius * Uniform(random, 1.25f, 1.6f);
        streak.width = shape.radius * Uniform(random, 0.08f, 0.16f);
    }
    shape.dropCount = UniformInt(random, 5, MaxDrops);
    for (int index = 0; index < shape.dropCount; index++) {
        const float angle = Uniform(random, 0.0f, 2.0f * Pi);
        const float distance = shape.radius * Uniform(random, 1.15f, 1.6f);
        Drop& drop = shape.drops[index];
        drop.x = shape.centre + cosf(angle) * distance;
        drop.y = shape.centre + sinf(angle) * distance;
        drop.radius = shape.radius * Uniform(random, 0.05f, 0.13f);
    }
    return shape;
}

bool InsideCircle(float x, float y, float centreX, float centreY, float radius)
{
    const float dx = x - centreX;
    const float dy = y - centreY;
    return dx * dx + dy * dy <= radius * radius;
}

bool InsideSplat(const SplatShape& shape, float x, float y)
{
    const float dx = x - shape.centre;
    const float dy = y - shape.centre;
    // The blob: its radius wobbles with the angle.
    const float angle = atan2f(dy, dx);
    float scale = 1.0f;
    for (int harmonic = 0; harmonic < HarmonicCount; harmonic++) {
        scale += shape.amplitudes[harmonic] * sinf((FirstHarmonic + harmonic) * angle + shape.phases[harmonic]);
    }
    if (dx * dx + dy * dy <= shape.radius * scale * shape.radius * scale) {
        return true;
    }
    // Streaks: wedges from the centre, each ending in a drop.
    for (int index = 0; index < shape.streakCount; index++) {
        const Streak& streak = shape.streaks[index];
        const float directionX = cosf(streak.angle);
        const float directionY = sinf(streak.angle);
        const float along = dx * directionX + dy * directionY;
        const float across = fabsf(dy * directionX - dx * directionY);
        if (along >= 0.0f && along <= streak.length) {
            const float halfWidth = streak.width * (1.0f - along / streak.length);
            if (across <= halfWidth) {
                return true;
            }
        }
        const float tipX = shape.centre + directionX * streak.length;
        const float tipY = shape.centre + directionY * streak.length;
        if (InsideCircle(x, y, tipX, tipY, streak.width * 0.9f)) {
            return true;
        }
    }
    for (int index = 0; index < shape.dropCount; index++) {
        const Drop& drop = shape.drops[index];
        if (InsideCircle(x, y, drop.x, drop.y, drop.radius)) {
            return true;
        }
    }
    return false;
}

/** An RGBA picture holding only the splat of `seed`. */
std::vector<unsigned char> PaintSplat(int seed)
{
    const SplatShape shape = MakeSplatShape(seed);
    std::vector<unsigned char> pixels(static_cast<size_t>(KeyBadgeSize) * KeyBadgeSize * 4, 0);
    const int samples = Supersample * Supersample;
    for (int y = 0; y < KeyBadgeSize; y++) {
        for (int x = 0; x < KeyBadgeSize; x++) {
            int inside = 0;
            for (int sampleY = 0; sampleY < Supersample; sampleY++) {
                for (int sampleX = 0; sampleX < Supersample; sampleX++) {
                    const float pointX = x + (sampleX + 0.5f) / Supersample;
                    const float pointY = y + (sampleY + 0.5f) / Supersample;
                    if (InsideSplat(shape, pointX, pointY)) {
                        inside++;
                    }
                }
            }
            if (inside == 0) {
                continue;
            }
            // Darker towards the centre, like dried blood.
            const float dx = x + 0.5f - shape.centre;
            const float dy = y + 0.5f - shape.centre;
            float depth = 1.0f - sqrtf(dx * dx + dy * dy) / (shape.radius * CoreReach);
            if (depth < 0.0f) {
                depth = 0.0f;
            }
            const float coverage = static_cast<float>(inside) / samples;
            const size_t index = static_cast<size_t>(y) * KeyBadgeSize + x;
            BlendOver(pixels, index, RimRed - CoreRedDrop * depth, RimGreen * (1.0f - depth),
                      RimBlue * (1.0f - depth), coverage * SplatOpacity);
        }
    }
    return pixels;
}

/**
 * Coverage (0..255, one byte per pixel) of `text` centred in the badge. With `spread` > 0 the text
 * is drawn at every offset within that radius, which gives the shape of its outline.
 */
std::vector<unsigned char> TextCoverage(const std::wstring& text, int height, int spread)
{
    std::vector<unsigned char> coverage(static_cast<size_t>(KeyBadgeSize) * KeyBadgeSize, 0);
    HDC dc = CreateCompatibleDC(nullptr);
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = KeyBadgeSize;
    info.bmiHeader.biHeight = -KeyBadgeSize;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (bitmap == nullptr) {
        fprintf(stderr, "Key hints: text bitmap could not be created\n");
        DeleteDC(dc);
        return coverage;
    }
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    HFONT font = CreateFontW(-height, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, LabelFontName);
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkMode(dc, TRANSPARENT);

    // Centre the capital letters: they reach from the internal leading down to the ascent.
    TEXTMETRICW metrics = {};
    GetTextMetricsW(dc, &metrics);
    SIZE extent = {};
    const int length = static_cast<int>(text.size());
    GetTextExtentPoint32W(dc, text.c_str(), length, &extent);
    const int left = (KeyBadgeSize - extent.cx) / 2;
    const int top = KeyBadgeSize / 2 - (metrics.tmInternalLeading + metrics.tmAscent) / 2;
    for (int dy = -spread; dy <= spread; dy++) {
        for (int dx = -spread; dx <= spread; dx++) {
            if (dx * dx + dy * dy > spread * spread + 1) {
                continue;
            }
            TextOutW(dc, left + dx, top + dy, text.c_str(), length);
        }
    }
    GdiFlush();

    // White text on black: the red channel (byte 2 of BGRA) is the coverage.
    const auto* source = static_cast<const unsigned char*>(bits);
    for (size_t pixel = 0; pixel < coverage.size(); pixel++) {
        coverage[pixel] = source[pixel * 4 + 2];
    }
    SelectObject(dc, oldFont);
    DeleteObject(font);
    SelectObject(dc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return coverage;
}

int LabelHeight(const std::string& label)
{
    if (label.size() <= 1) {
        return SingleLetterHeight;
    }
    if (label.size() <= 4) {
        return ShortLabelHeight;
    }
    return LongLabelHeight;
}

/** True when the point lies in the rectangle with rounded corners of `radius`. */
bool InsideRoundedRect(float x, float y, float left, float top, float right, float bottom, float radius)
{
    // Distance to the nearest point of the inner rectangle (the corners' centres).
    float nearestX = x;
    if (nearestX < left + radius) {
        nearestX = left + radius;
    }
    if (nearestX > right - radius) {
        nearestX = right - radius;
    }
    float nearestY = y;
    if (nearestY < top + radius) {
        nearestY = top + radius;
    }
    if (nearestY > bottom - radius) {
        nearestY = bottom - radius;
    }
    return InsideCircle(x, y, nearestX, nearestY, radius);
}

/** Colour of the mouse icon at one point; alpha 0 outside of it. */
struct IconColour {
    float grey;
    float alpha;
};

IconColour MouseColourAt(float x, float y, bool rightButton)
{
    const float width = KeyBadgeSize * MouseWidth;
    const float height = KeyBadgeSize * MouseHeight;
    const float left = (KeyBadgeSize - width) / 2.0f;
    const float top = (KeyBadgeSize - height) / 2.0f;
    const float right = left + width;
    const float bottom = top + height;
    const float radius = width / 2.0f;
    const float line = MouseLineWidth;
    const float splitY = top + height * MouseButtonPart;
    const float middleX = left + width / 2.0f;

    IconColour black = { 0.0f, 1.0f };
    IconColour white = { 255.0f, 1.0f };
    IconColour body = { MouseBodyGrey, MouseBodyAlpha };
    IconColour none = { 0.0f, 0.0f };
    if (!InsideRoundedRect(x, y, left, top, right, bottom, radius)) {
        return none;
    }
    if (!InsideRoundedRect(x, y, left + line, top + line, right - line, bottom - line, radius - line)) {
        return black;
    }
    if (fabsf(y - splitY) <= line / 2.0f) {
        return black;
    }
    if (y > splitY) {
        return body;
    }
    if (fabsf(x - middleX) <= line / 2.0f) {
        return black;
    }
    bool lit = x < middleX;
    if (rightButton) {
        lit = x > middleX;
    }
    if (lit) {
        return white;
    }
    return body;
}

} // namespace

std::vector<unsigned char> MakeKeyBadge(const std::string& label, int seed)
{
    std::vector<unsigned char> pixels = PaintSplat(seed);
    const std::wstring text(label.begin(), label.end());
    const int height = LabelHeight(label);
    const std::vector<unsigned char> outline = TextCoverage(text, height, OutlineWidth);
    const std::vector<unsigned char> fill = TextCoverage(text, height, 0);
    for (size_t index = 0; index < fill.size(); index++) {
        BlendOver(pixels, index, 0.0f, 0.0f, 0.0f, outline[index] / 255.0f);
        BlendOver(pixels, index, 255.0f, 255.0f, 255.0f, fill[index] / 255.0f);
    }
    return pixels;
}

std::vector<unsigned char> MakeMouseBadge(bool rightButton, int seed)
{
    std::vector<unsigned char> pixels = PaintSplat(seed);
    const int samples = Supersample * Supersample;
    for (int y = 0; y < KeyBadgeSize; y++) {
        for (int x = 0; x < KeyBadgeSize; x++) {
            // Average the samples with their alpha as weight.
            float greySum = 0.0f;
            float alphaSum = 0.0f;
            for (int sampleY = 0; sampleY < Supersample; sampleY++) {
                for (int sampleX = 0; sampleX < Supersample; sampleX++) {
                    const float pointX = x + (sampleX + 0.5f) / Supersample;
                    const float pointY = y + (sampleY + 0.5f) / Supersample;
                    const IconColour colour = MouseColourAt(pointX, pointY, rightButton);
                    greySum += colour.grey * colour.alpha;
                    alphaSum += colour.alpha;
                }
            }
            if (alphaSum <= 0.0f) {
                continue;
            }
            const float grey = greySum / alphaSum;
            const size_t index = static_cast<size_t>(y) * KeyBadgeSize + x;
            BlendOver(pixels, index, grey, grey, grey, alphaSum / samples);
        }
    }
    return pixels;
}

} // namespace ckz
