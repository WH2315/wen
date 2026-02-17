#pragma once

#define SERIALIZABLE_CLASS
#define SERIALIZABLE_MEMBER
#define SERIALIZE_PARENT_CLASS

#define ALL_PRIVATE_SERIALIZATION(ClassName) \
    friend struct wen::SerializeTraits<ClassName>;

namespace wen {

template <class T>
struct SerializeTraits {
    SerializeTraits() = delete;

    static void serialize(class SerializeStream& stream, const T& value);
    static void deserialize(class DeserializeStream& stream, T& value);
};

}  // namespace wen