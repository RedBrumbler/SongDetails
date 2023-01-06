#pragma once

#include "beatsaber-hook/shared/utils/typedefs-wrappers.hpp"
#include <stdint.h>
#include <vector>
#include <future>
#include <istream>
#include <chrono>

#include "Data/Song.hpp"

namespace SongDetailsCache {
    template<typename T>
    using shared_ptr_vector = std::shared_ptr<std::vector<T>>;

    template<typename T>
    inline shared_ptr_vector<T> make_shared_vec() { return std::make_shared<std::vector<T>>(); }
    namespace Structs {
        class SongProtoContainer;
    }
    class SongDetailsContainer {
        public:
            static std::future<void> Load(bool reload = false, int acceptableAgeHours = 1);
        private:
            friend struct Song;
            friend struct SongDifficulty;
            friend class SongDetails;
            friend class HexUtil;
            friend class SongArray;
            friend class DiffArray;

            static constexpr const int HASH_SIZE_BYTES = 20;
            static shared_ptr_vector<uint32_t> keys;
            static shared_ptr_vector<uint8_t> hashBytes;
            static shared_ptr_vector<uint32_t> hashBytesLUT;
            static shared_ptr_vector<std::string> songNames;
            static shared_ptr_vector<std::string> songAuthorNames;
            static shared_ptr_vector<std::string> levelAuthorNames;
            static shared_ptr_vector<std::string> uploaderNames;

            static std::chrono::sys_seconds scrapeEndedTimeUnix;
            static std::chrono::seconds updateThrottle;

            static shared_ptr_vector<Song> songs;
            static shared_ptr_vector<SongDifficulty> difficulties;

            static bool get_isDataAvailable() { return songs && !songs->empty(); }

            static UnorderedEventCallback<> dataAvailableOrUpdatedInternal;
            static UnorderedEventCallback<> dataLoadFailedInternal;
            static void Load_internal(bool reload, int acceptableAgeHours);
            static void Process(const std::vector<uint8_t>& data, bool force = true);
            static void Process(std::istream& istream, bool force = true);
            static void Process(const Structs::SongProtoContainer& parsedContainer, bool force = true);
    };
}