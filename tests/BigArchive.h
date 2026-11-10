// Read-only access to the resources of a Glu .big archive (version 2) for the tests.
// The game's own CBigFile_v2 is not used: loading it changes the archive path of the running game.
//
// Layout (little endian, offsets from the file start): "FGIB", version, hash name, then offset/size
// pairs of the hash table, info block, mime keys, compression table, strings and data. The hash
// table holds an info block offset per bucket; an entry is four words: name offset, data offset,
// size, (mime index << 16 | flags). Flag 1: the data is a uint32 compressed size and a zlib stream.
#pragma once

#include <stdint.h>
#include <filesystem>
#include <string>
#include <vector>

namespace ckz::test {

/** One resource of the archive. Names are only stored as hashes, so it is known by its data offset. */
struct BigEntry {
    uint32_t dataOffset;
    uint32_t size;  // bytes after decompression
    uint32_t flags;
};

class BigArchive {
public:
    /** Read the directory of `path`; false when the file is missing or not a version 2 archive. */
    bool Open(const std::filesystem::path& path);
    /** Every resource, each once, in data offset order. */
    const std::vector<BigEntry>& Entries() const { return m_entries; }
    /** The resource's bytes (inflated); empty on error. */
    std::vector<uint8_t> Read(const BigEntry& entry) const;

private:
    std::filesystem::path m_path;
    std::vector<BigEntry> m_entries;
};

/** True for an M3G file or a Swerve extended M3G file (by their 12-byte identifiers). */
bool IsM3gData(const std::vector<uint8_t>& data);

/** Short name of an archive entry for test names and messages, e.g. "m3g_0031a2f0". */
std::string EntryName(const BigEntry& entry);

} // namespace ckz::test
