// CSwerveLoader.cpp - loads M3G files (and plain images) into Swerve objects, resolving external
// references (XREFs) through the Glu file system and resource manager.
#include <stdarg.h>
#include <string.h>
#include <wchar.h>
#include "CSwerveLoader.h"
#include "CSwerve.h"
#include "ICFileMgr.h"
#include "CFileUtil.h"
#include "CFileInputStream.h"
#include "CStrChar.h"
#include "CStrCharBuffer.h"
#include "CStrWCharBuffer.h"
#include "CUtf.h"
#include "CStdUtil_Cocoa.h"

using namespace com::glu::platform;

// XREF kinds of the loader.
static const int XREFType_ByUserID = 0xfd;      // the referenced objects are picked by user id
static const int XREFType_Image = 0xfe;         // an image (format from GetXREFImageFormat)

static void malij297_ErrorMessage(const char* format, ...);
static components::CStrChar WcsToStr(const wchar_t* text);

components::CInputStream* CSwerveLoader_getResourceAsStream(const wchar_t* uri)
{
    components::ICFileMgr* fileMgr = components::ICFileMgr::GetInstance();
    components::CStrWChar path(uri);
    unsigned char exists;
    if (path.Find(fileMgr->GetApplicationPath(), 0) == 0 || path.Find(fileMgr->GetApplicationDataPath(), 0) == 0) {
        exists = fileMgr->Exists(path.m_string);
    } else {
        // a bare file name: look in the application, then in the data directory
        components::CStrWChar fullPath;
        components::CFileUtil::GetApplicationPathForFile(fullPath, path.m_string);
        exists = fileMgr->Exists(fullPath.m_string);
        if (exists) {
            path = fullPath;
        } else {
            components::CFileUtil::GetApplicationDataPathForFile(fullPath, path.m_string);
            unsigned char existsInData = fileMgr->Exists(fullPath.m_string);
            exists = 0;
            if (existsInData) {
                path = fullPath;
                exists = existsInData;
            }
        }
    }

    components::CInputStream* stream;
    if (exists) {
        components::CFileInputStream* file = new components::CFileInputStream();
        if (file != NULL && !file->Open(path.m_string)) {
            malij297_ErrorMessage("Failed to open stream for resource file [%s].", WcsToStr(path.m_string).m_string);
            delete file;
            file = NULL;
        }
        stream = file;
    } else {
        // a resource called like the file without directory and extension
        const wchar_t* fileName = NULL;
        components::CStrWChar resourceName;
        if (uri != NULL) {
            const wchar_t* separator = wcsrchr(uri, L'/');
            if (separator == NULL) {
                separator = wcsrchr(uri, L'\\');
            }
            fileName = separator + 1;
            if (separator == NULL) {
                fileName = uri;
            }
            const wchar_t* extension = wcschr(fileName, L'.');
            if (extension != NULL) {
                components::CStrWChar name(fileName);
                resourceName = name.GetSubString(0, (int)(extension - fileName) - 1);
            } else {
                resourceName = fileName;
            }
        } else {
            resourceName = fileName;
        }
        CResourceInputStream* resource = new CResourceInputStream();
        stream = NULL;
        if (resource != NULL) {
            if (!resource->Open(resourceName.m_string)) {
                stream = NULL;
                delete resource;
                malij297_ErrorMessage("Failed to open stream for resource handle [%s].",
                                      WcsToStr(resourceName.m_string).m_string);
            } else {
                stream = resource;
            }
        }
    }
    if (stream == NULL) {
        malij297_ErrorMessage("Failed to getResourceAsStream([%s])", WcsToStr(uri).m_string);
    }
    return stream;
}

/** Append a printf-style message to the runtime error text. */
static void malij297_ErrorMessage(const char* format, ...)
{
    char message[256];
    va_list arguments;
    va_start(arguments, format);
    CStdUtil_Cocoa::VSPrintF_S(message, sizeof(message), format, arguments);
    va_end(arguments);
    malij297_ErrorMessageAppend(message);
}

/** ASCII copy of `text` for messages (other characters become '.'). */
static components::CStrChar WcsToStr(const wchar_t* text)
{
    components::CStrCharBuffer buffer;
    if (text != NULL) {
        for (const wchar_t* cursor = text; *cursor != 0; cursor++) {
            if (*cursor < 0x80) {
                buffer.Append((char)*cursor);
            } else {
                buffer.Append('.');
            }
        }
    }
    return components::CStrChar(buffer.m_string);
}

