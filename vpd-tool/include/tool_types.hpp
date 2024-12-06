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


using Record = std::string;
using Keyword = std::string;

using KwData = std::tuple<Keyword, BinaryVector>;
using IpzData = std::tuple<Record, Keyword, BinaryVector>;
using WriteVpdParams = std::variant<IpzData, KwData>;

using IpzType = std::tuple<Record, Keyword>;
using ReadVpdParams = std::variant<IpzType, Keyword>;

using KeywordDefaultValue = BinaryVector;
using MfgCleanKeywordMap = std::unordered_map<Keyword,KeywordDefaultValue>;
using MfgCleanRecordMap = std::unordered_map<Record,MfgCleanKeywordMap>;
} // namespace types
} // namespace vpd
