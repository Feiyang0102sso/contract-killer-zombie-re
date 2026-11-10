// Read-only .big archive access for the tests, see BigArchive.h.
#include "BigArchive.h"
#include <zlib.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <fstream>
#include <set>

namespace ckz::test {
namespace {

constexpr char Signature[4] = { 'F', 'G', 'I', 'B' };
constexpr size_t HeaderWordsOffset = 12;  // after signature, version bytes and the hash name
constexpr uint32_t EmptyBucket = 0xffffffff;
constexpr uint32_t ListFlag = 0x80000000;
constexpr uint32_t CompressedFlag = 1;
constexpr size_t EntryWords = 4;

const uint8_t M3gIdentifier[12] = { 171, 74, 83, 82, 49, 56, 52, 187, 13, 10, 26, 10 };
const uint8_t SwerveM3gIdentifier[12] = { 187, 83, 87, 69, 82, 86, 69, 171, 13, 10, 26, 10 };

uint32_t Word(const std::vector<uint8_t>& bytes, size_t offset)
{
    uint32_t value = 0;
    memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

/** Read `size` bytes at `offset` of the file. */
std::vector<uint8_t> ReadRange(const std::filesystem::path& path, uint32_t offset, uint32_t size)
{
    std::vector<uint8_t> bytes(size);
    std::ifstream file(path, std::ios::binary);
    file.seekg(offset);
    file.read(reinterpret_cast<char*>(bytes.data()), size);
    if (!file) {
        bytes.clear();
    }
    return bytes;
}

/** Sort order of the entries: by data offset. */
bool ComesFirst(const BigEntry& left, const BigEntry& right)
{
    return left.dataOffset < right.dataOffset;
}

} // namespace

bool BigArchive::Open(const std::filesystem::path& path)
{
    m_path = path;
    m_entries.clear();
    const std::vector<uint8_t> header = ReadRange(path, 0, HeaderWordsOffset + 12 * 4);
    if (header.empty() || memcmp(header.data(), Signature, sizeof(Signature)) != 0 || header[4] < 2) {
        fprintf(stderr, "BigArchive: %s is not a version 2 archive\n", path.u8string().c_str());
        return false;
    }
    const uint32_t hashTableOffset = Word(header, HeaderWordsOffset);
    const uint32_t hashTableSize = Word(header, HeaderWordsOffset + 4);
    const uint32_t infoBlockOffset = Word(header, HeaderWordsOffset + 8);
    const uint32_t infoBlockSize = Word(header, HeaderWordsOffset + 12);
    const std::vector<uint8_t> hashTable = ReadRange(path, hashTableOffset, hashTableSize);
    const std::vector<uint8_t> infoBlock = ReadRange(path, infoBlockOffset, infoBlockSize);
    if (hashTable.empty() || infoBlock.empty()) {
        return false;
    }

    std::set<uint32_t> seen;
    for (size_t bucket = 0; bucket + 4 <= hashTable.size(); bucket += 4) {
        const uint32_t entryOffset = Word(hashTable, bucket);
        if (entryOffset == EmptyBucket) {
            continue;
        }
        // A bucket with several names starts with a count word.
        uint32_t count = 1;
        size_t cursor = entryOffset;
        const uint32_t first = Word(infoBlock, entryOffset);
        if (first & ListFlag) {
            count = first & ~ListFlag;
            cursor += 4;
        }
        for (uint32_t index = 0; index < count; index++) {
            BigEntry entry = {};
            entry.dataOffset = Word(infoBlock, cursor + 4);
            entry.size = Word(infoBlock, cursor + 8);
            entry.flags = Word(infoBlock, cursor + 12) & 0xffff;
            cursor += EntryWords * 4;
            // Several names may share one resource.
            if (seen.insert(entry.dataOffset).second) {
                m_entries.push_back(entry);
            }
        }
    }
    std::sort(m_entries.begin(), m_entries.end(), ComesFirst);
    return true;
}

std::vector<uint8_t> BigArchive::Read(const BigEntry& entry) const
{
    if ((entry.flags & CompressedFlag) == 0) {
        return ReadRange(m_path, entry.dataOffset, entry.size);
    }
    const std::vector<uint8_t> sizeWord = ReadRange(m_path, entry.dataOffset, 4);
    if (sizeWord.empty()) {
        return {};
    }
    const uint32_t compressedSize = Word(sizeWord, 0);
    std::vector<uint8_t> compressed = ReadRange(m_path, entry.dataOffset + 4, compressedSize);
    std::vector<uint8_t> bytes(entry.size);
    z_stream stream = {};
    stream.next_in = compressed.data();
    stream.avail_in = static_cast<uInt>(compressed.size());
    stream.next_out = bytes.data();
    stream.avail_out = static_cast<uInt>(bytes.size());
    if (inflateInit(&stream) != Z_OK) {
        return {};
    }
    const int status = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    if (status != Z_STREAM_END || stream.total_out != entry.size) {
        fprintf(stderr, "BigArchive: entry at 0x%08x does not inflate (%d)\n", entry.dataOffset, status);
        return {};
    }
    return bytes;
}

bool IsM3gData(const std::vector<uint8_t>& data)
{
    if (data.size() < sizeof(M3gIdentifier)) {
        return false;
    }
    if (memcmp(data.data(), M3gIdentifier, sizeof(M3gIdentifier)) == 0) {
        return true;
    }
    return memcmp(data.data(), SwerveM3gIdentifier, sizeof(SwerveM3gIdentifier)) == 0;
}

std::string EntryName(const BigEntry& entry)
{
    char name[32];
    snprintf(name, sizeof(name), "m3g_%08x", entry.dataOffset);
    return name;
}

} // namespace ckz::test
