// Game-data singleton, original 0x0e133c..0x0e13f0.
#include "CGameData.h"

CGameData::CGameData() : com::glu::platform::components::CSingleton(SingletonId)
{
    // m_resBank is deliberately not initialized in the shipped constructor.
    m_field0c = false;
    m_field0d = false;
    m_field10 = 0;
}
CGameData::~CGameData()
{
    // The base destructor unregisters the singleton; no extra owned object is released.
}
