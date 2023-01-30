#include "Utils.hpp"
#include "logging.hpp"
#include "Data/SongDetailsContainer.hpp"

#include "libcurl/shared/curl.h"
#include "libcurl/shared/easy.h"

#include <unordered_map>
#include <sstream>
#include <iomanip>

#define USER_AGENT "https://github.com/RedBrumbler/SongDetails/" VERSION
namespace SongDetailsCache {
    static const std::unordered_map<uint8_t, char> hexToChar = {
        {0x0, '0'}, {0x1, '1'}, {0x2, '2'}, {0x3, '3'},
        {0x4, '4'}, {0x5, '5'}, {0x6, '6'}, {0x7, '7'},
        {0x8, '8'}, {0x9, '9'}, {0xa, 'a'}, {0xb, 'b'},
        {0xc, 'c'}, {0xd, 'd'}, {0xe, 'e'}, {0xf, 'f'},
    };

    std::string HexUtil::ByteArrayToHex(std::span<uint8_t> bytes) {
        std::stringstream str;
        static const char hex[] = "0123456789ABCDEF";
        for (auto v : bytes) {
            str << hex[(v >> 4) & 0xF] << hex[v & 0xF];
        }
        return str.str();
    }

    std::string HexUtil::SongBytesToHash(std::size_t index) {
        std::string result;
        result.resize(SongDetailsContainer::HASH_SIZE_BYTES * 2);
        const uint8_t* bytes = SongDetailsContainer::hashBytes->operator[](index);

        uint16_t* resultP2 = (uint16_t*)result.data();
        for (std::size_t i = 0; i < SongDetailsContainer::HASH_SIZE_BYTES; i++) {
            static const char hex[] = "0123456789ABCDEF";
            result[i*2+0] = hex[(bytes[i] >> 4) & 0xF];
			result[i*2+1] = hex[bytes[i] & 0xF];
        }
        return result;
    }

    static const std::unordered_map<char, uint8_t> charToHex = {
        {'a', 0xa}, {'A', 0xa}, {'b', 0xb}, {'B', 0xb},
        {'c', 0xc}, {'C', 0xc}, {'d', 0xd}, {'D', 0xd},
        {'e', 0xe}, {'E', 0xe}, {'f', 0xf}, {'F', 0xf},
        {'0', 0x0}, {'1', 0x1}, {'2', 0x2}, {'3', 0x3},
        {'4', 0x4}, {'5', 0x5}, {'6', 0x6}, {'7', 0x7},
        {'8', 0x8}, {'9', 0x9}
    };

    SongHash HexUtil::ToSongHash(std::string_view hex) {
        try {
            return *(SongHash*)ToBytes(hex).data();
        } catch (const std::exception& e) {
            return SongHash();
        }
    }

    std::vector<uint8_t> HexUtil::ToBytes(std::string_view hex) {
        std::vector<uint8_t> result{};
        result.resize(hex.size() / 2);
        char left, right;
        try {
            std::size_t byteIdx = 0;
            for (std::size_t i = 0; i < hex.size(); i += 2, byteIdx++) {
                left = hex[i]; right = hex[i + 1];
                result[byteIdx] = ((charToHex.at(left) << 4) | charToHex.at(right));
            }
        } catch (const std::out_of_range& e) {
            throw "Hex string has a non-hex character";
        }
        return result;
    }


    std::string query_encode(std::string_view s) {
		std::stringstream ret;

		#define IS_BETWEEN(ch, low, high) (ch >= low && ch <= high)
		#define IS_ALPHA(ch) (IS_BETWEEN(ch, 'A', 'Z') || IS_BETWEEN(ch, 'a', 'z'))
		#define IS_DIGIT(ch) IS_BETWEEN(ch, '0', '9')
		#define IS_HEXDIG(ch) (IS_DIGIT(ch) || IS_BETWEEN(ch, 'A', 'F') || IS_BETWEEN(ch, 'a', 'f'))

		for(size_t i = 0; i < s.size();) {
			char ch = s[i++];

			if (IS_ALPHA(ch) || IS_DIGIT(ch))
			{
				ret << ch;
			}
			else if ((ch == '%') && IS_HEXDIG(s[i+0]) && IS_HEXDIG(s[i+1]))
			{
				ret << s.substr(i-1, 3);
				i += 2;
			}
			else
			{
				switch (ch)
				{
					case '-':
					case '.':
					case '_':
					case '~':
					case '!':
					case '$':
					case '&':
					case '\'':
					case '(':
					case ')':
					case '*':
					case '+':
					case ',':
					case ';':
					case '=':
					case ':':
					case '@':
					case '/':
					case '?':
					case '[':
					case ']':
						ret << ch;
						break;

					default:
					{
						static const char hex[] = "0123456789ABCDEF";
						ret << "%" << hex[(ch >> 4) & 0xF] << hex[ch & 0xF];
						break;
					}
				}
			}
		}

		return ret.str();
	}

	std::size_t CurlWrite_CallbackFunc_StdString(void *contents, std::size_t size, std::size_t nmemb, std::string *s) {
		std::size_t newLength = size * nmemb;
		try {
			s->append((char*)contents, newLength);
		} catch(std::bad_alloc &e) {
			//handle memory problem
			CRITICAL("Failed to allocate string of size: %lu", newLength);
			return 0;
		}
		return newLength;
	}

    std::future<WebUtil::WebResponse> WebUtil::GetAsync(std::string_view url, uint32_t timeout, const std::unordered_map<std::string, std::string>& headers) {
        return std::async(std::launch::async, std::bind(&WebUtil::GetAsync_internal, url, timeout, headers));
    }

    WebUtil::WebResponse WebUtil::GetAsync_internal(std::string_view url, uint32_t timeout, const std::unordered_map<std::string, std::string>& headers) {
        WebResponse response;

        // Init curl
        auto* curl = curl_easy_init();
        struct curl_slist *curl_headers = NULL;
        for (const auto& [key, value] : headers) {
            curl_headers = curl_slist_append(curl_headers, fmt::format("{}: {}", key, value).c_str());
        }

        // Set curl_headers
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);

        curl_easy_setopt(curl, CURLOPT_URL, query_encode(url).c_str());

        // Don't wait forever, time out after TIMEOUT seconds.
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);

        // Follow HTTP redirects if necessary.
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.content);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        auto res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK) {
            ERROR("curl_easy_perform() failed: %u: %s", res, curl_easy_strerror(res));
        }
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.httpCode);
        curl_easy_cleanup(curl);
        return response;
    }
}