// CVertex.cpp - names of the intrinsic vertex attributes.
#include "CVertex.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// Attribute names (the BlendIndices one is spelled "BlendIndics" in the original data).
static const wchar_t CVertex_Attribute_Id_Color_str[] = L"Color";
static const wchar_t CVertex_Attribute_Id_Depth_str[] = L"Depth";
static const wchar_t CVertex_Attribute_Id_Binormal_str[] = L"Binormal";
static const wchar_t CVertex_Attribute_Id_BlendIndices_str[] = L"BlendIndics";
static const wchar_t CVertex_Attribute_Id_BlendWeights_str[] = L"BlendWeights";
static const wchar_t CVertex_Attribute_Id_Fog_str[] = L"Fog";
static const wchar_t CVertex_Attribute_Id_Normal_str[] = L"Normal";
static const wchar_t CVertex_Attribute_Id_PointSize_str[] = L"PointSize";
static const wchar_t CVertex_Attribute_Id_Position_str[] = L"Position";
static const wchar_t CVertex_Attribute_Id_Position1_str[] = L"Position1";
static const wchar_t CVertex_Attribute_Id_PositionT_str[] = L"PositionT";
static const wchar_t CVertex_Attribute_Id_Sample_str[] = L"Sample";
static const wchar_t CVertex_Attribute_Id_Tangent_str[] = L"Tangent";
static const wchar_t CVertex_Attribute_Id_TessFactor_str[] = L"TessFactor";
static const wchar_t CVertex_Attribute_Id_TexCoord_str[] = L"TexCoord";
static const wchar_t CVertex_Attribute_Id_TexCoord1_str[] = L"TexCoord1";
static const wchar_t CVertex_Attribute_Id_TexCoord2_str[] = L"TexCoord2";
static const wchar_t CVertex_Attribute_Id_TexCoord3_str[] = L"TexCoord3";
static const wchar_t CVertex_Attribute_Id_TexCoord4_str[] = L"TexCoord4";
static const wchar_t CVertex_Attribute_Id_TexCoord5_str[] = L"TexCoord5";
static const wchar_t CVertex_Attribute_Id_TexCoord6_str[] = L"TexCoord6";
static const wchar_t CVertex_Attribute_Id_TexCoord7_str[] = L"TexCoord7";

const wchar_t* CVertex::Attribute::Id::GetInstrinsicName(unsigned int id)
{
    switch (id) {
    case Color:
        return CVertex_Attribute_Id_Color_str;
    case Depth:
        return CVertex_Attribute_Id_Depth_str;
    case Binormal:
        return CVertex_Attribute_Id_Binormal_str;
    case BlendIndics:
        return CVertex_Attribute_Id_BlendIndices_str;
    case BlendWeights:
        return CVertex_Attribute_Id_BlendWeights_str;
    case Fog:
        return CVertex_Attribute_Id_Fog_str;
    case Normal:
        return CVertex_Attribute_Id_Normal_str;
    case PointSize:
        return CVertex_Attribute_Id_PointSize_str;
    case Position:
        return CVertex_Attribute_Id_Position_str;
    case Position1:
        return CVertex_Attribute_Id_Position1_str;
    case PositionT:
        return CVertex_Attribute_Id_PositionT_str;
    case Sample:
        return CVertex_Attribute_Id_Sample_str;
    case Tangent:
        return CVertex_Attribute_Id_Tangent_str;
    case TessFactor:
        return CVertex_Attribute_Id_TessFactor_str;
    case TexCoord:
        return CVertex_Attribute_Id_TexCoord_str;
    case TexCoord1:
        return CVertex_Attribute_Id_TexCoord1_str;
    case TexCoord2:
        return CVertex_Attribute_Id_TexCoord2_str;
    case TexCoord3:
        return CVertex_Attribute_Id_TexCoord3_str;
    case TexCoord4:
        return CVertex_Attribute_Id_TexCoord4_str;
    case TexCoord5:
        return CVertex_Attribute_Id_TexCoord5_str;
    case TexCoord6:
        return CVertex_Attribute_Id_TexCoord6_str;
    case TexCoord7:
        return CVertex_Attribute_Id_TexCoord7_str;
    }
    return NULL;
}

} } } }
