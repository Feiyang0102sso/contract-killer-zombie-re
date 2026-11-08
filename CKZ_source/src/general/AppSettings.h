// AppSettings.h - user settings of the game (sound, music, volume, controls, language ...),
// stored in Documents/gameset.dat behind the "GAMESETTINGS v1.6" header.
#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "CArrayInputStream.h"
#include "COutputStream.h"
#include "TimeValue.h"
#include "xString.h"

class AppSettings {
public:
    /** Settings file reader: an array stream with a string reader. */
    class ReadStream : public com::glu::platform::components::CArrayInputStream {
    public:
        /** A string stored as a 16-bit length and 16-bit characters. */
        XString ReadString();
    };

    /** Settings file writer: grows an in-memory buffer in 64-byte steps. */
    class WriteStream : public com::glu::platform::components::COutputStream {
    public:
        WriteStream()
        {
            m_buffer = NULL;
            m_capacity = 0;
            m_size = 0;
            m_failed = false;
        }

        virtual ~WriteStream();

        /** Make room for `size` bytes; 0 once an allocation failed. */
        int ReallocBuf(int size);
        virtual void WriteInternal(const unsigned char* data, unsigned int size);
        /** Write a string as a 16-bit length and 16-bit characters; false once a write failed. */
        bool WriteString(const XString& text);

        unsigned char* m_buffer;    // 0x14
        unsigned int m_capacity;    // 0x18 multiple of 64
        unsigned int m_size;        // 0x1c bytes written
        bool m_failed;              // 0x20 out of memory
    };

    /** Number of per-profile flags stored in the file. */
    enum { FLAG_COUNT = 3 };

    AppSettings();
    ~AppSettings();

    /** Read gameset.dat; 1 when it was valid. */
    int Load();
    /** Write gameset.dat (warns when the disk is full); 1 on success. */
    int Save();
    int ReadHeader(ReadStream* stream);
    int Read(ReadStream* stream);
    int Write(WriteStream* stream);

    void SetLocale(int locale, bool save);
    bool IsSoundEnabled() const;
    void SetSoundEnabled(bool enabled);
    bool IsMusicEnabled() const;
    void SetMusicEnabled(bool enabled);
    bool IsVibrationEnabled() const;
    void SetVibrationEnabled(bool enabled);
    bool IsFireButtonEnabled() const;
    void SetFireButtonEnabled(bool enabled);
    bool IsAccelerationEnabled() const;
    bool IsLocalPushEnabled() const;
    void SetLocalPushEnabled(bool enabled);
    int Volume() const;
    void SetVolume(int volume);
    void SetCheatsEnabled(bool enabled);

    static const wchar_t settingsFilename[];    // L"gameset.dat"
    static const char settingsMarkStr[];        // "GAMESETTINGS v1.6"

    bool m_cheatsEnabled;           // 0x00
    int m_locale;                   // 0x04 -1: system language
    int m_field08;                  // 0x08 stored as a byte
    TimeValue m_playTime;           // 0x0c
    int m_volume;                   // 0x14 0..10
    bool m_soundEnabled;            // 0x18
    bool m_musicEnabled;            // 0x19
    bool m_vibrationEnabled;        // 0x1a
    bool m_fireButtonEnabled;       // 0x1b
    bool m_accelerationEnabled;     // 0x1c
    bool m_localPushEnabled;        // 0x1d
    bool m_field1e;                 // 0x1e not stored
    XString m_text0;                // 0x20 stored strings (meaning not known)
    XString m_text1;                // 0x24
    XString m_text2;                // 0x28
    XString m_text3;                // 0x2c
    bool m_field30;                 // 0x30 stored; cleared by the destructor
    bool m_flags[FLAG_COUNT];       // 0x31
    int m_field34;                  // 0x34
    int m_field38;                  // 0x38 100 by default (stored only by newer files)

private:
    /** Defaults of the values that are reset when the file is missing or invalid. */
    void ResetDefaults()
    {
        m_locale = -1;
        for (int index = 0; index < FLAG_COUNT; index++) {
            m_flags[index] = false;
        }
        m_flags[0] = true;
        m_field34 = 0;
        m_field38 = 100;
    }
};

#endif
