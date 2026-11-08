// AppSettings.cpp - user settings of the game (sound, music, volume, controls, language ...),
// stored in Documents/gameset.dat behind the "GAMESETTINGS v1.6" header.
#include "AppSettings.h"
#include <string.h>
#include "App.h"
#include "CFileUtil.h"
#include "ICFileMgr.h"
#include "ICFile.h"
#include "CStrWChar.h"
#include "YesNoDialog.h"
#include "CDH_SoundManager.h"
#include "CGameAnalytics.h"

using namespace com::glu::platform::components;

/** Highest volume step. */
static const int MAX_VOLUME = 10;
/** Write buffer granularity. */
static const int WRITE_BUFFER_STEP = 64;

const wchar_t AppSettings::settingsFilename[] = L"gameset.dat";
const char AppSettings::settingsMarkStr[] = "GAMESETTINGS v1.6";

XString AppSettings::ReadStream::ReadString()
{
    int length = ReadInt16();
    XString text;
    text.SetSize(length * sizeof(wchar_t), 0, false);
    for (int index = 0; index < length; index++) {
        text.SetChar(index, ReadInt16());
    }
    return text;
}

AppSettings::WriteStream::~WriteStream()
{
    delete[] m_buffer;
}

int AppSettings::WriteStream::ReallocBuf(int size)
{
    if (m_failed) {
        return 0;
    }
    int capacity = (size + WRITE_BUFFER_STEP - 1) & ~(WRITE_BUFFER_STEP - 1);
    if ((unsigned int)capacity == m_capacity) {
        return 1;
    }
    unsigned char* buffer = new unsigned char[capacity];
    if (buffer == NULL) {
        m_failed = true;
        return 0;
    }
    memcpy(buffer, m_buffer, m_capacity);
    delete[] m_buffer;
    m_buffer = buffer;
    m_capacity = capacity;
    return 1;
}

void AppSettings::WriteStream::WriteInternal(const unsigned char* data, unsigned int size)
{
    if (ReallocBuf(m_size + size) == 1) {
        memcpy(m_buffer + m_size, data, size);
        m_size += size;
    }
}

bool AppSettings::WriteStream::WriteString(const XString& text)
{
    unsigned int length = text.Length();
    WriteInt16((short)length);
    for (int index = 0; index < length; index++) {
        WriteInt16(text.Char(index));
    }
    return !m_failed;
}

AppSettings::AppSettings()
{
    m_text0 = m_text1 = m_text2 = m_text3 = XString("");
    m_field30 = true;
    m_field08 = 0;
    m_volume = MAX_VOLUME;
    m_musicEnabled = true;
    m_soundEnabled = true;
    m_vibrationEnabled = false;
    m_fireButtonEnabled = true;
    m_accelerationEnabled = false;
    m_localPushEnabled = true;
    m_field1e = true;
    m_cheatsEnabled = false;
    ResetDefaults();
    if (Load() == 0) {
        ResetDefaults();
        Save();
    }
    m_playTime.Pause();
}

int AppSettings::Load()
{
    CStrWChar path;
    CFileUtil::GetApplicationDataPathForFile(path, settingsFilename);
    int result = 0;
    ICFile* file = ICFileMgr::GetInstance()->Open(path.m_string, ICFileMgr::AccessRead);
    if (file != NULL) {
        result = 0;
        file->Seek(0, ICFile::SeekEnd);
        unsigned int size = file->Tell();
        file->Seek(0, ICFile::SeekSet);
        unsigned char* data = (unsigned char*)malloc(size);
        if (data != NULL) {
            result = 0;
            if (file->Read(data, size) == size) {
                ReadStream stream;
                stream.Open(data, size);
                result = Read(&stream);
            }
            free(data);
        }
        ICFileMgr::GetInstance()->Close(file);
    }
    return result;
}

int AppSettings::Save()
{
    if (!App::IsEnoughFreeSpace() && WindowApp::m_instance->m_desktop != NULL) {
        WindowApp::AddModal(new SimpleMessage(Window::ResString("IDS_NO_FREESPACE_WARNING"), 0));
        return 0;
    }
    int result = 0;
    WriteStream stream;
    if (Write(&stream) == 1) {
        CStrWChar path;
        CFileUtil::GetApplicationDataPathForFile(path, settingsFilename);
        if (CFileUtil::SafeWriteFile(path.m_string, stream.m_buffer, stream.m_size)) {
            result = 1;
        }
    }
    return result;
}

AppSettings::~AppSettings()
{
    m_field30 = false;
    Save();
}

int AppSettings::ReadHeader(ReadStream* stream)
{
    char header[sizeof(settingsMarkStr)];
    int result = 0;
    stream->Read((unsigned char*)header, sizeof(settingsMarkStr));
    if (!stream->m_error) {
        if (strcmp(header, settingsMarkStr) == 0) {
            result = 1;
        }
    }
    return result;
}

