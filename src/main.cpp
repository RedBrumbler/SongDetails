
// make modloader happy
#include "SongDetails.hpp"
#include "logging.hpp"
extern "C" void setup(ModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
}

SongDetailsCache::SongDetails* songdetails;
extern "C" void load() {
    INFO("Loading Song Details cache");
    songdetails = SongDetailsCache::SongDetails::Init().get();
    INFO("Done loading");
}