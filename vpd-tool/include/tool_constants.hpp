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
} // namespace constants
} // namespace vpd