int AppSettings::Read(ReadStream* stream)
{
    int result = 0;
    if (ReadHeader(stream) == 1) {
        m_text0 = stream->ReadString();
        m_text1 = stream->ReadString();
        m_text2 = stream->ReadString();
        m_text3 = stream->ReadString();
        m_field30 = stream->ReadUInt8() != 0;
        m_field08 = stream->ReadUInt8();
        // the play time is stored as the raw bits of the float
        int playTimeBits = stream->ReadInt32();
        m_playTime.SetTimeSec(*(float*)&playTimeBits);
        m_volume = stream->ReadInt16();
        m_soundEnabled = stream->ReadUInt8() != 0;
        m_musicEnabled = stream->ReadUInt8() != 0;
        m_vibrationEnabled = stream->ReadUInt8() != 0;
        m_fireButtonEnabled = stream->ReadUInt8() != 0;
        m_accelerationEnabled = stream->ReadUInt8() != 0;
        m_localPushEnabled = stream->ReadUInt8() != 0;
        if (m_field08 >= 0 && m_volume >= 0 && m_volume <= MAX_VOLUME) {
            m_locale = stream->ReadInt32();
            for (int index = 0; index < FLAG_COUNT; index++) {
                m_flags[index] = stream->ReadUInt8() != 0;
            }
            m_field34 = stream->ReadInt32();
            result = 1;
            if (stream->Available() > 3) {
                m_field38 = stream->ReadInt32();
            }
        }
    }
    return result;
}

int AppSettings::Write(WriteStream* stream)
{
    int result = 0;
    stream->Write((const unsigned char*)settingsMarkStr, sizeof(settingsMarkStr));
    if (!stream->m_failed) {
        stream->WriteString(m_text0);
        stream->WriteString(m_text1);
        stream->WriteString(m_text2);
        stream->WriteString(m_text3);
        stream->WriteUInt8(m_field30);
        stream->WriteUInt8((unsigned char)m_field08);
        // the play time is stored as the raw bits of the float
        float playTime = m_playTime.TimeSec();
        stream->WriteInt32(*(int*)&playTime);
        stream->WriteInt16((short)m_volume);
        stream->WriteUInt8(m_soundEnabled);
        stream->WriteUInt8(m_musicEnabled);
        stream->WriteUInt8(m_vibrationEnabled);
        stream->WriteUInt8(m_fireButtonEnabled);
        stream->WriteUInt8(m_accelerationEnabled);
        stream->WriteUInt8(m_localPushEnabled);
        stream->WriteInt32(m_locale);
        for (int index = 0; index < FLAG_COUNT; index++) {
            stream->WriteUInt8(m_flags[index]);
        }
        stream->WriteInt32(m_field34);
        stream->WriteInt32(m_field38);
        result = 0;
        if (!stream->m_failed) {
            result = 1;
        }
    }
    return result;
}

void AppSettings::SetLocale(int locale, bool save)
{
    m_locale = locale;
}

bool AppSettings::IsSoundEnabled() const
{
    return m_soundEnabled;
}

void AppSettings::SetSoundEnabled(bool enabled)
{
    m_soundEnabled = enabled;
    if (!enabled) {
        ((App*)WindowApp::m_instance)->m_soundManager->StopSounds();
    }
}

bool AppSettings::IsMusicEnabled() const
{
    return m_musicEnabled;
}

void AppSettings::SetMusicEnabled(bool enabled)
{
    m_musicEnabled = enabled;
    if (!enabled) {
        ((App*)WindowApp::m_instance)->m_soundManager->StopMusic();
    }
}

bool AppSettings::IsVibrationEnabled() const
{
    return m_vibrationEnabled;
}

void AppSettings::SetVibrationEnabled(bool enabled)
{
    m_vibrationEnabled = enabled;
}

bool AppSettings::IsFireButtonEnabled() const
{
    return m_fireButtonEnabled;
}

void AppSettings::SetFireButtonEnabled(bool enabled)
{
    m_fireButtonEnabled = enabled;
}

bool AppSettings::IsAccelerationEnabled() const
{
    return m_accelerationEnabled;
}

bool AppSettings::IsLocalPushEnabled() const
{
    return m_localPushEnabled;
}

void AppSettings::SetLocalPushEnabled(bool enabled)
{
    m_localPushEnabled = enabled;
    CGameAnalytics::logPushSwitch(enabled);
}

int AppSettings::Volume() const
{
    return m_volume;
}

void AppSettings::SetVolume(int volume)
{
    if (m_volume != volume) {
        m_volume = volume;
        if (volume < 0) {
            m_volume = 0;
        } else if (volume > MAX_VOLUME) {
            m_volume = MAX_VOLUME;
        }
    }
}

void AppSettings::SetCheatsEnabled(bool enabled)
{
    m_cheatsEnabled = enabled;
}
