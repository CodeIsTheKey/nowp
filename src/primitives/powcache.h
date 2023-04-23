// Copyright (c) 2022 The Raptoreum developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POWCACHE_H
#define POWCACHE_H

#include <serialize.h>
#include <sync.h>
#include <uint256.h>
#include <util/system.h>
#include <util/unordered_lru_cache.h>

extern RecursiveMutex cs_pow;

/// @brief Maximum size of cache, in elements
static const int64_t DEFAULT_POWCACHE_MAX_ELEMENTS  = 1000000;
/// @brief Save after this many new elements
static const int     DEFAULT_POWCACHE_SAVE_INTERVAL = 720;
/// @brief Validate every PowCache entry before use (for testing)
static bool          DEFAULT_POWCACHE_VALIDATE      = false;

class CPowCache : public unordered_lru_cache<uint256, uint256, std::hash<uint256>>
{
private:
    static CPowCache* instance;
    static const int CURRENT_VERSION = 1;

    int    nVersion;
    size_t nSavedSize;
    int    nSaveInterval;
    bool   bValidate;

public:
    static CPowCache& Instance();

    CPowCache(int64_t maxElements = DEFAULT_POWCACHE_MAX_ELEMENTS, bool validate = DEFAULT_POWCACHE_VALIDATE, int64_t saveInterval = DEFAULT_POWCACHE_SAVE_INTERVAL);
    virtual ~CPowCache();

    void Clear();
    void CheckAndRemove();
    bool IsValidate() const { return bValidate; }
    void Load();
    void Save();
    void DoMaintenance();

    std::string ToString() const;

    template<typename Stream> void Serialize(Stream& s) { this->SerializationOp(s, CSerActionSerialize()); }
    template<typename Stream> void Unserialize(Stream& s) { SerializationOp(s, CSerActionUnserialize()); }

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        LOCK(cs_pow);
        READWRITE(nVersion);

        uint64_t cacheSize = size();
        READWRITE(COMPACTSIZE(cacheSize));

        if (ser_action.ForRead())
        {
            uint256 headerHash;
            uint256 powHash;
            for (int i = 0; i < cacheSize; ++i)
            {
                READWRITE(headerHash);
                READWRITE(powHash);
                insert(headerHash, powHash);
            }
            nVersion = CURRENT_VERSION;
            nSavedSize = size();
        }
        else
        {
            for (auto it = cacheMap.begin(); it != cacheMap.end(); ++it)
            {
                uint256 headerHash = it->first;
                uint256 powHash    = it->second.first;
                READWRITE(headerHash);
                READWRITE(powHash);
            };
            nSavedSize = size();
        }
    }
};

#endif // POWCACHE_H
