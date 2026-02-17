#pragma once

#include "core/serialize/serialize.hpp"

namespace wen {

// 序列化流
class SerializeStream {
    friend class DeserializeStream;

public:
    SerializeStream();
    ~SerializeStream();

    template <class T>
    std::enable_if_t<std::is_arithmetic_v<T>, void> write(T value) {
        *reinterpret_cast<T*>(getSafePtr(sizeof(T))) = value;
    }

    void write(const char* buffer, size_t size);

private:
    void* getSafePtr(size_t size);

private:
    std::vector<uint8_t> data_;
};

template <class T>
std::enable_if_t<std::is_arithmetic_v<T>, SerializeStream>& operator<<(SerializeStream& stream, T value) {
    SerializeTraits<T>::serialize(stream, value);
    return stream;
}

template <class T>
std::enable_if_t<(!std::is_arithmetic_v<T>) && std::is_constructible_v<SerializeTraits<T>>, SerializeStream>& operator<<(SerializeStream& stream, const T& value) {
    SerializeTraits<T>::serialize(stream, value);
    return stream;
};

template <class T>
std::enable_if_t<std::is_constructible_v<SerializeTraits<T>>, SerializeStream>& operator<<(SerializeStream& stream, const T* value) {
    SerializeTraits<T>::serialize(stream, *value);
    return stream;
}

template <class T>
void SerializeTraits<T>::serialize(SerializeStream& stream, const T& value) {
    stream.write<T>(value);
}

// 反序列化流
class DeserializeStream {
public:
    DeserializeStream(SerializeStream&&);
    ~DeserializeStream();

    template <class T>
    std::enable_if_t<std::is_arithmetic_v<T>, T> read() {
        offset_ -= sizeof(T);
        return *reinterpret_cast<T*>(&data_[offset_]);
    }

    void read(void* buffer, size_t size);

private:
    size_t offset_;
    std::vector<uint8_t> data_;
};

template <class T>
std::enable_if_t<std::is_constructible_v<T>, DeserializeStream>& operator>>(DeserializeStream& stream, T& value) {
    SerializeTraits<T>::deserialize(stream, value);
    return stream;
}

template <class T>
std::enable_if_t<std::is_constructible_v<T>, DeserializeStream>& operator>>(DeserializeStream& stream, T* value) {
    SerializeTraits<T>::deserialize(stream, *value);
    return stream;
}

template <class T>
void SerializeTraits<T>::deserialize(DeserializeStream& stream, T& value) {
    value = stream.read<T>();
}

// std::string
template <>
struct SerializeTraits<std::string> {
    static void serialize(SerializeStream& stream, const std::string& value) {
        stream.write(value.data(), value.length());  
        stream << value.length();
    }

    static void deserialize(DeserializeStream& stream, std::string& value) {
        std::string::size_type length;
        stream >> length;
        value.resize(length);
        stream.read(value.data(), length);
    }
};

// std::vector<T>
template <typename T>
struct SerializeTraits<std::vector<T>> {
    static void serialize(SerializeStream& stream, const std::vector<T>& value) {
        auto capacity = value.capacity();
        auto size = value.size();
        for (size_t i = 0; i < size; i++) {
            stream << value[size - i - 1];
        }
        stream << capacity << size;
    }

    static void deserialize(DeserializeStream& stream, std::vector<T>& value) {
        size_t capacity, size;
        stream >> size >> capacity;
        value.reserve(capacity);
        value.resize(size);
        for (size_t i = 0; i < size; i++) {
            stream >> value[i];
        }
    }
};

// std::pair<T1, T2>
template <typename T1, typename T2>
struct SerializeTraits<std::pair<T1, T2>> {
    static void serialize(SerializeStream& stream, const std::pair<T1, T2>& value) {
        stream << value.first << value.second;
    }

    static void deserialize(DeserializeStream& stream, std::pair<T1, T2>& value) {
        stream >> value.second >> value.first;
    }
};

// std::map<T1, T2>
template <typename T1, typename T2>
struct SerializeTraits<std::map<T1, T2>> {
    static void serialize(SerializeStream& stream, const std::map<T1, T2>& value) {
        auto size = value.size();
        for (auto& pair : value) {
            stream << pair;
        }
        stream << size;
    }

    static void deserialize(DeserializeStream& stream, std::map<T1, T2>& value) {
        size_t size;
        stream >> size;
        std::pair<T1, T2> pair;
        for (size_t i = 0; i < size; i++) {
            stream >> pair;
            value.insert(std::move(pair));
        }
    }
};

}  // namespace wen