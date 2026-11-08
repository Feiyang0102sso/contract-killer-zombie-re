// CNGSConst.cpp - per social network name tables of gServe, indexed by eSocialNetworkType
// (0 Glu social, 1 Game Center, 2 Facebook, 3 Twitter, 4 MySpace, 5 OpenFeint).
// Data only: the original object has no functions and no inline code (written by fx1; values
// exported from the original with tools/m3_data.py, symbol order as in the debug map).
#include "CNGSConst.h"

/** Credentials file keys of the per network user ids. */
const wchar_t* SocialNetworkIDNames[] = {
    L"gluid",
    L"gamecenterid",
    L"facebookid",
    L"twitterid",
    L"myspaceid",
    L"openfeintid"
};

/** Credentials file keys of the per network aliases. */
const wchar_t* SocialNetworkAliasNames[] = {
    L"gluAlias",
    L"gamecenterAlias",
    L"facebookAlias",
    L"twitterAlias",
    L"myspaceAlias",
    L"openfeintAlias"
};

/** Server attribute names of the per network user ids (no attribute for Facebook). */
const char* SocialNetwork_NGS_ID_TOKEN[] = {
    "GLUSOCIALID",
    "GAMECENTERID",
    "FACEBOOKID",       // b109: was "" (ref showed odd-address strings as empty)
    "TWITTERID",
    "MYSPACEID",
    "OPENFEINTID"
};

/** Server attribute names of the per network aliases (only Glu social and MySpace have one). */
const char* SocialNetwork_NGS_ALIAS_TOKEN[] = {
    // b109: entries 1, 2, 3, 5 restored from the original table (ref showed odd-address strings as "")
    "GLUSOCIALALIAS",
    "GAMECENTERALIAS",
    "FACEBOOKALIAS",
    "TWITTERALIAS",
    "MYSPACEALIAS",
    "OPENFEINTALIAS",
};
