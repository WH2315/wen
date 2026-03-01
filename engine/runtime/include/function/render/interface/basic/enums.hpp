#pragma once

#include <vulkan/vulkan.hpp>

namespace wen::Renderer {

const std::string SWAPCHAIN_IMAGE_ATTACHMENT = "swapchain_image_attachment";
const std::string DEPTH_ATTACHMENT = "depth_attachment";
const std::string IMGUI_DOCKING_ATTACHMENT = "imgui_docking_attachment";
const std::string EXTERNAL_SUBPASS = "external_subpass";

template <typename FlagBitsType>
struct FlagTraits {
    static VULKAN_HPP_CONST_OR_CONSTEXPR bool isBitmask = false;
};

template <typename BitType>
class Flags {
public:
    using MaskType = typename std::underlying_type<BitType>::type;

    // constructors
    VULKAN_HPP_CONSTEXPR Flags() VULKAN_HPP_NOEXCEPT : m_mask(0) {}

    VULKAN_HPP_CONSTEXPR Flags(BitType bit) VULKAN_HPP_NOEXCEPT
        : m_mask(static_cast<MaskType>(bit)) {}

    VULKAN_HPP_CONSTEXPR Flags(Flags<BitType> const& rhs) VULKAN_HPP_NOEXCEPT = default;

    VULKAN_HPP_CONSTEXPR explicit Flags(MaskType flags) VULKAN_HPP_NOEXCEPT
        : m_mask(flags) {}

    // relational operators
#if defined(VULKAN_HPP_HAS_SPACESHIP_OPERATOR)
    auto operator<=>(Flags<BitType> const&) const = default;
#else
    VULKAN_HPP_CONSTEXPR bool operator<(Flags<BitType> const& rhs) const
        VULKAN_HPP_NOEXCEPT {
        return m_mask < rhs.m_mask;
    }

    VULKAN_HPP_CONSTEXPR bool operator<=(Flags<BitType> const& rhs) const
        VULKAN_HPP_NOEXCEPT {
        return m_mask <= rhs.m_mask;
    }

    VULKAN_HPP_CONSTEXPR bool operator>(Flags<BitType> const& rhs) const
        VULKAN_HPP_NOEXCEPT {
        return m_mask > rhs.m_mask;
    }

    VULKAN_HPP_CONSTEXPR bool operator>=(Flags<BitType> const& rhs) const
        VULKAN_HPP_NOEXCEPT {
        return m_mask >= rhs.m_mask;
    }

    VULKAN_HPP_CONSTEXPR bool operator==(Flags<BitType> const& rhs) const
        VULKAN_HPP_NOEXCEPT {
        return m_mask == rhs.m_mask;
    }

    VULKAN_HPP_CONSTEXPR bool operator!=(Flags<BitType> const& rhs) const
        VULKAN_HPP_NOEXCEPT {
        return m_mask != rhs.m_mask;
    }
#endif

    // logical operator
    VULKAN_HPP_CONSTEXPR bool operator!() const VULKAN_HPP_NOEXCEPT { return !m_mask; }

    // bitwise operators
    VULKAN_HPP_CONSTEXPR Flags<BitType> operator&(Flags<BitType> const& rhs) const
        VULKAN_HPP_NOEXCEPT {
        return Flags<BitType>(m_mask & rhs.m_mask);
    }

    VULKAN_HPP_CONSTEXPR Flags<BitType> operator|(Flags<BitType> const& rhs) const
        VULKAN_HPP_NOEXCEPT {
        return Flags<BitType>(m_mask | rhs.m_mask);
    }

    VULKAN_HPP_CONSTEXPR Flags<BitType> operator^(Flags<BitType> const& rhs) const
        VULKAN_HPP_NOEXCEPT {
        return Flags<BitType>(m_mask ^ rhs.m_mask);
    }

    VULKAN_HPP_CONSTEXPR Flags<BitType> operator~() const VULKAN_HPP_NOEXCEPT {
        return Flags<BitType>(m_mask ^ FlagTraits<BitType>::allFlags.m_mask);
    }

    // assignment operators
    VULKAN_HPP_CONSTEXPR_14 Flags<BitType>& operator=(Flags<BitType> const& rhs)
        VULKAN_HPP_NOEXCEPT = default;

    VULKAN_HPP_CONSTEXPR_14 Flags<BitType>& operator|=(Flags<BitType> const& rhs)
        VULKAN_HPP_NOEXCEPT {
        m_mask |= rhs.m_mask;
        return *this;
    }

    VULKAN_HPP_CONSTEXPR_14 Flags<BitType>& operator&=(Flags<BitType> const& rhs)
        VULKAN_HPP_NOEXCEPT {
        m_mask &= rhs.m_mask;
        return *this;
    }

    VULKAN_HPP_CONSTEXPR_14 Flags<BitType>& operator^=(Flags<BitType> const& rhs)
        VULKAN_HPP_NOEXCEPT {
        m_mask ^= rhs.m_mask;
        return *this;
    }

    // cast operators
    explicit VULKAN_HPP_CONSTEXPR operator bool() const VULKAN_HPP_NOEXCEPT {
        return !!m_mask;
    }

