// CFacebookMessage.mm - parameters of a Facebook feed dialog.
#include "CFacebookMessage.h"
#include <string.h>
#include "CUtil.h"
#include "FBConnect.h"

using com::glu::platform::components::CStrWChar;

CFacebookMessage::CFacebookMessage()
{
    m_message = [[NSMutableDictionary alloc] init];
    m_actionLinks = [[NSMutableArray alloc] init];
    m_attachment = [[NSMutableDictionary alloc] init];
    m_privacy = [[NSMutableDictionary alloc] init];
}

CFacebookMessage::~CFacebookMessage()
{
    if (m_privacy != nil) {
        [m_privacy release];
        m_privacy = nil;
    }
    if (m_attachment != nil) {
        [m_attachment release];
        m_attachment = nil;
    }
    if (m_actionLinks != nil) {
        [m_actionLinks release];
        m_actionLinks = nil;
    }
    if (m_message != NULL) {
        [(NSMutableDictionary*)m_message release];
        m_message = NULL;
    }
}

unsigned char CFacebookMessage::SetParam(const char* key, const wchar_t* value)
{
    unsigned char accepted = 0;
    // a prefilled user message is not allowed by Facebook
    if (strcmp(key, "message") != 0) {
        NSMutableDictionary* node = GetNode(key, 1);
        accepted = 0;
        if (node != nil) {
            NSString* name = [NSString stringWithCString:key encoding:NSASCIIStringEncoding];
            if (value == NULL) {
                [node removeObjectForKey:name];
            } else {
                [node setObject:[CUtil WcharToNSString:value] forKey:name];
            }
            accepted = 1;
        }
    }
    return accepted;
}

NSMutableDictionary* CFacebookMessage::GetNode(const char*& path, unsigned char create)
{
    NSMutableDictionary* node = (NSMutableDictionary*)m_message;
    const char* nodeName = path;
    while (true) {
        const char* separator = strchr(nodeName, '/');
        if (separator == NULL) {
            return node;
        }
        node = GetNode(path, separator - nodeName, create);
        if (node == nil) {
            break;
        }
        nodeName = separator + 1;
        path = nodeName;
    }
    return nil;
}

unsigned char CFacebookMessage::GetParam(const char* key, CStrWChar& value)
{
    unsigned char found = 0;
    NSMutableDictionary* node = GetNode(key, 0);
    if (node != nil) {
        found = 1;
        NSString* text = [node objectForKey:[NSString stringWithCString:key encoding:NSASCIIStringEncoding]];
        value = [CUtil NSStringToWchar:text];
    }
    return found;
}

NSMutableDictionary* CFacebookMessage::GetNode(const char*& path, int length, unsigned char create)
{
    const char* name = path;
    if (strncmp(name, "action_links", length) == 0) {
        NSMutableArray* links = m_actionLinks;
        if ([links count] == 0) {
            if (!create) {
                return nil;
            }
            [links addObject:[[NSMutableDictionary alloc] init]];
        }
        return [links objectAtIndex:0];
    }
    if (strncmp(name, "attachment", length) == 0) {
        return m_attachment;
    }
    if (strncmp(name, "privacy", length) == 0) {
        return m_privacy;
    }
    if (strncmp(name, "media", length) == 0) {
        NSMutableDictionary* attachment = m_attachment;
        NSMutableArray* media = [attachment objectForKey:@"media"];
        if (media == nil || [media count] == 0) {
            if (!create) {
                return nil;
            }
            PushMedia();
            media = [attachment objectForKey:@"media"];
        }
        return [media objectAtIndex:0];
    }
    return nil;
}

NSMutableDictionary* CFacebookMessage::PushMedia()
{
    NSMutableDictionary* attachment = m_attachment;
    NSMutableArray* media = [attachment objectForKey:@"media"];
    if (media == nil) {
        media = [[NSMutableArray alloc] init];
        [attachment setObject:media forKey:@"media"];
    }
    NSMutableDictionary* entry = [[NSMutableDictionary alloc] init];
    [media addObject:entry];
    return entry;
}

void CFacebookMessage::PopMedia()
{
    NSMutableArray* media = [m_attachment objectForKey:@"media"];
    if (media != nil && [media count] != 0) {
        [media removeLastObject];
        if ([media count] == 0) {
            [m_attachment removeObjectForKey:@"media"];
        }
    }
}

void* CFacebookMessage::GetMessage()
{
    FB_SBJSON* writer = [[FB_SBJSON new] autorelease];

    NSString* actionLinks = nil;
    if ([m_actionLinks count] != 0) {
        actionLinks = [writer stringWithObject:m_actionLinks];
    }
    NSString* attachment = nil;
    if ([m_attachment count] != 0) {
        attachment = [writer stringWithObject:m_attachment];
    }
    NSString* privacy = nil;
    if ([m_privacy count] != 0) {
        privacy = [writer stringWithObject:m_privacy];
    }

    NSMutableDictionary* params = [NSMutableDictionary dictionaryWithDictionary:(NSMutableDictionary*)m_message];
    if (actionLinks != nil) {
        [params setObject:actionLinks forKey:@"link"];
    }
    if (attachment != nil) {
        [params setObject:attachment forKey:@"picture"];
    }
    if (privacy != nil) {
        [params setObject:privacy forKey:@"privacy"];
    }
    return params;
}

CStrWChar CFacebookMessage::GetTargetUserId()
{
    CStrWChar targetId;
    GetParam("target_id", targetId);
    return targetId;
}

void CFacebookMessage::SetText(const wchar_t* message, const wchar_t* targetId, const wchar_t* prompt)
{
    SetParam("message", message);
    if (targetId != NULL) {
        SetParam("target_id", targetId);
    }
    if (prompt != NULL) {
        SetParam("user_message_prompt", prompt);
    }
}

void CFacebookMessage::SetActionLInk(const wchar_t* text, const wchar_t* href)
{
    SetParam("action_link/text", text);
    SetParam("action_link/href", href);
}

void CFacebookMessage::SetAttachment(const wchar_t* name, const wchar_t* description, const wchar_t* href, const wchar_t* caption)
{
    SetParam("attachment/name", name);
    SetParam("attachment/description", description);
    SetParam("attachment/href", href);
    if (caption != NULL) {
        SetParam("attachment/caption", caption);
    }
}

void CFacebookMessage::SetAttachmentImage(const wchar_t* source, const wchar_t* href)
{
    SetParam("attachment/media/type", L"image");
    SetParam("attachment/media/src", source);
    SetParam("attachment/media/href", href);
}
