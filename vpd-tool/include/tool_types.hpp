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

//IpzType contains tuple of <Record, Keyword>
using IpzType = std::tuple<std::string, std::string>;

//ReadVpdParams either of IPZ or keyword format
using ReadVpdParams = std::variant<IpzType, std::string>;

//KwData contains tuple of <keywordName, KeywordValue>
using KwData = std::tuple<std::string, BinaryVector>;

//IpzData contains tuple of <RecordName, KeywordName, KeywordValue>
using IpzData = std::tuple<std::string, std::string, BinaryVector>;

//WriteVpdParams either of IPZ or keyword format
using WriteVpdParams = std::variant<IpzData, KwData>;
using SystemKeywordInfo =
    std::tuple<std::string, BinaryVector,
               bool, std::string, std::string>;

// Map of system backplane records to list of keywords and its related data. {
//  Record : { Keyword, Default value, Is MFG
//  reset required, Backup Record, Backup Keyword }}
using SystemKeywordsMap =
    std::unordered_map<std::string, std::vector<SystemKeywordInfo>>;

} // namespace types
} // namespace vpd
