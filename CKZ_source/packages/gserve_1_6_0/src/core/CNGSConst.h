// (created by b027x as PROVISIONAL; made final by fx1 together with core/CNGSConst.cpp)
// CNGSConst.h - per social network name tables of gServe, indexed by eSocialNetworkType
// (defined in CNGSConst.cpp; only the tables used by the social interfaces are declared).
#ifndef CNGSCONST_H
#define CNGSCONST_H

/** Server attribute names of the per network user ids ("GLUSOCIALID", "GAMECENTERID", ...). */
extern const char* SocialNetwork_NGS_ID_TOKEN[];
/** Server attribute names of the per network aliases ("GLUSOCIALALIAS", ..., "MYSPACEALIAS"). */
extern const char* SocialNetwork_NGS_ALIAS_TOKEN[];
/** Credentials file keys of the per network ids (wide strings). */  // added by b030x
extern const wchar_t* SocialNetworkIDNames[];  // added by b030x
/** Credentials file keys of the per network aliases (wide strings). */  // added by b030x
extern const wchar_t* SocialNetworkAliasNames[];  // added by b030x

#endif
