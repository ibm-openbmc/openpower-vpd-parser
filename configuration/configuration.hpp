#pragma once

#include "types.hpp"

namespace vpd
{
namespace config
{

/**
 * @brief Map of IM to HW version.
 *
 * The map holds HW version corresponding to a given IM value.
 * To add a new system, just update the below map.
 * {IM value, {Default, {HW_version, version}}}
 */
types::SystemTypeMap systemType{
    {"50001001", {"50001001_v2", {{"0001", ""}}}},
    {"50001000", {"50001000_v2", {{"0001", ""}}}},
    {"50001002", {"50001002", {}}},
    {"50003000",
     {"50003000_v2", {{"000A", ""}, {"000B", ""}, {"000C", ""}, {"0014", ""}}}},
    {"50004000", {"50004000", {}}}};

std::unordered_map<std::string, std::string> systemSpecificBackupMap{
    {"50001001", "backup_restore_50001001.json"}};
} // namespace config
} // namespace vpd
