#include "Data/SongDetailsContainer.hpp"
#include "logging.hpp"
#include "SongDetails.hpp"
#include "Data/DataGetter.hpp"
#include <sys/stat.h>
#include "StopWatch.hpp"

#include "SongProto.pb.h"

namespace SongDetailsCache {
    shared_ptr_vector<uint32_t> SongDetailsContainer::keys;
    shared_ptr_vector<SongHash> SongDetailsContainer::hashBytes;
    shared_ptr_vector<uint32_t> SongDetailsContainer::hashBytesLUT;
    shared_ptr_vector<std::string> SongDetailsContainer::songNames;
    shared_ptr_vector<std::string> SongDetailsContainer::songAuthorNames;
    shared_ptr_vector<std::string> SongDetailsContainer::levelAuthorNames;
    shared_ptr_vector<std::string> SongDetailsContainer::uploaderNames;

    std::chrono::sys_seconds SongDetailsContainer::scrapeEndedTimeUnix;
    std::chrono::seconds SongDetailsContainer::updateThrottle = std::chrono::seconds(0);

    shared_ptr_vector<Song> SongDetailsContainer::songs;
    shared_ptr_vector<SongDifficulty> SongDetailsContainer::difficulties;

    UnorderedEventCallback<> SongDetailsContainer::dataAvailableOrUpdatedInternal;
    UnorderedEventCallback<> SongDetailsContainer::dataLoadFailedInternal;

    std::future<void> SongDetailsContainer::Load(bool reload, int acceptableAgeHours) {
        return std::async(std::launch::async, std::bind(&SongDetailsContainer::Load_internal, reload, acceptableAgeHours));
    }

    void SongDetailsContainer::Load_internal(bool reload, int acceptableAgeHours) {
        using namespace std::chrono;
        using namespace std::chrono_literals;
        struct stat fInfo = {0};

        bool shouldLoadFresh = false;
        if (stat(DataGetter::cachePath().c_str(), &fInfo) == 0) { // only succeeds if exists
            if (!get_isDataAvailable() || reload) {
                auto cachedStreamOpt = DataGetter::ReadCachedDatabase();
                if (cachedStreamOpt) {
                    DEBUG("Processing cached data!");
                    StopWatch sw; sw.Start();
                    Process(*cachedStreamOpt, false);
                    DEBUG("Processed cached data in {}ms", sw.EllapsedMilliseconds());
                    cachedStreamOpt->close();
                }
            }

            if (std::chrono::system_clock::now().time_since_epoch() - scrapeEndedTimeUnix.time_since_epoch() > std::chrono::hours(std::max(acceptableAgeHours, 1))) {
                shouldLoadFresh = true;
            }
        } else { // didn't exist or otherwise failed
            shouldLoadFresh = true;
        }

        if (!shouldLoadFresh || (system_clock::now().time_since_epoch() - updateThrottle < 30min)) {
            SongDetails::isLoading = false;
            return;
        }

        for (const auto& [src, _] : DataGetter::dataSources) {
            try {
                auto db = DataGetter::UpdateAndReadDatabase(src).get();
                if (!db.has_value()) {
                    break;
                }

                StopWatch sw; sw.Start();
                Process(*db->data);
                DEBUG("Processed new data in {}ms", sw.EllapsedMilliseconds());
                sw.Restart();
                DataGetter::WriteCachedDatabase(*db).wait();
                DEBUG("Wrote data in {}ms", sw.EllapsedMilliseconds());

                if (get_isDataAvailable()) {
                    break;
                }

                ERROR("Data load failed for unknown reason");
            } catch (...) {
                ERROR("Failed to download from source {}", src);
            }
        }

        if (!get_isDataAvailable()) {
            dataLoadFailedInternal.invoke();
        }
        SongDetails::isLoading = false;
    }

    void SongDetailsContainer::Process(std::istream& istream, bool force) {
        if (!force && songs) return;
        StopWatch sw; sw.Start();
        Structs::SongProtoContainer parsedContainer;
        if (!parsedContainer.ParseFromIstream(&istream)) {
            ERROR("Failed to parse Song container from istream!");
            return;
        }

        INFO("Parsed protobuf in {}ms", sw.EllapsedMilliseconds());
        Process(parsedContainer, force);
    }

    void SongDetailsContainer::Process(const std::vector<uint8_t>& data, bool force) {
        if (!force && songs) return;

        StopWatch sw; sw.Start();
        Structs::SongProtoContainer parsedContainer;
        if (!parsedContainer.ParseFromArray(data.data(), data.size())) {
            ERROR("Failed to parse Song container from data!");
            return;
        }

        INFO("Parsed protobuf in {}ms", sw.EllapsedMilliseconds());
        Process(parsedContainer, force);
    }

