// CHash_gServe.cpp - chained hash table with a prime bucket count.
#include "CHash_gServe.h"
#include "CPool.h"

using namespace com::glu::platform;

unsigned char CHash_gServe::Init(unsigned long minBuckets, unsigned long poolSize, unsigned char poolGrowable)
{
    // smallest odd prime >= minBuckets
    int bucketCount = minBuckets + ((minBuckets + 1) & 1);
    int divisor = 3;
    while (divisor * divisor <= bucketCount) {
        if (bucketCount % divisor == 0) {
            bucketCount += 2;
            divisor = 3;
        } else {
            divisor += 2;
        }
    }
    m_bucketCount = bucketCount;
    m_buckets = new Entry*[m_bucketCount];
    memset(m_buckets, 0, m_bucketCount * sizeof(Entry*));
    m_pool = new components::CPool(poolSize, sizeof(Entry), poolGrowable);
    return m_pool != NULL;
}

void CHash_gServe::Destroy()
{
    if (m_buckets != NULL) {
        delete[] m_buckets;
        m_bucketCount = 0;
        m_buckets = NULL;
    }
    if (m_pool != NULL) {
        delete m_pool;
        m_pool = NULL;
    }
}

void CHash_gServe::RemoveAll()
{
    for (int i = 0; i < m_bucketCount; i++) {
        while (m_buckets[i] != NULL) {
            Entry* entry = m_buckets[i];
            m_buckets[i] = entry->m_next;
            m_pool->Delete(entry);
        }
    }
}
