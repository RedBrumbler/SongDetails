#include "Data/SongDifficulty.hpp"
#include "Data/SongDetailsContainer.hpp"
#include "Data/Song.hpp"
#include "SongProto.pb.h"

namespace SongDetailsCache {
    const SongDifficulty SongDifficulty::none{0, nullptr};
    SongDifficulty::SongDifficulty(std::size_t songIndex, const Structs::SongDifficultyProto* proto) noexcept :
        songIndex(songIndex),
        characteristic(static_cast<MapCharacteristic>(proto && proto->has_characteristic() ? proto->characteristic() : 1)),
        difficulty(static_cast<MapDifficulty>(proto && proto->has_difficulty() ? proto->difficulty() : 4)),
        stars(proto ? proto->starst100() / 100.0f : 0),
        njs(proto ? proto->njst100() / 100.0f : 0),
        bombs(proto ? proto->bombs() : 0),
        notes(proto ? proto->notes() : 0),
        obstacles(proto ? proto->obstacles() : 0),
        mods(static_cast<MapMods>(proto && proto->has_mods() ? proto->mods() : 0))
        {}

    const Song& SongDifficulty::song() const noexcept {
        return SongDetailsContainer::songs->operator[](songIndex);
    }

    bool SongDifficulty::ranked() const noexcept {
        return stars > 0 && song().rankedStatus == RankedStatus::Ranked;
    }

    float SongDifficulty::approximatePpValue() const noexcept {
        if (stars <= 0.05 || !ranked()) return 0;
        return stars * 43.146f;
    }

    bool SongDifficulty::usesMods(const MapMods& usedMods) const noexcept {
        return hasFlags(mods, usedMods);
    }
}