bool CSwerveLoader::isRelative(const components::CStrWChar& uri)
{
    int remaining = uri.m_length;
    if (remaining == 0) {
        return true;
    }
    const wchar_t* cursor = uri.m_string;
    wchar_t character = *cursor;
    if (remaining >= 2) {
        if (character == L'/') {
            // "//authority" is relative to the scheme only
            if (cursor[1] != L'/') {
                return false;
            }
            return true;
        }
    } else if (character == L'/') {
        return false;
    }
    // a scheme ends with ':' before any of "/?#"
    while (true) {
        remaining--;
        cursor++;
        if (character == L'#' || character == L'/' || character == L':' || character == L'?' || remaining == 0) {
            break;
        }
        character = *cursor;
    }
    return character != L':';
}

components::CStrWChar CSwerveLoader::resolveURI(const components::CStrWChar& base, const components::CStrWChar& relative)
{
    for (int index = base.m_length - 1; index >= 0; index--) {
        if (base.m_string[index] == L'/') {
            return base.GetSubString(0, index + 1) + relative;
        }
    }
    return components::CStrWChar(L"/") + relative;
}

int CSwerveLoader::load(unsigned int size, const unsigned char* data, int offset, core::CVector<IObject3D*>& objects)
{
    if (offset < 0 || (unsigned int)offset >= size) {
        malij297_ErrorMessage("InvalidParameter : invalid offset %d", offset);
        return 0x1000e;
    }
    if (data == NULL) {
        malij297_ErrorMessage("NullPointer : data");
        return 0x1000f;
    }
    components::CArrayInputStream stream;
    int error = 1;
    if (stream.Open(data + offset, size - offset)) {
        error = loadStreamer(0, NULL, -1, &stream, NULL, 0, objects);
    }
    return error;
}

/** True when `data` starts with the M3G (or Swerve extended M3G) file identifier. */
static bool hasM3GIdentifier(const unsigned char* data, unsigned int size)
{
    static const unsigned char M3GIDENTIFIER[12] = { 171, 74, 83, 82, 49, 56, 52, 187, 13, 10, 26, 10 };
    static const unsigned char M3GEXTIDENTIFIER[12] = { 187, 83, 87, 69, 82, 86, 69, 171, 13, 10, 26, 10 };
    if (size >= 12 && memcmp(data, M3GIDENTIFIER, 12) == 0) {
        return true;
    }
    if (size >= 12 && memcmp(data, M3GEXTIDENTIFIER, 12) == 0) {
        return true;
    }
    return false;
}