    void SongDetailsContainer::Process(const Structs::SongProtoContainer& parsedContainer, bool force) {
        if (!force && songs) return;

        scrapeEndedTimeUnix = std::chrono::sys_seconds(std::chrono::seconds(parsedContainer.scrapeendedtimeunix()));
        auto& parsedField = parsedContainer.songs();

        if (parsedField.size() == 0) {
            ERROR("Parsing data yielded no songs!");
            return;
        }
        const auto len = parsedField.size();
        StopWatch sw; sw.Start();

        std::vector<const SongDetailsCache::Structs::SongProto*> parsed;
        parsed.resize(len);
        INFO("Got {} songs in data", len);
        for (std::size_t idx = 0; const auto& s : parsedField) parsed[idx++] = &s;
        // sort a copied vector
        std::stable_sort(parsed.begin(), parsed.end(), [](auto lhs, auto rhs){
            return lhs->mapid() < rhs->mapid();
        });

        INFO("Sorted input in {}ms", sw.EllapsedMilliseconds());

        // we run everything with resize so everything is already valid memory
        auto newSongs = make_shared_vec<Song>();
        newSongs->reserve(len);

		auto newKeys = make_shared_vec<uint32_t>();
        newKeys->reserve(len);
		auto newHashes = make_shared_vec<SongHash>();
        newHashes->reserve(len);
		auto newHashesLUT = make_shared_vec<uint32_t>();
        newHashesLUT->reserve(len);

		auto newSongNames = make_shared_vec<std::string>();
        newSongNames->reserve(len);
		auto newSongAuthorNames = make_shared_vec<std::string>();
        newSongAuthorNames->reserve(len);
		auto newLevelAuthorNames = make_shared_vec<std::string>();
        newLevelAuthorNames->reserve(len);
		auto newUploaderNames = make_shared_vec<std::string>();
        newUploaderNames->reserve(len);

        auto newDiffs = make_shared_vec<SongDifficulty>();
        std::size_t diffLen = 0;
        for (auto s : parsed) diffLen += s->difficulties().size();
        newDiffs->reserve(diffLen);
        sw.Restart();
        std::size_t diffIndex = 0;
        for (std::size_t i = 0; i < len; i++) {
            const auto& parsedSong = parsed[i];
            uint8_t diffCount = std::min(255, parsedSong->difficulties_size());
            const auto& builtSong = newSongs->emplace_back(i, diffIndex, diffCount, parsedSong);
            newKeys->emplace_back(parsedSong->mapid());
            newHashes->emplace_back(parsedSong->hashbytes());

            newSongNames->emplace_back(parsedSong->songname());
            newSongAuthorNames->emplace_back(parsedSong->songauthorname());
            newLevelAuthorNames->emplace_back(parsedSong->levelauthorname());
            newUploaderNames->emplace_back(parsedSong->uploadername());
            if (parsedSong->difficulties().empty()) continue;

            for (const auto& diff : parsedSong->difficulties()) {
                newDiffs->emplace_back(i, &diff);
                diffIndex++;
            }
        }
        INFO("Processed songs in {}ms", sw.EllapsedMilliseconds());
        sw.Restart();
        // we need to sort the hashes, but not the original vector, let's just use pointers because that's more efficient than by copy
        std::vector<const Song*> sortedByHashes{};
        sortedByHashes.resize(len);
        for (std::size_t idx = 0; const auto& s : *newSongs) sortedByHashes[idx++] = &s;
        std::stable_sort(sortedByHashes.begin(), sortedByHashes.end(), [&](const Song* lhs, const Song* rhs){
            return newHashes->operator[](lhs->index).c1 < newHashes->operator[](rhs->index).c1;
        });

        for (const auto& sorted : sortedByHashes) {
            newHashesLUT->emplace_back(sorted->index);
        }

        INFO("Made hashes LUT in {}ms", sw.EllapsedMilliseconds());

        INFO("Finished Processing");
        if (!force && songs) return;

        INFO("Assigning new values");
        songs = newSongs;
        keys = newKeys;
        hashBytes = newHashes;
        hashBytesLUT = newHashesLUT;

        songNames = newSongNames;
        songAuthorNames = newSongAuthorNames;
        levelAuthorNames = newLevelAuthorNames;
        uploaderNames = newUploaderNames;

        difficulties = newDiffs;

        if (get_isDataAvailable()) {
            try {
                dataAvailableOrUpdatedInternal.invoke();
            } catch (const std::exception& _) {}
        }
    }

    SongHash::SongHash(const std::string& str) : SongHash() { std::memcpy((void*)&c1, str.c_str(), SongDetailsContainer::HASH_SIZE_BYTES); }
    SongHash::SongHash() : c1(0), c2(0), c3(0) {}
}