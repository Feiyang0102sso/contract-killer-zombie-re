// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CSaveManager.cpp not written yet
// b078c: implementation recovered; the preceding PROVISIONAL comment is historical.
// CSaveManager.h - saves the player profile (an XML document); only what src/general uses.
#ifndef CSAVEMANAGER_H
#define CSAVEMANAGER_H

#include "tinyxml.h"
#include "xString.h"
#include "CStrWChar.h"

/** Numeric results recovered from the original save/error branches. */
enum ESaveResult {
    SAVE_WRITE_ERROR = 0,
    SAVE_CRC_ERROR = 1,
    SAVE_XML_ERROR = 2,
    SAVE_SUCCESS = 3,
    SAVE_PAUSED = 4
};

class CSaveManager {
public:
    CSaveManager();

    ESaveResult Init();
    /** Write the save file; `force` also when nothing changed. */
    // b078c: the original implementation actually ignores the force argument.
    ESaveResult Save(bool force);
    void Update();
    ESaveResult ParseData(unsigned char* data, int size, TiXmlDocument* document);
    unsigned int GetData(unsigned char** data);
    bool IsSaveValid(const com::glu::platform::components::CStrWChar& path);
    void ShowSaveErrorMessage(ESaveResult result);
    void Reset();

    /** Root node of the save document. */  // added by b055x
    TiXmlElement* GetRootElement();  // added by b055x
    /** Write the save file on the next Update(). */  // added by b055x
    void SaveOnNextUpdate(unsigned char force);  // added by b055x
    /** Stop / allow saving while GServe synchronizes. */  // added by b055x
    void PauseByGServe(unsigned char paused);  // added by b055x
    /** Stop saving while the 3D game runs. */  // added by b055x
    void PauseSaving();  // added by b055x
    void ResumeSaving();  // added by b055x

    TiXmlDocument m_document;               // 0x00 the save file
    // Former m_data48[0x54 - 0x48]: layout not known yet.
    TiXmlElement* m_root;                   // 0x48
    union {
        unsigned int m_flags;
        struct {
            bool m_pending;                // 0x4c
            unsigned char m_force;          // 0x4d
            bool m_pauseSaving;             // 0x4e
            unsigned char m_pauseGServe;    // 0x4f
        };
    };
    int m_saveState;                        // 0x50
    XString m_fileName;                     // 0x54
};

#endif
