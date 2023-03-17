#pragma once

#include <sdbusplus/asio/property.hpp>

namespace vpd
{
namespace types
{
// This covers mostly all the data type supported over Dbus for a property.
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

 using MapperGetObject =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

using ParsedVPD = std::unordered_map<std::string,
                                  std::unordered_map<std::string, std::string>>;
using BinaryVector = std::vector<uint8_t>;
using kwdVpdValueTypes = std::variant<size_t, BinaryVector, std::string>;
using KeywordVpdMap = std::unordered_map<std::string, kwdVpdValueTypes>;
using VPDMapVariant = std::variant<ParsedVPD, KeywordVpdMap>;

using IMValue = std::string;
using HWValue = std::sring;
using SystemTypeMap = std::unordered_map<IMValue, std::vector<HWValue>>;

} // namespace types
} // namespace vpd