// CLocation.h - location resources, camera constraints and named scene sections.
#ifndef CLOCATION_H
#define CLOCATION_H
#include "xString.h"
#include "Vector_ss.h"
class TiXmlNode;
class TiXmlElement;

class CLocation {
public:
    /** An object configuration placed at the named scene transform. */
    struct SObject {
        XString m_name;
        XString m_pointName;
    };
    struct SOptionalPart {
        XString m_name;
        XString m_group;
        bool m_hidden;
    };
    struct SSector {
        XString m_name;
        Vector<XString> m_points;
    };
    CLocation(TiXmlNode* node);
    void Parse(TiXmlNode* node);
    void ParseInfo(TiXmlNode* node);
    void ParseModel(TiXmlNode* node);
    void ParseAmbient(TiXmlNode* node);
    void ParseGraphics(TiXmlElement* element);
    void ParseObjects(TiXmlNode* node);
    void ParseSectors(TiXmlNode* node);
    void ParseOptionalParts(TiXmlNode* node);

    /** Original @0x142784 falls back to the first sector when the name is absent. */  // added by b080
    const Vector<XString>& GetSectorPoints(const XString& name) const  // added by b080
    {
        int sectorCount = m_sectors.size();
        for (int index = 0; index < sectorCount; index++) {
            if (m_sectors[index].m_name == name) {
                return m_sectors[index].m_points;
            }
        }
        return m_sectors[0].m_points;
    }

    XString m_name;                  // 0x00
    XString m_title;                 // 0x04
    XString m_bigModel;              // 0x08
    XString m_smallModel;            // 0x0c
    XString m_light;                 // 0x10
    const char* m_ambientResource;   // 0x14 only written when wav/ogg is present
    XString m_cameraImagesKeyset;    // 0x18
    XString m_cameraName;            // 0x1c
    float m_cameraMaxAngleH;         // 0x20
    float m_cameraMaxAngleV;         // 0x24
    float m_baseLuminance;           // 0x28
    int m_sectorCapacity;            // 0x2c
    XString m_lightingNode;          // 0x30
    Vector<SObject> m_objects;       // 0x34
    Vector<SOptionalPart> m_parts;   // 0x44
    Vector<SSector> m_sectors;       // 0x54
};
#endif
