// CAppConfig.h - start-up configuration of the application: graphics init parameters, audio
// output format and the names of the properties file and the resource archive.
#ifndef CAPPCONFIG_H
#define CAPPCONFIG_H

#include <stddef.h>
#include "CStrWChar.h"
#include "ICGraphics.h"

namespace com { namespace glu { namespace platform { namespace framework {

class CAppConfig {
public:
    CAppConfig()
    {
        m_graphicsParams = NULL;
    }

    ~CAppConfig();

    /**
     * Set every field. `graphicsParams` is a list ended by a key 0; it is copied (one entry
     * past the terminator is copied too, as the original does).
     */
    void Copy(unsigned int graphicsMode, const graphics::ICGraphics::InitParam* graphicsParams,
              unsigned int audioChannels, unsigned int audioBitsPerSample, unsigned int audioSampleRate,
              unsigned int audioBufferLength, unsigned int audioMaxSounds, unsigned char audioSoftwareMixer,
              const components::CStrWChar& propertiesFile, unsigned char flag30,
              const components::CStrWChar& resourceFile)
    {
        m_graphicsMode = graphicsMode;
        if (m_graphicsParams != NULL) {
            delete[] m_graphicsParams;
            m_graphicsParams = NULL;
        }
        if (graphicsParams != NULL) {
            // count up to and including the terminating entry
            int last = 0;
            while (graphicsParams[last++].m_key != 0) {
            }
#ifdef CKZ_PC
            // PORT: The terminator is the last entry the caller supplied.
            m_graphicsParams = new graphics::ICGraphics::InitParam[last];
            for (int index = 0; index < last; index++) {
                m_graphicsParams[index] = graphicsParams[index];
            }
#else
            m_graphicsParams = new graphics::ICGraphics::InitParam[last + 1];
            int index = last;
            do {
                m_graphicsParams[index] = graphicsParams[index];
                index--;
            } while (index >= 0);
#endif
        } else {
            m_graphicsParams = NULL;
        }
        m_audioChannels = audioChannels;
        m_audioBitsPerSample = audioBitsPerSample;
        m_audioSampleRate = audioSampleRate;
        m_audioBufferLength = audioBufferLength;
        m_audioMaxSounds = audioMaxSounds;
        m_audioSoftwareMixer = audioSoftwareMixer;
        m_propertiesFile = propertiesFile;
        m_flag30 = flag30;
        m_resourceFile = resourceFile;
    }

    unsigned int m_graphicsMode;                            // 0x00 (5 by default)
    graphics::ICGraphics::InitParam* m_graphicsParams;      // 0x04 owned copy, NULL when none
    unsigned int m_audioChannels;                           // 0x08
    unsigned int m_audioBitsPerSample;                      // 0x0c
    unsigned int m_audioSampleRate;                         // 0x10
    unsigned int m_audioBufferLength;                       // 0x14
    unsigned int m_audioMaxSounds;                          // 0x18
    unsigned char m_audioSoftwareMixer;                     // 0x1c
    components::CStrWChar m_propertiesFile;                 // 0x20 "properties.dat"
    unsigned char m_flag30;                                 // 0x30 meaning unknown (default left uninitialized)
    components::CStrWChar m_resourceFile;                   // 0x34 "<app>.big"
};

inline CAppConfig::~CAppConfig()
{
    delete[] m_graphicsParams;
}

} } } }

#endif
