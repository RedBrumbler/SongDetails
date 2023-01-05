#include "SongArray.hpp"
#include "Utils.hpp"
#include "Data/SongDetailsContainer.hpp"

namespace SongDetailsCache {
    SongArray::SongArray() noexcept {};
    SongArray::const_iterator SongArray::begin() const noexcept {
        return SongDetailsContainer::songs->begin();
    }

    SongArray::const_iterator SongArray::end() const noexcept {
        return SongDetailsContainer::songs->end();
    }

    bool SongArray::empty() const noexcept {
        return SongDetailsContainer::songs->empty();
    }

    std::size_t SongArray::size() const noexcept {
        return SongDetailsContainer::songs->size();
    }

    const Song& SongArray::operator [](std::size_t index) const noexcept {
        return SongDetailsContainer::songs->operator[](index);
    }

    const Song& SongArray::at(std::size_t index) const noexcept {
        if (!SongDetailsContainer::songs || index < 0 || index > SongDetailsContainer::songs->size()) return Song::none;
        return operator[](index);
    }

	static constexpr const float hashLookupDivisorInverse = 1.0f / (float)std::numeric_limits<uint32_t>::max();

    bool SongArray::FindByHash(const std::string_view& hash, const Song*& out) {
        if (hash.size() != 40) {
            out = &Song::none;
            return false;
        }

        auto bytes = HexUtil::ToBytes(hash);
        auto _a = bytes.data();
        auto c1 = *(uint32_t*)_a;
        auto c2 = *(uint64_t*)(_a + 4);
        auto c3 = *(uint64_t*)(_a + 12);
        auto comp = [&](uint8_t* a) -> bool {
            return *(uint64_t*)(a + 4) == c2 &&
                    *(uint64_t*)(a + 12) == c3;
        };
        uint32_t sz = SongDetailsContainer::songs->size();
		uint32_t searchNeedle = (uint32_t)(sz * c1 * hashLookupDivisorInverse);

		for(uint32_t i = searchNeedle; i < sz; i++) {
			uint32_t songIndex = SongDetailsContainer::hashBytesLUT->operator[](i);
			uint8_t* hBytes = SongDetailsContainer::hashBytes->data() + (songIndex * SongDetailsContainer::HASH_SIZE_BYTES);
            uint32_t a = *(uint32_t*)hBytes;

            if (a > c1)
                break;
            else if (a != c1)
                continue;
            if (comp(hBytes)) {
                out = &SongDetailsContainer::songs->operator[](songIndex);
                return true;
            }

        }

		for(uint32_t i = searchNeedle; i >= 0; i--) {
			uint32_t songIndex = SongDetailsContainer::hashBytesLUT->operator[](i);
			uint8_t* hBytes = SongDetailsContainer::hashBytes->data() + (songIndex * SongDetailsContainer::HASH_SIZE_BYTES);
            uint32_t a = *(uint32_t*)hBytes;

            if(a < c1)
                break;
            else if(a != c1)
                continue;

            if(comp(hBytes)) {
                out = &SongDetailsContainer::songs->operator[](songIndex);
                return true;
            }
        }

        out = &Song::none;
        return true;
    }

    const Song& SongArray::FindByHash(const std::string_view& hash) {
        const Song* res;
        if (FindByHash(hash, res)) return *res;
        else return Song::none;
    }

    bool SongArray::FindByMapId(const std::string& key, const Song*& out) {
        return FindByMapId(std::stoul(key, nullptr, 16), out);
    }

    const Song& SongArray::FindByMapId(const std::string& key) {
        const Song* res;
        if (FindByMapId(key, res)) return *res;
        return Song::none;
    }

    bool SongArray::FindByMapId(uint32_t key, const Song*& out) {
        auto itr = std::find(SongDetailsContainer::keys->begin(), SongDetailsContainer::keys->end(), key);
        if (itr == SongDetailsContainer::keys->end()) {
            out = &Song::none;
            return false;
        }
        out = &SongDetailsContainer::songs->operator[](itr - SongDetailsContainer::keys->begin());
        return true;
    }

    const Song& SongArray::FindByMapId(uint32_t key) {
        const Song* res;
        if (FindByMapId(key, res)) return *res;
        return Song::none;
    }
}