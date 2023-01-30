#pragma once

#include "Data/SongDetailsContainer.hpp"
#include <string>
#include <vector>
#include <span>
#include <future>
#include <unordered_map>

namespace SongDetailsCache {
    class HexUtil {
        public:
            static std::string ByteArrayToHex(std::span<uint8_t> bytes);
            static std::string SongBytesToHash(std::size_t index);

            /// @throw throws if the passed hex string contains non-hex letters
            static std::vector<uint8_t> ToBytes(std::string_view hex);
            static SongHash ToSongHash(std::string_view hex);
    };

    class WebUtil {
        public:
            struct WebResponse {
                long httpCode;
                std::string headers;
                std::string content;
            };
            static std::future<WebResponse> GetAsync(std::string_view url, uint32_t timeout, const std::unordered_map<std::string, std::string>& headers);
        private:
            static WebResponse GetAsync_internal(std::string_view url, uint32_t timeout, const std::unordered_map<std::string, std::string>& headers);
    };
}