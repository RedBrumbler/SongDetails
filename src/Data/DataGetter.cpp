#include "Data/DataGetter.hpp"
#include "Utils.hpp"
#include "logging.hpp"

#include <vector>
#include <fstream>
#include <sstream>
#include <fmt/core.h>
#include <sys/stat.h>
#include "gzip/decompress.hpp"

namespace SongDetailsCache {
    std::filesystem::path DataGetter::basePath{"/sdcard/ModData/com.beatgames.beatsaber/Mods/SongDetails"};
    std::filesystem::path DataGetter::cachePath() { return basePath / "SongDetailsCache.proto"; }
    std::filesystem::path DataGetter::cachePathEtag(std::string_view source) {
        return basePath / fmt::format("SongDetailsCache.proto.{}.etag", source);
    }
    // just copied from the C# binary (order from bottom to top)
    const std::unordered_map<std::string, std::string> DataGetter::dataSources {
		// Caches stuff for 5 hours, bandwidth 512KB/s, but at least its a way to get the data at all for people behind China Firewall
		{ "WGzeyu", "https://beatmods.wgzeyu.com/github/BeatSaberScrappedData/songDetails2.gz" },
		// Caches stuff for 12 hours as backup
		{ "JSDelivr", "https://cdn.jsdelivr.net/gh/andruzzzhka/BeatSaberScrappedData/songDetails2.gz" },
        { "Direct", "https://raw.githubusercontent.com/andruzzzhka/BeatSaberScrappedData/master/songDetails2.gz" },
    };

    std::optional<std::ifstream> DataGetter::ReadCachedDatabase() {
        if (!std::filesystem::exists(cachePath())) return std::nullopt;
        return std::ifstream(cachePath(), std::ios::binary);
    }

    std::future<std::optional<DataGetter::DownloadedDatabase>> DataGetter::UpdateAndReadDatabase(std::string_view dataSourceName) {
        return std::async(std::launch::async, std::bind(&DataGetter::UpdateAndReadDatabase_internal, dataSourceName));
    }

    std::future<void> DataGetter::WriteCachedDatabase(DownloadedDatabase& db) {
        return std::async(std::launch::async, std::bind(&DataGetter::WriteCachedDatabase_internal, db));
    }

    bool DataGetter::HasCachedData(int maximumAgeHours) {
        using namespace std::chrono;
        if (!std::filesystem::exists(cachePath())) return false;
        struct stat fInfo = {0};
        if (stat(cachePath().c_str(), &fInfo) == 0) {
            return seconds(fInfo.st_mtim.tv_sec) > (system_clock::now().time_since_epoch() - hours(maximumAgeHours));
        }

        return false;
    }

    std::optional<DataGetter::DownloadedDatabase> DataGetter::UpdateAndReadDatabase_internal(std::string_view dataSourceName) {
        std::string dataSource;
        for (const auto& [key, value] : dataSources) {
            if (key == dataSourceName) {
                dataSource = value;
            }
        }
        // if we don't find a datasource url just return;
        if (dataSource.empty()) {
            ERROR("Could not find correct datasource for name {}", dataSourceName);
            return std::nullopt;
        }

        std::unordered_map<std::string, std::string> req_headers {
            {"Accept", "*/*"}
        };

        if (std::filesystem::exists(cachePathEtag(dataSourceName))) {
            std::ifstream(cachePathEtag(dataSourceName)) >> req_headers["If-None-Match"];
        }

        auto resp = WebUtil::GetAsync(dataSource, 20, req_headers).get();
        // value is same
        if (resp.httpCode == 304) {
            INFO("Etag was the same, returning nullopt");
            return std::nullopt;
        }
        if (resp.httpCode != 200) {
            ERROR("HTTP response code was {}", resp.httpCode);
            throw std::runtime_error("Failed to dl database");
        }

        DownloadedDatabase downloadedDatabase;
        // assign data source
        downloadedDatabase.source = dataSourceName;

        // get etag from buffer
        std::istringstream buf(resp.headers);
        std::string line;
        while (std::getline(buf, line)) {
            if (line.starts_with("ETag: ")) {
                auto tagStart = line.find_first_of('"');
                auto tagLength = (line.find_last_of('"') + 1) - tagStart;
                downloadedDatabase.etag = line.substr(tagStart, tagLength);
                break;
            }
        }

        // decompress contents as gzip
        downloadedDatabase.data = std::make_shared<std::vector<uint8_t>>();
        gzip::Decompressor decompressor{};
        DEBUG("Decompressing data");
        decompressor.decompress<std::vector<uint8_t>>(*downloadedDatabase.data, resp.content.c_str(), resp.content.size());

        // DEBUG("Downloaded Database source   : {}", downloadedDatabase.source);
        // DEBUG("Downloaded Database etag     : {}", downloadedDatabase.etag);
        // DEBUG("Downloaded Database data     : {}", fmt::ptr(downloadedDatabase.data->data()));
        // DEBUG("Downloaded Database data size: {}", downloadedDatabase.data->size());
        return downloadedDatabase;
    }

    void DataGetter::WriteCachedDatabase_internal(DownloadedDatabase& db) {
        // DEBUG("Writing cached Database with source {}", db.source);
        std::filesystem::create_directories(cachePath().parent_path());
        auto out = std::ofstream(cachePath(), std::ios::binary);
        out.write((char*)db.data->data(), db.data->size());

        // DEBUG("Writing etag as well: '{}'", db.etag);
        auto etagPath = cachePathEtag(db.source);
        auto etagOut = std::ofstream(etagPath, std::ios::binary);
        etagOut.write(db.etag.data(), db.etag.size());
    }
}