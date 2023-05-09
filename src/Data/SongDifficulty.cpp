#include "Data/SongDifficulty.hpp"
#include "Data/SongDetailsContainer.hpp"
#include "Data/Song.hpp"
#include "SongProto.pb.h"

namespace SongDetailsCache {
    const SongDifficulty SongDifficulty::none{0, nullptr};
    SongDifficulty::SongDifficulty(std::size_t songIndex, const Structs::SongDifficultyProto* proto) noexcept :
        songIndex(songIndex),
        characteristic(proto && proto->has_characteristic() ? static_cast<MapCharacteristic>(proto->characteristic()) : MapCharacteristic::Standard),
        difficulty(proto && proto->has_difficulty() ? static_cast<MapDifficulty>(proto->difficulty()) : MapDifficulty::ExpertPlus),
        starsSS(proto ? proto->starst100() / 100.0f : 0),
        starsBL(proto ? proto->starst100bl() / 100.0f : 0),
        njs(proto ? proto->njst100() / 100.0f : 0),
        bombs(proto ? proto->bombs() : 0),
        notes(proto ? proto->notes() : 0),
        obstacles(proto ? proto->obstacles() : 0),
        mods(proto && proto->has_mods() ? static_cast<MapMods>(proto->mods()) : MapMods::None)
        {}

    const Song& SongDifficulty::song() const noexcept {
        return SongDetailsContainer::songs->operator[](songIndex);
    }

    bool SongDifficulty::ranked() const noexcept {
        return this->rankedBL() || this->rankedSS();
    }

    bool SongDifficulty::rankedBL() const noexcept {
        return starsBL > 0 && hasFlags(song().rankedStates, RankedStates::BeatleaderRanked);
    }

    bool SongDifficulty::rankedSS() const noexcept {
        return starsSS > 0 && hasFlags(song().rankedStates, RankedStates::ScoresaberRanked);
    }

    [[deprecated("This function is deprecated. It only works for scoresaber ranked difficulties and is not accurate.")]]
    float SongDifficulty::approximatePpValue() const noexcept {
        if (starsSS <= 0.05 || !rankedSS()) return 0;
        return starsSS * 43.146f;
    }

    bool SongDifficulty::usesMods(const MapMods& usedMods) const noexcept {
        return hasFlags(mods, usedMods);
    }
}