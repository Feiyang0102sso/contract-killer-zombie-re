// ResMap.cpp - resource name map read from res_map.dat.
#include "ResMap.h"
#include <string.h>
#include <wchar.h>
#include "CStrWChar.h"
#include "CFileUtil.h"
#include "CFileInputStream.h"
#include "CApplet.h"
#include "CApp.h"
#include "CResource.h"
#include "ICResourceManager.h"
#include "App.h"
#include "CUpdateManager.h"

using namespace com::glu::platform;
using com::glu::platform::components::CStrChar;
using com::glu::platform::components::CStrWChar;

const wchar_t* ResMap::RESMAP_FILENAME = L"res_map.dat";
const char* ResMap::SEPARATOR = "=";

ResMap::ResMap()
    : m_fileNames(8), m_resourceIds(8)
{
}

ResMap::~ResMap()
{
}

void ResMap::init()
{
    m_fileNames.SetTableSize(1000);
    CStrWChar path;
    components::CFileUtil::GetApplicationPathForFile(path, RESMAP_FILENAME);
    components::CFileInputStream stream;
    stream.Open(path.m_string);
    unsigned int size = stream.m_length;
    char* data = new char[size + 1];
    data[size] = 0;
    stream.Read((unsigned char*)data, size);
    stream.Close();
    parseMapData(data, size);
    delete[] data;
}

void ResMap::parseMapData(const char* data, unsigned int size)
{
    const char* line = data;
    while ((unsigned int)(line - data) < size) {
        int endLength;
        const char* end = strstr(line, "\r\n");
        if (end != NULL) {
            endLength = 2;
        } else {
            end = strchr(line, '\n');
            if (end == NULL) {
                end = strchr(line, '\r');
                if (end == NULL) {
                    return;
                }
            }
            endLength = 1;
        }
        parseMapStr(line, end - line);
        line = end + endLength;
    }
}

void ResMap::parseMapStr(const char* line, unsigned int length)
{
    const char* separator = strstr(line, SEPARATOR);
    int keyLength = separator - line;
    char* resourceId = new char[keyLength + 1];
    strncpy(resourceId, line, keyLength + 1);
    resourceId[keyLength] = 0;
    int valueLength = line + length - separator;
    char* fileName = new char[valueLength];
    strncpy(fileName, separator + 1, valueLength);
    fileName[valueLength - 1] = 0;
    addMapRecord(resourceId, fileName);
    delete[] resourceId;
    delete[] fileName;
}

void ResMap::addMapRecord(const char* resourceId, const char* fileName)
{
    CStrChar id(resourceId);
    m_fileNames.Add(XString(resourceId), XString(fileName));
    m_resourceIds.Add(XString(fileName), id);
}

XString ResMap::collapsePathToString(const XString& path)
{
    XString copy = path;
    return copy.Replace(L"/", L"\\", true);
}

XString ResMap::getDownloadFileName(const XString& path) const
{
    Vector<XString> parts = path.Split(L"/");
    if (parts.size() > 0) {
        CStrWChar resourcePath;
        ((App*)WindowApp::m_instance)->m_updateManager->makeResourcePath(parts[parts.size() - 1].m_data, resourcePath);
        return XString(resourcePath.m_string);
    }
    return XString(L"");
}

XString ResMap::getFilename(const char* name) const
{
    XString* fileName = m_fileNames.Get(XString(name));
    if (fileName != NULL) {
        return *fileName;
    }
    return XString("");
}

const char* ResMap::getResourceID(const XString& fileName) const
{
    if (fileName.m_data == NULL) {
        return NULL;
    }
    if (fileName == XString("")) {
        return NULL;
    }
    CStrChar* id = m_resourceIds.Get(fileName);
    if (id->m_length == 0) {
        return NULL;
    }
    return id->m_string;
}

char* ResMap::getDataFromFile(const XString& path) const
{
    components::CFileInputStream stream;
    char* data = NULL;
    if (path.ByteSize() != 0 && stream.Open(path.m_data)) {
        unsigned int size = stream.m_length;
        data = new char[size + 1];
        stream.Read((unsigned char*)data, size);
        data[size] = 0;
        stream.Close();
    }
    return data;
}

char* ResMap::loadText(const XString& name) const
{
    // 1. downloaded update
    char* text = getDataFromFile(getDownloadFileName(name));
    if (text != NULL) {
        return text;
    }

    // 2. copy in the application data folder
    CStrWChar path(name.m_data);
    components::CFileUtil::GetApplicationDataPathForFile(path, collapsePathToString(name).m_data);
    text = getDataFromFile(XString(path.m_string));
    if (text == NULL) {
        // 3. bundled resource
        systems::ICResourceManager* resources = (systems::ICResourceManager*)CApplet::m_pApp->m_resourceManager;
        systems::CResource* resource = NULL;
        resources->CreateResource(getResourceID(name), &resource);
        if (resource != NULL) {
            unsigned int size = ((CStrChar*)resource->GetData())->m_length;
            text = new char[size + 1];
            memcpy(text, ((CStrChar*)resource->GetData())->m_string, size);
            text[size] = 0;
            resources->ReleaseResource(getResourceID(name));
        }
    }
    return text;
}
