#pragma once

namespace SongDetailsCache {
    /// @brief Enum describing common mods that maps tend to use
    enum class MapMods {
        None =              0x0,
        NoodleExtensions =  0x1,
        NE =                0x1, // alias
        MappingExtensions = 0x2,
        ME =                0x2, // alias
        Chroma =            0x4,
        Cinema =            0x8,
    };

    static MapMods operator |(const MapMods& lhs, const MapMods& rhs) {
        return static_cast<MapMods>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }
    static MapMods& operator |=(MapMods& lhs, const MapMods& rhs) {
        lhs = lhs | rhs;
        return lhs;
    }

    static MapMods operator &(const MapMods& lhs, const MapMods& rhs) {
        return static_cast<MapMods>(static_cast<int>(lhs) & static_cast<int>(rhs));
    }
    static MapMods& operator &=(MapMods& lhs, const MapMods& rhs) {
        lhs = lhs & rhs;
        return lhs;
    }

    static bool hasFlags(const MapMods& lhs, const MapMods& rhs) {
        return (lhs & rhs) == rhs;
    }
}

// if we have fmt, add formatting methods
#if __has_include("fmt/core.h")
#include <fmt/core.h>
#include <sstream>
template <> struct fmt::formatter<::SongDetailsCache::MapMods> : formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(::SongDetailsCache::MapMods c, FormatContext& ctx) {
        std::stringstream result;
        result << "[";
        bool any = false;
        bool first = true;
        if (SongDetailsCache::hasFlags(c, SongDetailsCache::MapMods::NoodleExtensions)) {
            result << "NoodleExtensions";
            first = false;
            any = true;
        }
        if (SongDetailsCache::hasFlags(c, SongDetailsCache::MapMods::MappingExtensions)) {
            if (first) result << "MappingExtensions";
            else result << ", MappingExtensions";
            first = false;
            any = true;
        }
        if (SongDetailsCache::hasFlags(c, SongDetailsCache::MapMods::Chroma)) {
            if (first) result << "Chroma";
            else result << ", Chroma";
            first = false;
            any = true;
        }
        if (SongDetailsCache::hasFlags(c, SongDetailsCache::MapMods::Cinema)) {
            if (first) result << "Cinema";
            else result << ", Cinema";
            first = false;
            any = true;
        }

        if (!any) result << "None";
        result << "]";
        return result.str();
    }
};
#endif