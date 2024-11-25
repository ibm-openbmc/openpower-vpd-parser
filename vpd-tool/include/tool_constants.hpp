#pragma once

#include <cstdint>

namespace vpd
{
namespace constants
{
static constexpr auto KEYWORD_SIZE = 2;
static constexpr auto RECORD_SIZE = 4;
static constexpr auto INDENTATION = 4;
static constexpr auto SUCCESS = 0;
static constexpr auto FAILURE = -1;

constexpr auto inventoryManagerService =
    "xyz.openbmc_project.Inventory.Manager";
constexpr auto baseInventoryPath = "/xyz/openbmc_project/inventory";
constexpr auto ipzVpdInfPrefix = "com.ibm.ipzvpd.";

constexpr auto vpdManagerService = "com.ibm.VPD.Manager";
constexpr auto vpdManagerObjectPath = "/com/ibm/VPD/Manager";
constexpr auto vpdManagerInfName = "com.ibm.VPD.Manager";
constexpr auto inventoryItemInf = "xyz.openbmc_project.Inventory.Item";
constexpr auto kwdVpdInf = "com.ibm.ipzvpd.VINI";
constexpr auto locationCodeInf = "com.ibm.ipzvpd.Location";
constexpr auto assetInf = "xyz.openbmc_project.Inventory.Decorator.Asset";
} // namespace constants
} // namespace vpd
