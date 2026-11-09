// Use the Windows system font for dynamic text. The original bitmap fonts are unchanged.
#include "CSystemFont_Cocoa.h"
#include "CRenderSurfaceBuffer.h"
#include "ICRenderSurface.h"
#include <windows.h>
#include <wchar.h>

using namespace com::glu::platform::graphics;

namespace {

HFONT NativeFont(UIFont* font) { return reinterpret_cast<HFONT>(font); }

int TextLength(const wchar_t* text, int length)
{
    const int available = static_cast<int>(wcslen(text));
    if (length < 0 || length > available) {
        return available;
    }
    return length;
}

} // namespace

CSystemFont_Cocoa::CSystemFont_Cocoa() : m_font(nullptr) {}
CSystemFont_Cocoa::~CSystemFont_Cocoa() { Destroy(); }
void CSystemFont_Cocoa::Destroy()
{
    if (m_font != nullptr) {
        DeleteObject(NativeFont(m_font));
        m_font = nullptr;
    }
    CSystemFont::Destroy();
}

unsigned char CSystemFont_Cocoa::FinalizeInitialization(const unsigned char* data, unsigned int size)
{
    if (!Parse(data, size)) {
        return 0;
    }
    const wchar_t* name = L"Segoe UI";
    if (m_fontName.m_length != 0) {
        name = m_fontName.m_string;
    }
    DWORD quality = NONANTIALIASED_QUALITY;
    if (m_antialias) {
        quality = ANTIALIASED_QUALITY;
    }
    m_font = reinterpret_cast<UIFont*>(CreateFontW(-m_pointSize, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        quality, DEFAULT_PITCH, name));
    return m_font != nullptr;
}

int CSystemFont_Cocoa::GetFontHeight() const
{
    if (m_font == nullptr) {
        return 0;
    }
    return m_pointSize;
}

int CSystemFont_Cocoa::GetMinLineSpacing() const { return 0; }
int CSystemFont_Cocoa::GetMinCharSpacing() const { return m_pointSize / 10; }

int CSystemFont_Cocoa::GetWidth(wchar_t character) const
{
    if (m_font == nullptr) {
        return 0;
    }
    HDC dc = CreateCompatibleDC(nullptr);
    SelectObject(dc, NativeFont(m_font));
    SIZE size = {};
    GetTextExtentPoint32W(dc, &character, 1, &size);
    DeleteDC(dc);
    return size.cx;
}

int CSystemFont_Cocoa::MeasureTextWidth(const wchar_t* text, int length, int maxWidth, int* fitCount)
{
    if (fitCount != nullptr) {
        *fitCount = 0;
    }
    if (m_font == nullptr || text == nullptr) {
        return 0;
    }
    length = TextLength(text, length);
    HDC dc = CreateCompatibleDC(nullptr);
    SelectObject(dc, NativeFont(m_font));
    int fit = length;
    SIZE size = {};
    if (maxWidth >= 0) {
        GetTextExtentExPointW(dc, text, length, maxWidth, &fit, nullptr, &size);
    }
    GetTextExtentPoint32W(dc, text, fit, &size);
    DeleteDC(dc);
    if (fitCount != nullptr) {
        *fitCount = fit;
    }
    return size.cx;
}

unsigned char CSystemFont_Cocoa::InitializeSurfaceWithText(ICRenderSurface& surface, const wchar_t* text,
                                                           int length, int maxWidth, int maxHeight)
{
    if (m_font == nullptr || text == nullptr) {
        return 0;
    }

    int width = MeasureTextWidth(text, length, -1, nullptr);
    int height = GetFontHeight();
    if (maxWidth >= 0 && width > maxWidth) {
        width = maxWidth;
    }
    if (maxHeight >= 0 && height > maxHeight) {
        height = maxHeight;
    }
    if (width <= 0 || height <= 0) {
        return 0;
    }

    const ICRenderSurface::InitParam params[] = {
        { ICRenderSurface::InitParam_Width, static_cast<unsigned int>(width) },
        { ICRenderSurface::InitParam_Height, static_cast<unsigned int>(height) },
        { ICRenderSurface::InitParam_ColorFormat, RSBFormat_A8B8G8R8 },
        { 0, 0 }
    };
    if (!surface.Initialize(params)) {
        return 0;
    }

    HDC dc = CreateCompatibleDC(nullptr);
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    void* pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (bitmap == nullptr) {
        DeleteDC(dc);
        return 0;
    }
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    SelectObject(dc, NativeFont(m_font));
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkColor(dc, RGB(0, 0, 0));
    TextOutW(dc, 0, 0, text, TextLength(text, length));
    GdiFlush();

    unsigned char* destination = nullptr;
    int pitch = 0;
    surface.Lock(&destination, pitch, ICRenderSurface::LockType_ReadWrite, nullptr);
    if (destination != nullptr) {
        const auto* source = static_cast<const unsigned char*>(pixels);
        for (int row = 0; row < height; row++) {
            auto* target = destination + row * pitch;
            for (int col = 0; col < width; col++) {
                target[0] = 255;
                target[1] = 255;
                target[2] = 255;
                target[3] = source[(row * width + col) * 4];
                target += 4;
            }
        }
        surface.Unlock();
    }
    SelectObject(dc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return destination != nullptr;
}
