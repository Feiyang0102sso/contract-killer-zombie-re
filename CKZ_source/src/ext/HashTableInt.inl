/** Real linked-list hash instance for integer wrapper keys; weak implementation from the original CUnitsController.o. */
template <class T>
struct HashTableKey {
    HashTableKey(T value) : m_value(value) {}
    T m_value;
};

template <class T, class Value>
class HashTable<HashTableKey<T>, Value> {
public:
    struct Entry {
        Entry(const HashTableKey<T>& key, const Value& value) : m_key(key), m_value(value) {}
        HashTableKey<T> m_key;
        Value m_value;
        Entry* m_next;
    };

    HashTable(int bucketCount = 8)
    {
        unsigned int count = 8;
        if (bucketCount > 0) {
            count = MathLib::CeilPowerOfTwo(bucketCount);
        }
        m_bucketCount = count;
        m_buckets = new Entry*[m_bucketCount];
        memset(m_buckets, 0, m_bucketCount * sizeof(Entry*));
        m_count = 0;
    }
    ~HashTable()
    {
        Clear();
        delete[] m_buckets;
    }
    void Add(const HashTableKey<T>& key, const Value& value);
    Value* Get(const HashTableKey<T>& key) const;
    bool Remove(const HashTableKey<T>& key);
    void Clear();
    void Grow();

    Entry** m_buckets;
    unsigned int m_bucketCount;
    int m_count;
};

template <class T, class Value>
void HashTable<HashTableKey<T>, Value>::Add(const HashTableKey<T>& key, const Value& value)
{
    if ((m_count >> 1) > (int)m_bucketCount) {
        Grow();
    }
    Entry** link = &m_buckets[key.m_value & (m_bucketCount - 1)];
    Entry* entry;
    while ((entry = *link) != NULL && entry->m_key.m_value < key.m_value) {
        link = &entry->m_next;
    }
    ++m_count;
    Entry* added = new Entry(key, value);
    *link = added;
    added->m_next = entry;
}

template <class T, class Value>
Value* HashTable<HashTableKey<T>, Value>::Get(const HashTableKey<T>& key) const
{
    Entry* entry = m_buckets[key.m_value & (m_bucketCount - 1)];
    while (entry != NULL) {
        if (entry->m_key.m_value == key.m_value) {
            return &entry->m_value;
        }
        if (entry->m_key.m_value > key.m_value) {
            return NULL;
        }
        entry = entry->m_next;
    }
    return NULL;
}

template <class T, class Value>
bool HashTable<HashTableKey<T>, Value>::Remove(const HashTableKey<T>& key)
{
    unsigned int bucket = key.m_value & (m_bucketCount - 1);
    Entry* entry = m_buckets[bucket];
    Entry* previous = NULL;
    while (entry != NULL) {
        if (entry->m_key.m_value == key.m_value) {
            if (previous == NULL) {
                m_buckets[bucket] = entry->m_next;
            } else {
                previous->m_next = entry->m_next;
            }
            --m_count;
            delete entry;
            return true;
        }
        previous = entry;
        entry = entry->m_next;
    }
    return false;
}

template <class T, class Value>
void HashTable<HashTableKey<T>, Value>::Clear()
{
    if (m_count <= 0) {
        return;
    }
    for (int index = 0; index < (int)m_bucketCount; ++index) {
        Entry* entry = m_buckets[index];
        while (entry != NULL) {
            Entry* next = entry->m_next;
            delete entry;
            entry = next;
        }
        m_buckets[index] = NULL;
    }
    m_count = 0;
}

template <class T, class Value>
void HashTable<HashTableKey<T>, Value>::Grow()
{
    Entry** oldBuckets = m_buckets;
    int oldCount = m_bucketCount;
    m_bucketCount = oldCount * 2;
    m_count = 0;
    m_buckets = new Entry*[m_bucketCount];
    memset(m_buckets, 0, m_bucketCount * sizeof(Entry*));
    for (int index = 0; index < oldCount; ++index) {
        Entry* entry = oldBuckets[index];
        while (entry != NULL) {
            Entry* next = entry->m_next;
            Add(entry->m_key, entry->m_value);
            delete entry;
            entry = next;
        }
    }
    delete[] oldBuckets;
}
