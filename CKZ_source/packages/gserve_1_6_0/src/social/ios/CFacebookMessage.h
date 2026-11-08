// CFacebookMessage.h - Facebook feed post: top level parameters plus the "action_links",
// "attachment" (with "media") and "privacy" sub objects, JSON encoded by GetMessage().
// Owned by social/ios/CFacebookMessage.mm (b027, written by b027x). sizeof 0x14.
#ifndef CFACEBOOKMESSAGE_H
#define CFACEBOOKMESSAGE_H

#include "CNGSSocialMessage.h"

// The header is also included by C++ units (CNGSSocialMessage.cpp): Foundation classes are opaque there.
#ifdef __OBJC__
@class NSMutableArray;
@class NSMutableDictionary;
#else
class NSMutableArray;
class NSMutableDictionary;
#endif

class CFacebookMessage : public CNGSSocialMessage {
public:
    CFacebookMessage();
    virtual ~CFacebookMessage();

    /** "message" is refused (Facebook does not allow a prefilled message); paths "node/key". */
    virtual unsigned char SetParam(const char* key, const wchar_t* value);
    virtual unsigned char GetParam(const char* key, com::glu::platform::components::CStrWChar& value);
    /** Autoreleased dialog parameters: the top level ones plus the JSON of the sub objects. */
    virtual void* GetMessage();
    virtual com::glu::platform::components::CStrWChar GetTargetUserId();
    virtual void SetText(const wchar_t* message, const wchar_t* targetId, const wchar_t* prompt);
    virtual void SetActionLInk(const wchar_t* text, const wchar_t* href);
    virtual void SetAttachment(const wchar_t* name, const wchar_t* description, const wchar_t* href, const wchar_t* caption);
    virtual void SetAttachmentImage(const wchar_t* source, const wchar_t* href);
    /** Append an empty media entry to the attachment and return it. */
    virtual NSMutableDictionary* PushMedia();
    /** Remove the last media entry (and "media" once empty). */
    virtual void PopMedia();

    /** Walk the "a/b/" prefix of `path`; `path` is left at the leaf key. NULL when a node is missing. */
    NSMutableDictionary* GetNode(const char*& path, unsigned char create);
    /** Sub object named by the first `length` characters of `path`. */
    NSMutableDictionary* GetNode(const char*& path, int length, unsigned char create);

    NSMutableArray* m_actionLinks;          // +0x08 one dictionary
    NSMutableDictionary* m_attachment;      // +0x0c
    NSMutableDictionary* m_privacy;         // +0x10
};

#endif
