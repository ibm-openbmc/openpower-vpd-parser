#pragma once

#include <sdbusplus/message/types.hpp>

#include <cstdint>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

namespace vpd
{
namespace types
{
using BinaryVector = std::vector<uint8_t>;

using KwSize = uint8_t;
using RecordId = uint8_t;
using RecordSize = uint16_t;
using RecordType = uint16_t;
using RecordOffset = uint16_t;
using RecordLength = uint16_t;
using ECCOffset = uint16_t;
using ECCLength = uint16_t;
using PoundKwSize = uint16_t;

using Record = std::string;
using Keyword = std::string;

using KwSize = uint8_t;
using PoundKwSize = uint16_t;

using IpzData = std::tuple<Record, Keyword, BinaryVector>;
using KwData = std::tuple<Keyword, BinaryVector>;
using VpdData = std::variant<IpzData, KwData>;

using IpzType = std::tuple<Record, Keyword>;

using WriteVpdParams = std::variant<IpzData, KwData>;
using ReadVpdParams = std::variant<IpzType, Keyword>;

/* A type for holding the innermost map of property::value.*/
using IPZKwdValueMap = std::unordered_map<std::string, std::string>;
/*IPZ VPD Map of format <Record name, <keyword, value>>*/
using IPZVpdMap = std::unordered_map<std::string, IPZKwdValueMap>;

/*Value types supported by Keyword VPD*/
using KWdVPDValueType = std::variant<BinaryVector, std::string, size_t>;
/* This hold map of parsed data of keyword VPD type*/
using KeywordVpdMap = std::unordered_map<std::string, KWdVPDValueType>;

using RecordOffsetList = std::vector<uint32_t>;

using VPDMapVariant = std::variant<std::monostate, IPZVpdMap, KeywordVpdMap>;

using RecordData = std::tuple<RecordOffset, RecordLength, ECCOffset, ECCLength>;

// This covers mostly all the data type supported over DBus for a property.
// clang-format off
using DbusVariantType = std::variant<
    std::vector<std::tuple<std::string, std::string, std::string>>,
    std::vector<std::string>,
    std::vector<double>,
    std::string,
    int64_t,
    uint64_t,
    double,
    int32_t,
    uint32_t,
    int16_t,
    uint16_t,
    uint8_t,
    bool,
    BinaryVector,
    std::vector<uint32_t>,
    std::vector<uint16_t>,
    sdbusplus::message::object_path,
    std::tuple<uint64_t, std::vector<std::tuple<std::string, std::string, double, uint64_t>>>,
    std::vector<std::tuple<std::string, std::string>>,
    std::vector<std::tuple<uint32_t, std::vector<uint32_t>>>,
    std::vector<std::tuple<uint32_t, size_t>>,
    std::vector<std::tuple<sdbusplus::message::object_path, std::string,
                           std::string, std::string>>
 >;
} // namespace types
} // namespace vpd