int CSwerveLoader::loadStreamer(unsigned int referenceCount, IObject3D** references, int imageFormat,
                                components::CInputStream* stream, const wchar_t* uri, int xrefType,
                                core::CVector<IObject3D*>& objects)
{
    if (stream == NULL) {
        malij297_ErrorMessage("NullPointer : stream");
        return 0x1000f;
    }
    ISwerve* runtime = CSwerve::GetInstance()->m_swerve;
    unsigned char buffer[0x1000];
    unsigned int size = stream->Available();
    if (size > 0x1000) {
        size = 0x1000;
    }
    stream->Read(buffer, size);

    if (imageFormat == -1 && xrefType != XREFType_Image && size >= 12 && hasM3GIdentifier(buffer, size)) {
        // an M3G file: decode it chunk by chunk, resolving XREFs as they come
        CSwerveLoader loader;
        loader.m_loader = NULL;
        runtime->m_functions->CreateObject(SwerveObjectType_Loader, (void**)&loader.m_loader);
        int error = loader.m_loader->m_functions->Begin(loader.m_loader);
        if (error == 0) {
            while (true) {
                int consumed;
                error = loader.m_loader->m_functions->Decode(loader.m_loader, 0x1000, buffer, 0, size, &consumed);
                if (error != 0) {
                    break;
                }
                if (consumed > 0) {
                    error = loader.processXREFs(referenceCount, references, uri);
                    if (error != 0) {
                        break;
                    }
                    error = loader.m_loader->m_functions->Decode(loader.m_loader, 0x1000, buffer, consumed,
                                                                 size - consumed, &consumed);
                    if (error != 0) {
                        break;
                    }
                }
                size = stream->Available();
                if (size > 0x1000) {
                    size = 0x1000;
                }
                if (size == 0) {
                    break;
                }
                stream->Read(buffer, size);
            }
            if (error == 0) {
                error = loader.m_loader->m_functions->End(loader.m_loader);
                if (error == 0) {
                    int count = 0;
                    error = loader.m_loader->m_functions->GetObjectCount(loader.m_loader, &count);
                    if (error == 0) {
                        error = 0;
                        for (int index = 0; index < count; index++) {
                            IObject3D* object;
                            error = loader.m_loader->m_functions->GetObject(loader.m_loader, index, &object);
                            if (error == 0) {
                                objects.Add(object);
                            }
                        }
                        if (count == 0 && error == 0) {
                            error = 0x10000;
                            malij297_ErrorMessage("General : Unexpected error loading m3g file [%s].", WcsToStr(uri).m_string);
                        }
                    }
                }
            }
        }
        if (loader.m_loader != NULL) {
            loader.m_loader->m_functions->Release(loader.m_loader);
        }
        return error;
    }

    if (xrefType == XREFType_ByUserID) {
        return 0x1000f;
    }

    // a plain image
    enum { ImageKind_JPEG = 1, ImageKind_KTX = 2, ImageKind_PNG = 3 };
    int kind = ImageKind_PNG;
    // the loaders create an Image2D, which is an IObject3D of the scene graph
    IObject3D* image = NULL;
    if (!malij297_HasPNGIdentifier(0x1000, buffer)) {
        kind = ImageKind_KTX;
        if (!malij297_HasKTXIdentifier(0x1000, buffer)) {
            kind = ImageKind_JPEG;
            if (!malij297_HasJPEGIdentifier(0x1000, buffer)) {
                return 0x1000f;
            }
        }
    }
    unsigned int rest = stream->Available();
    int total = rest + size;
    unsigned char* data = (unsigned char*)malloc(total);
    if (data == NULL) {
        return 2;
    }
    memcpy(data, buffer, size);
    if (rest != 0) {
        stream->Read(data + size, rest);
    }
    int error;
    if (kind == ImageKind_KTX) {
        error = malij297_KTXLoadBuffer(runtime, total, data, 0, (IImage2D**)&image);
    } else if (kind == ImageKind_JPEG) {
        error = malij297_JPEGLoadBuffer(runtime, total, data, 0, (IImage2D**)&image);
    } else {
        error = malij297_PNGLoadBuffer(runtime, total, data, 0, (IImage2D**)&image);
    }
    if (data != NULL) {
        free(data);
    }
    if (error != 0) {
        return error;
    }
    if (image != NULL) {
        objects.Add(image);
        return 0;
    }
    malij297_ErrorMessage("CorruptFile : Invalid file type [%s].", WcsToStr(uri).m_string);
    return 0x1000b;
}