    explicit VULKAN_HPP_CONSTEXPR operator MaskType() const VULKAN_HPP_NOEXCEPT {
        return m_mask;
    }

#if defined(VULKAN_HPP_FLAGS_MASK_TYPE_AS_PUBLIC)
public:
#else
private:
#endif
    MaskType m_mask;
};

#if !defined(VULKAN_HPP_HAS_SPACESHIP_OPERATOR)
// relational operators only needed for pre C++20
template <typename BitType>
VULKAN_HPP_CONSTEXPR bool operator<(BitType bit,
                                    Flags<BitType> const& flags) VULKAN_HPP_NOEXCEPT {
    return flags.operator>(bit);
}

template <typename BitType>
VULKAN_HPP_CONSTEXPR bool operator<=(BitType bit,
                                     Flags<BitType> const& flags) VULKAN_HPP_NOEXCEPT {
    return flags.operator>=(bit);
}

template <typename BitType>
VULKAN_HPP_CONSTEXPR bool operator>(BitType bit,
                                    Flags<BitType> const& flags) VULKAN_HPP_NOEXCEPT {
    return flags.operator<(bit);
}

template <typename BitType>
VULKAN_HPP_CONSTEXPR bool operator>=(BitType bit,
                                     Flags<BitType> const& flags) VULKAN_HPP_NOEXCEPT {
    return flags.operator<=(bit);
}

template <typename BitType>
VULKAN_HPP_CONSTEXPR bool operator==(BitType bit,
                                     Flags<BitType> const& flags) VULKAN_HPP_NOEXCEPT {
    return flags.operator==(bit);
}

template <typename BitType>
VULKAN_HPP_CONSTEXPR bool operator!=(BitType bit,
                                     Flags<BitType> const& flags) VULKAN_HPP_NOEXCEPT {
    return flags.operator!=(bit);
}
#endif

// bitwise operators
template <typename BitType>
VULKAN_HPP_CONSTEXPR Flags<BitType> operator&(BitType bit, Flags<BitType> const& flags)
    VULKAN_HPP_NOEXCEPT {
    return flags.operator&(bit);
}

template <typename BitType>
VULKAN_HPP_CONSTEXPR Flags<BitType> operator|(BitType bit, Flags<BitType> const& flags)
    VULKAN_HPP_NOEXCEPT {
    return flags.operator|(bit);
}

template <typename BitType>
VULKAN_HPP_CONSTEXPR Flags<BitType> operator^(BitType bit, Flags<BitType> const& flags)
    VULKAN_HPP_NOEXCEPT {
    return flags.operator^(bit);
}

// bitwise operators on BitType
template <typename BitType,
          typename std::enable_if<FlagTraits<BitType>::isBitmask, bool>::type = true>
VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR Flags<BitType> operator&(
    BitType lhs, BitType rhs) VULKAN_HPP_NOEXCEPT {
    return Flags<BitType>(lhs) & rhs;
}

template <typename BitType,
          typename std::enable_if<FlagTraits<BitType>::isBitmask, bool>::type = true>
VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR Flags<BitType> operator|(
    BitType lhs, BitType rhs) VULKAN_HPP_NOEXCEPT {
    return Flags<BitType>(lhs) | rhs;
}

template <typename BitType,
          typename std::enable_if<FlagTraits<BitType>::isBitmask, bool>::type = true>
VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR Flags<BitType> operator^(
    BitType lhs, BitType rhs) VULKAN_HPP_NOEXCEPT {
    return Flags<BitType>(lhs) ^ rhs;
}

template <typename BitType,
          typename std::enable_if<FlagTraits<BitType>::isBitmask, bool>::type = true>
VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR Flags<BitType> operator~(BitType bit)
    VULKAN_HPP_NOEXCEPT {
    return ~(Flags<BitType>(bit));
}

enum class ShaderStage : uint32_t {
    eVertex = static_cast<uint32_t>(vk::ShaderStageFlagBits::eVertex),
    eFragment = static_cast<uint32_t>(vk::ShaderStageFlagBits::eFragment),
    eRaygen = static_cast<uint32_t>(vk::ShaderStageFlagBits::eRaygenKHR),
    eMiss = static_cast<uint32_t>(vk::ShaderStageFlagBits::eMissKHR),
    eClosestHit = static_cast<uint32_t>(vk::ShaderStageFlagBits::eClosestHitKHR),
    eIntersection = static_cast<uint32_t>(vk::ShaderStageFlagBits::eIntersectionKHR),\
    eCompute = static_cast<uint32_t>(vk::ShaderStageFlagBits::eCompute),
};

using ShaderStages = Flags<ShaderStage>;

template <>
struct FlagTraits<ShaderStage> {
    static VULKAN_HPP_CONST_OR_CONSTEXPR bool isBitmask = true;
    static VULKAN_HPP_CONST_OR_CONSTEXPR ShaderStages allFlags = ShaderStage::eVertex | ShaderStage::eFragment | ShaderStage::eRaygen | ShaderStage::eMiss | ShaderStage::eClosestHit | ShaderStage::eIntersection;
};

enum class AttachmentType {
    eColor,
    eDepth,
    eStencil,
    eRGBA8Snorm,
    eRGBA32Sfloat,
};

enum class InputRate {
    eVertex,
    eInstance,
};

enum class VertexType {
    eInt32,
    eInt32x2,
    eInt32x3,
    eInt32x4,
    eUint32,
    eUint32x2,
    eUint32x3,
    eUint32x4,
    eFloat,
    eFloat2,
    eFloat3,
    eFloat4,
};

enum class IndexType {
    eUint16,
    eUint32,
};

}  // namespace wen::Renderer