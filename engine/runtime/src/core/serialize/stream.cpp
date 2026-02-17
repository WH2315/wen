#include "core/serialize/stream.hpp"

namespace wen {

SerializeStream::SerializeStream() { data_.reserve(32); }

SerializeStream::~SerializeStream() { data_.clear(); }

void* SerializeStream::getSafePtr(size_t size) {
    while (data_.size() + size > data_.capacity()) {
        data_.reserve(static_cast<size_t>(data_.capacity() * 1.5));
    }
    data_.resize(data_.size() + size);
    return &data_.data()[data_.size() - size];
}

void SerializeStream::write(const char* buffer, size_t size) {
    auto ptr = getSafePtr(size);
    memcpy(ptr, buffer, size);
}

DeserializeStream::DeserializeStream(SerializeStream&& stream) {
    data_ = std::move(stream.data_);
    offset_ = data_.size();
}

DeserializeStream::~DeserializeStream() {
    data_.clear();
    offset_ = 0;
}

void DeserializeStream::read(void* buffer, size_t size) {
    offset_ -= size;
    memcpy(buffer, &data_.data()[offset_], size);
}

}  // namespace wen