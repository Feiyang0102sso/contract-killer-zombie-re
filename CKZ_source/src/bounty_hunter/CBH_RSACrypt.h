// PROVISIONAL: owner CBH_RSACrypt.cpp remains to be recovered (b074).
// Signatures verified against the two original CBH_XorCrypt symbols.
// b074: complete two-method owner restored; the historical provisional note is retained.
#ifndef CBH_RSACRYPT_H
#define CBH_RSACRYPT_H
class CBH_XorCrypt {
public:
    // Original ends with sxtb: the key is returned as a signed char.
    static char Cipher(char* data, int size);
    // Historical void declaration corrected: every original exit writes r0=0.
    // int is an ABI-level return inference; the save-manager caller ignores it.
    static int Decypher(char* data, int size, unsigned char key);
};
#endif
