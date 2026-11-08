// CHash.cpp - chained hash table with a prime bucket count.
#include "CHash.h"
#include "CPool.h"

namespace com { namespace glu { namespace platform { namespace components {

unsigned char CHash::Init(unsigned long minBuckets, unsigned long poolSize, unsigned char poolGrowable)
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
    m_pool = new CPool(poolSize, sizeof(Entry), poolGrowable);
    return m_pool != NULL;
}

void CHash::Destroy()
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

void CHash::Insert(unsigned int key, void* value)
{
    unsigned int index = key % m_bucketCount;
    Entry* entry = (Entry*)m_pool->New(sizeof(Entry));
    Entry* oldHead = m_buckets[index];
    m_buckets[index] = entry;
    entry->m_key = key;
    entry->m_value = value;
    entry->m_next = oldHead;
}

void CHash::Remove(unsigned int key)
{
    unsigned int index = key % m_bucketCount;
    Entry* previous = NULL;
    Entry* entry = m_buckets[index];
    while (entry != NULL) {
        if (entry->m_key == key) {
            if (previous != NULL) {
                previous->m_next = entry->m_next;
            } else {
                m_buckets[index] = entry->m_next;
            }
            m_pool->Delete(entry);
            return;
        }
        previous = entry;
        entry = entry->m_next;
    }
}

void CHash::RemoveAll()
{
    for (int i = 0; i < m_bucketCount; i++) {
        while (m_buckets[i] != NULL) {
            Entry* entry = m_buckets[i];
            m_buckets[i] = entry->m_next;
            m_pool->Delete(entry);
        }
    }
}

unsigned char CHash::Find(unsigned int key, void** outValue) const
{
    Entry* entry = m_buckets[key % m_bucketCount];
    while (entry != NULL) {
        if (entry->m_key == key) {
            if (outValue != NULL) {
                *outValue = entry->m_value;
            }
            return 1;
        }
        entry = entry->m_next;
    }
    return 0;
}

} } } }
