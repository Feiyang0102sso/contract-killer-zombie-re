// CNGSTokens.h - JSON keys and file names shared by the gServe sources (created by b016x).
// The original defines them as header-level static const arrays (internal symbols named
// NGS_*_TOKEN / CREDENTIAL_FILE_NAME in every object that uses them); the original header
// name is unknown. Other batches append the tokens they need here.
#ifndef CNGSTOKENS_H
#define CNGSTOKENS_H

#include <wchar.h>

static const wchar_t CREDENTIAL_FILE_NAME[] = L"Credentials.dat";

static const char NGS_ERROR_CODE_TOKEN[] = "errorcode";
static const char NGS_MESSAGE_TOKEN[] = "message";
static const char NGS_MAC_TOKEN[] = "mac";
static const char NGS_TYPE_TOKEN[] = "type";
static const char NGS_HEADER_TOKEN[] = "header";
static const char NGS_LIST_TOKEN[] = "list";

// persistence tokens (added by b025x)
static const char NGS_INTEGER_VALUE_TOKEN[] = "intValue";                                   // added by b025x
static const char NGS_STRING_VALUE_TOKEN[] = "stringValue";                                 // added by b025x
static const char NGS_COLLECTION_TOKEN[] = "collectiontype";                                // added by b025x
static const char NGS_NAME_TOKEN[] = "name";                                                // added by b025x (original symbol name unknown)
static const char NGS_AVATAR_ATTRIBUTES_TOKEN[] = "avatarattributes";                       // added by b025x
static const char NGS_MESSAGE_TYPE_AVATAR_UPDATE_ITEM_STATS[] = "ngs.avatar.update.item.stats";    // added by b025x
static const char NGS_USER_TOKEN[] = "user";                                                // added by b025x
static const char NGS_REGISTER_AVATAR_TOKEN[] = "registeravatar";                           // added by b025x
static const char NGS_MESSAGE_TYPE_AVATAR_QUERY_ATTRIBUTES[] = "ngs.avatar.query.attributes";      // added by b025x
static const char NGS_GAME_ID_TOKEN[] = "gameid";                                           // added by b025x
static const char NGS_FILENAME_TOKEN[] = "filename";                                        // added by b025x
static const char NGS_DATA_TOKEN[] = "data";                                                // added by b025x

static const char NGS_ID_TOKEN[] = "id";                                                    // added by b023x (name from CNGSLocalUser.o)

// user management tokens (added by b030x; names from the CNGSLocalUser.o debug map)
static const char NGS_GAMERTAG_TOKEN[] = "gamertag";                                        // added by b030x
static const char NGS_PASSWORD_TOKEN[] = "password";                                        // added by b030x
static const char NGS_FACEBOOK_ID_TOKEN[] = "facebookid";                                   // added by b030x
static const char NGS_ATTRIBUTENAME_TOKEN[] = "attributeName";                              // added by b030x
static const char NGS_STRINGVALUE_TOKEN[] = "stringValue";                                  // added by b030x
static const char NGS_USERATTRIBUTE_TOKEN[] = "userattribute";                              // added by b030x
static const char NGS_USERATTRIBUTES_TOKEN[] = "userattributes";                            // added by b030x
static const char NGS_STATUS_TOKEN[] = "status";                                            // added by b030x (name guessed: unnamed __const string in CNGSRemoteUser.o)
static const char NGS_EMAIL_TOKEN[] = "email";                                              // added by b030x (name guessed: unnamed __const string in CNGSLocalUser.o)

#endif
