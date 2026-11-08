// CMD5_gServe.h - MD5 digest singleton of gserve (hex digest of a C string).
// The digest code is the well known RSA-derived C++ MD5 class (Frank Thilo's md5.cpp).
// Owned by util/CMD5_gServe.cpp (b032, written by b027x).
#ifndef CMD5_GSERVE_H
#define CMD5_GSERVE_H

#include "CSingleton.h"

class CMD5_gServe : public com::glu::platform::components::CSingleton {
public:
    /** Singleton key in CApp::m_singletons. */
    enum { SingletonId = 0x0925d292 };

    /** MD5 processes the input in 64 byte blocks. */
    enum { BLOCK_SIZE = 64 };

    CMD5_gServe();
    virtual ~CMD5_gServe();

    /** The registered instance, created on first use (CFileUtil_gServe::GenerateCacheFilePath).
        `void* found` then cast gives the original cmp+bne (verified by b020). */
    static inline CMD5_gServe* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        CMD5_gServe* instance = (CMD5_gServe*)found;
        if (instance == NULL) {
            instance = new CMD5_gServe();
        }
        return instance;
    }

    /** Hex digest of `text` in a malloc'd buffer (the caller frees it). */
    char* GenerateMD5(const char* text);

    /** Resets the context (inlined into GenerateMD5). */
    inline void init()
    {
        m_finalized = false;
        m_count[0] = 0;
        m_count[1] = 0;
        // load magic initialization constants
        m_state[0] = 0x67452301;
        m_state[1] = 0xefcdab89;
        m_state[2] = 0x98badcfe;
        m_state[3] = 0x10325476;
    }

    /** Continues the digest with `length` more bytes. */
    void update(const unsigned char* input, unsigned int length);
    /** Pads the message, appends the bit count and stores the digest. */
    CMD5_gServe& finalize();
    /** 32 hex digits (+ terminator) in a malloc'd buffer, NULL before finalize(). */
    char* hexdigest();

private:
    /** Applies the MD5 compression function to one 64 byte block. */
    void transform(const unsigned char block[BLOCK_SIZE]);

    /** Little endian bytes -> words (len is a multiple of 4). */
    static inline void decode(unsigned int output[], const unsigned char input[], unsigned int len)
    {
        for (unsigned int i = 0, j = 0; j < len; i++, j += 4) {
            output[i] = ((unsigned int)input[j]) | (((unsigned int)input[j + 1]) << 8)
                      | (((unsigned int)input[j + 2]) << 16) | (((unsigned int)input[j + 3]) << 24);
        }
    }

    /** Words -> little endian bytes (len is a multiple of 4). */
    static inline void encode(unsigned char output[], const unsigned int input[], unsigned int len)
    {
        for (unsigned int i = 0, j = 0; j < len; i++, j += 4) {
            output[j] = input[i] & 0xff;
            output[j + 1] = (input[i] >> 8) & 0xff;
            output[j + 2] = (input[i] >> 16) & 0xff;
            output[j + 3] = (input[i] >> 24) & 0xff;
        }
    }

    bool m_finalized;                       // 0x08
    unsigned char m_buffer[BLOCK_SIZE];     // 0x09 bytes that did not fill the last block
    unsigned int m_count[2];                // 0x4c number of bits (low, high)
    unsigned int m_state[4];                // 0x54 digest so far (A, B, C, D)
    unsigned char m_digest[16];             // 0x64 the result; object size 0x74
};

#endif
