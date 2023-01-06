#pragma once

#include <vector>
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

    static std::vector<std::string> toVectorOfStrings(const MapMods& mods) {
        std::vector<std::string> result{};
        if (hasFlags(mods, MapMods::NoodleExtensions)) result.emplace_back("Noodle Extensions");
        if (hasFlags(mods, MapMods::MappingExtensions)) result.emplace_back("Mapping Extensions");
        if (hasFlags(mods, MapMods::Chroma)) result.emplace_back("Chroma");
        if (hasFlags(mods, MapMods::Cinema)) result.emplace_back("Cinema");
        return result;
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
        auto vec = ::SongDetailsCache::toVectorOfStrings(c);
        if (vec.empty()) return "None";
        std::stringstream result;
        for (bool first = true; const auto& v : vec) {
            if (first) first = false;
            else result << ", ";
            result << v;
        }
        return result.str();
    }
};
#endif