#pragma once

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
            static std::vector<uint8_t> ToBytes(const std::string_view& hex);
    };

    class WebUtil {
        public:
            struct WebResponse {
                long httpCode;
                std::string headers;
                std::string content;
            };
            static std::future<WebResponse> GetAsync(const std::string_view& url, uint32_t timeout, const std::unordered_map<std::string, std::string>& headers);
        private:
            static WebResponse GetAsync_internal(const std::string_view& url, uint32_t timeout, const std::unordered_map<std::string, std::string>& headers);
    };
}