int CSwerveLoader::processXREFs(unsigned int referenceCount, IObject3D** references, const wchar_t* baseUri)
{
    unsigned char* uriBuffer = NULL;
    int uriCapacity = 0;
    int error;
    while (true) {
        int length;
        error = m_loader->m_functions->GetXREFURI(m_loader, 0, NULL, &length);
        if (error != 0) {
            break;
        }
        error = 0;
        if (length == -1) {
            break;
        }
        if (length == 0) {
            // no URI: the XREF is resolved from the objects the caller passed
            int type;
            error = m_loader->m_functions->GetXREFType(m_loader, &type);
            if (error != 0) {
                break;
            }
            if (type != XREFType_ByUserID) {
                malij297_ErrorMessage("InvalidParameter : Unexpected zero length URI in XREF [%s].", WcsToStr(baseUri).m_string);
                error = 0x1000e;
                break;
            }
            if (references == NULL) {
                malij297_ErrorMessage("InvalidParameter : No referenceObjects to resolve XREFs [%s].", WcsToStr(baseUri).m_string);
                error = 0x1000e;
                break;
            }
            int count;
            error = m_loader->m_functions->GetXREFUserIDs(m_loader, 0, NULL, &count);
            if (error != 0) {
                break;
            }
            error = 2;
            int* userIds = new int[count];
            if (userIds == NULL) {
                break;
            }
            error = m_loader->m_functions->GetXREFUserIDs(m_loader, count, userIds, &count);
            if (error != 0) {
                break;
            }
            error = 0;
            for (int remaining = count; remaining > 0; remaining--) {
                error = 0;
                IObject3D* found = NULL;
                int reference = 0;
                while (true) {
                    // 0xc04ba/0xc04bc uses a signed bound, including counts with the high bit set.
                    if (reference >= (int)referenceCount) {
                        error = 0x1000e;
                        malij297_ErrorMessage(
                            "InvalidParameter : XREF resource [%s] contained no match for User ID [%d] in referenceObjects[].",
                            WcsToStr(baseUri).m_string, userIds[remaining - 1]);
                        break;
                    }
                    references[reference]->m_functions->Find(references[reference], userIds[remaining - 1], &found);
                    if (found != NULL) {
                        break;
                    }
                    reference++;
                }
                if (error != 0) {
                    break;
                }
                error = m_loader->m_functions->ResolveXREF(m_loader, found);
            }
            if (userIds != NULL) {
                delete[] userIds;
            }
            if (length == -1) {
                break;
            }
            continue;
        }

        // load the referenced file
        if (uriBuffer == NULL || uriCapacity < length) {
            uriBuffer = new unsigned char[length + 1];
            uriCapacity = length + 1;
        }
        error = m_loader->m_functions->GetXREFURI(m_loader, uriCapacity, uriBuffer, &length);
        if (error != 0) {
            break;
        }
        uriBuffer[length] = 0;
        int wideLength = core::CUtf::GetWcsLen(uriBuffer, uriCapacity);
        components::CStrWCharBuffer xrefUri(wideLength);
        core::CUtf::Utf8ToWcs(uriBuffer, uriCapacity, xrefUri.m_string, wideLength);
        bool relative = isRelative(components::CStrWChar(xrefUri.m_string));
        if (relative && baseUri != NULL) {
            components::CStrWChar base(baseUri);
            components::CStrWChar reference(xrefUri.m_string);
            components::CStrWChar resolved = resolveURI(base, reference);
            if (resolved.m_string != xrefUri.m_string) {
                xrefUri.m_length = 0;
                xrefUri.m_string[0] = 0;
                xrefUri.Append(resolved.m_string);
            }
        }

        bool stopProcessing = true;
        int type;
        error = m_loader->m_functions->GetXREFType(m_loader, &type);
        if (error == 0) {
            core::CVector<IObject3D*> loaded;
            int format = -1;
            if (type != XREFType_Image ||
                (error = m_loader->m_functions->GetXREFImageFormat(m_loader, &format)) == 0) {
                components::CInputStream* stream = CSwerveLoader_getResourceAsStream(xrefUri.m_string);
                error = loadStreamer(referenceCount, references, format, stream, xrefUri.m_string, type, loaded);
                if (stream != NULL) {
                    delete stream;
                }
                if (error == 0) {
                    bool releaseLoaded = false;
                    if ((unsigned int)(type - XREFType_Image) < 2) {
                        // an image or a whole file: the first object is the target
                        error = m_loader->m_functions->ResolveXREF(m_loader, loaded.m_data[0]);
                        releaseLoaded = true;
                    } else {
                        int count;
                        error = m_loader->m_functions->GetXREFUserIDs(m_loader, 0, NULL, &count);
                        if (error == 0) {
                            error = 2;
                            int* userIds = new int[count];
                            if (userIds != NULL) {
                                error = m_loader->m_functions->GetXREFUserIDs(m_loader, count, userIds, &count);
                                if (error == 0) {
                                    releaseLoaded = true;
                                    for (int remaining = count; remaining > 0; remaining--) {
                                        error = 0;
                                        IObject3D* found = NULL;
                                        int index = 0;
                                        while (true) {
                                            if (index >= loaded.m_size) {
                                                error = 0x1000e;
                                                malij297_ErrorMessage(
                                                    "InvalidParameter : XREF resource [%s] contained no match for User ID [%d].",
                                                    WcsToStr(xrefUri.m_string).m_string, userIds[remaining - 1]);
                                                break;
                                            }
                                            loaded.m_data[index]->m_functions->Find(loaded.m_data[index],
                                                                                    userIds[remaining - 1], &found);
                                            if (found != NULL) {
                                                break;
                                            }
                                            index++;
                                        }
                                        if (error != 0) {
                                            break;
                                        }
                                        error = m_loader->m_functions->ResolveXREF(m_loader, found);
                                    }
                                    // 0xc0758 skips this cleanup if the ID query failed.
                                    if (userIds != NULL) {
                                        delete[] userIds;
                                    }
                                }
                            }
                        }
                    }
                    // the loader keeps what it resolved
                    // 0xc0724/0xc0742/0xc0758 skip Release when enumeration cannot run.
                    if (releaseLoaded) {
                        for (int index = 0; index < loaded.m_size; index++) {
                            loaded.m_data[index]->m_functions->Release(loaded.m_data[index]);
                        }
                        // 0xc080a..0xc083e stops after a successful external ResolveXREF.
                        stopProcessing = error == 0;
                    }
                }
            }
        }
        if (stopProcessing) {
            break;
        }
        if (length == -1) {
            break;
        }
    }
    if (uriBuffer != NULL) {
        delete[] uriBuffer;
    }
    return error;
}
