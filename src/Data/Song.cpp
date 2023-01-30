#include "Data/Song.hpp"
#include "Data/SongDetailsContainer.hpp"
#include "SongProto.pb.h"
#include "Utils.hpp"

namespace SongDetailsCache {
    const Song Song::none(-1, 0, 0, nullptr);
    Song::Song(std::size_t index, std::size_t diffOffset, uint8_t diffCount, const Structs::SongProto* proto) noexcept :
        index(index),
        diffOffset(diffOffset),
        diffCount(diffCount),
        bpm(proto ? proto->bpm() : 0),
        downloadCount(proto ? proto->downloadcount() : 0),
        upvotes(proto ? proto->upvotes() : 0),
        downvotes(proto ? proto->downvotes() : 0),
        uploadTimeUnix(proto ? proto->uploadtimeunix() : 0),
        rankedChangeUnix(proto ? proto->rankedchangeunix() : 0),
        songDurationSeconds(proto ? proto->songdurationseconds() : 0),
        rankedStatus(static_cast<RankedStatus>(proto ? proto->rankedstate() : 0))
        {}

    float Song::min(std::function<float(const SongDifficulty&)> func) const {
        float min = std::numeric_limits<float>::max(); // for minimum, start with the highest value!
        for (const auto& diff : *this) min = std::min(min, func(diff));
        return min;
    }

    float Song::max(std::function<float(const SongDifficulty&)> func) const {
        float max = std::numeric_limits<float>::min(); // for maximum, start with the lowest value!
        for (const auto& diff : *this) max = std::max(max, func(diff));
        return max;
    }

    float Song::rating() const noexcept {
        float tot = upvotes + downvotes;
        if (tot == 0) return 0;
        float tmp = upvotes / tot;
        return (float)(tmp - (tmp - 0.5f) * pow(2, -log10f(tot + 1)));
    }

    float Song::minNJS() const noexcept { return min([](const auto& diff){ return diff.njs; }); }
    float Song::maxNJS() const noexcept { return max([](const auto& diff){ return diff.njs; }); }
    float Song::minStar() const noexcept { 
        if (rankedStatus != RankedStatus::Ranked) return 0.0f;
        return min([](const auto& diff){ return diff.ranked() ? diff.stars : std::numeric_limits<float>::max(); }); 
    }
    float Song::maxStar() const noexcept { 
        if (rankedStatus != RankedStatus::Ranked) return 0.0f;
        return max([](const auto& diff){ return diff.stars; });
    }
    float Song::minPP() const noexcept { 
        if (rankedStatus != RankedStatus::Ranked) return 0.0f;
        return min([](const auto& diff){ return diff.ranked() ? diff.approximatePpValue() : std::numeric_limits<float>::max(); }); 
    }
    float Song::maxPP() const noexcept {
        if (rankedStatus != RankedStatus::Ranked) return 0.0f;
        return max([](const auto& diff){ return diff.approximatePpValue(); }); 
    }

    std::chrono::sys_time<std::chrono::seconds> Song::uploadTime() const noexcept {
        return std::chrono::sys_time<std::chrono::seconds>(std::chrono::seconds(uploadTimeUnix));
    }

    std::chrono::seconds Song::songDuration() const noexcept {
        return std::chrono::seconds(songDurationSeconds);
    }

    std::string Song::key() const noexcept {
        return fmt::format("{:x}", mapId());
    }

    uint32_t Song::mapId() const noexcept {
        return SongDetailsContainer::keys->operator[](index);
    }

    std::string Song::hash() const noexcept {
        return HexUtil::SongBytesToHash(index);
    }

    const std::string& Song::songName() const noexcept {
        return SongDetailsContainer::songNames->operator[](index);
    }

    const std::string& Song::songAuthorName() const noexcept {
        return SongDetailsContainer::songAuthorNames->operator[](index);
    }

    const std::string& Song::levelAuthorName() const noexcept {
        return SongDetailsContainer::levelAuthorNames->operator[](index);
    }

    const std::string& Song::uploaderName() const noexcept {
        return SongDetailsContainer::uploaderNames->operator[](index);
    }

    std::string Song::coverURL() const noexcept {
        auto h = hash();
        std::transform(h.begin(), h.end(), h.begin(), [](auto c){ return std::tolower(c); });
        return fmt::format("https://cdn.beatsaver.com/{}.jpg", h);
    }

    bool Song::GetDifficulty(const SongDifficulty*& outDiff, MapDifficulty diff, MapCharacteristic characteristic) const noexcept {
        for (std::size_t i = 0; i < diffCount; i++) {
            const auto& x = SongDetailsContainer::difficulties->operator[](i + diffOffset);
            if (x.difficulty == diff && x.characteristic == characteristic) {
                outDiff = &x;
                return true;
            }
        }

        outDiff = nullptr;
        return false;
    }

    bool Song::GetDifficulty(const SongDifficulty*& outDiff, MapDifficulty diff, std::string_view characteristic) const noexcept {
        if (characteristic == "Standard")
            return GetDifficulty(outDiff, diff);
        if(characteristic == "360Degree" || characteristic == "Degree360" || characteristic == "ThreeSixtyDegree")
            return GetDifficulty(outDiff, diff, MapCharacteristic::ThreeSixtyDegree);
        if(characteristic == "90Degree" || characteristic == "Degree90" || characteristic == "NinetyDegree")
            return GetDifficulty(outDiff, diff, MapCharacteristic::NinetyDegree);
        MapCharacteristic c;
        if (parse(characteristic, c))
            return GetDifficulty(outDiff, diff, c);
        return false;
    }

    const SongDifficulty& Song::GetDifficulty(MapDifficulty diff, MapCharacteristic characteristic) const noexcept {
        const SongDifficulty* result = nullptr;
        if (GetDifficulty(result, diff, characteristic)) return *result;
        return SongDifficulty::none;
    }

    const SongDifficulty& Song::GetDifficulty(MapDifficulty diff, std::string_view characteristic) const noexcept {
        const SongDifficulty* result = nullptr;
        if (GetDifficulty(result, diff, characteristic)) return *result;
        return SongDifficulty::none;
    }

    Song::difficulty_const_iterator Song::begin() const noexcept {
        return std::next(SongDetailsContainer::difficulties->begin(), diffOffset);
    }
    Song::difficulty_const_iterator Song::end() const noexcept {
        return std::next(SongDetailsContainer::difficulties->begin(), diffOffset + diffCount);
    }
}