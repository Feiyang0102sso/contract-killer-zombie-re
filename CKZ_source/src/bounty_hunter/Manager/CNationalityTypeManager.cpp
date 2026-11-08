// CNationalityTypeManager.cpp - owns nationality records populated by the game.
#include "CNationalityTypeManager.h"

CNationalityTypeManager::CNationalityTypeManager()
{
    // The original constructor only initializes the empty hash table.
}

CNationalityTypeManager::~CNationalityTypeManager()
{
    m_types.DeleteContents();
}
