// Serializable.h - objects that save to / load from the big-endian game streams.
#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include "DataOutputStream.h"
#include "DataInputStream.h"
#include "Vector_ss.h"

class Serializable {
public:
    virtual void Serialize(DataOutputStream& stream) = 0;
    virtual void DeSerialize(JDataInputStream& stream) = 0;
};

/** Element count, then every element as an int. */
template <class T>
void SERIALIZE_VECTOR(Vector<T>& vector, DataOutputStream& stream)
{
    stream.writeInt(vector.size());
    for (int index = 0; index < vector.size(); index++) {
        stream.writeInt(vector[index]);
    }
}

/** Append the elements written by SERIALIZE_VECTOR (the vector is not cleared first). */
template <class T>
void DESERIALIZE_VECTOR(Vector<T>& vector, JDataInputStream& stream)
{
    int count = stream.readInt();
    for (int index = 0; index < count; index++) {
        T element = (T)stream.readInt();
        vector.addElement(element);
    }
}

#endif
