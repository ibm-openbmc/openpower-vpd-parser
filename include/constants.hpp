#pragma once

#include <cstdint>
#include <iostream>
namespace vpd
{
namespace constants
{
static constexpr uint8_t IPZ_DATA_START = 11;
static constexpr uint8_t IPZ_DATA_START_TAG = 0x84;
static constexpr uint8_t IPZ_RECORD_END_TAG = 0x78;

static constexpr uint8_t KW_VPD_DATA_START = 0;
static constexpr uint8_t KW_VPD_START_TAG = 0x82;
static constexpr uint8_t KW_VPD_PAIR_START_TAG = 0x84;
static constexpr uint8_t ALT_KW_VPD_PAIR_START_TAG = 0x90;
static constexpr uint8_t KW_VPD_END_TAG = 0x78;
static constexpr uint8_t KW_VAL_PAIR_END_TAG = 0x79;

static constexpr auto DDIMM_11S_BARCODE_START = 416;
static constexpr auto DDIMM_11S_BARCODE_START_TAG = "11S";
static constexpr auto DDIMM_11S_FORMAT_LEN = 3;
static constexpr auto DDIMM_11S_BARCODE_LEN = 26;
static constexpr auto PART_NUM_LEN = 7;
static constexpr auto SERIAL_NUM_LEN = 12;
static constexpr auto CCIN_LEN = 4;
static constexpr auto CONVERT_MB_TO_KB = 1024;
static constexpr auto CONVERT_GB_TO_KB = 1024 * 1024;

static constexpr auto SPD_BYTE_2 = 2;
static constexpr auto SPD_BYTE_3 = 3;
static constexpr auto SPD_BYTE_4 = 4;
static constexpr auto SPD_BYTE_6 = 6;
static constexpr auto SPD_BYTE_12 = 12;
static constexpr auto SPD_BYTE_13 = 13;
static constexpr auto SPD_BYTE_18 = 18;
static constexpr auto SPD_BYTE_234 = 234;
static constexpr auto SPD_BYTE_235 = 235;
static constexpr auto SPD_BYTE_BIT_0_3_MASK = 0x0F;
static constexpr auto SPD_BYTE_MASK = 0xFF;
static constexpr auto SPD_MODULE_TYPE_DDIMM = 0x0A;
static constexpr auto SPD_DRAM_TYPE_DDR5 = 0x12;
static constexpr auto SPD_DRAM_TYPE_DDR4 = 0x0C;

static constexpr auto LAST_KW = "PF";
static constexpr auto POUND_KW = '#';
static constexpr auto MB_YEAR_END = 4;
static constexpr auto MB_MONTH_END = 7;
static constexpr auto MB_DAY_END = 10;
static constexpr auto MB_HOUR_END = 13;
static constexpr auto MB_MIN_END = 16;
static constexpr auto MB_RESULT_LEN = 19;
static constexpr auto MB_LEN_BYTES = 8;
static constexpr auto UUID_LEN_BYTES = 16;
static constexpr auto UUID_TIME_LOW_END = 8;
static constexpr auto UUID_TIME_MID_END = 13;
static constexpr auto UUID_TIME_HIGH_END = 18;
static constexpr auto UUID_CLK_SEQ_END = 23;
static constexpr auto MAC_ADDRESS_LEN_BYTES = 6;
static constexpr auto ONE_BYTE = 1;
static constexpr auto TWO_BYTES = 2;

static constexpr auto VALUE_0 = 0;
static constexpr auto VALUE_1 = 1;
static constexpr auto VALUE_2 = 2;
static constexpr auto VALUE_3 = 3;
static constexpr auto VALUE_4 = 4;
static constexpr auto VALUE_5 = 5;
static constexpr auto VALUE_6 = 6;
static constexpr auto VALUE_7 = 7;
static constexpr auto VALUE_8 = 8;

static constexpr auto MASK_BYTE_BITS_01 = 0x03;
static constexpr auto MASK_BYTE_BITS_345 = 0x38;
static constexpr auto MASK_BYTE_BITS_012 = 0x07;
static constexpr auto MASK_BYTE_BITS_567 = 0xE0;
static constexpr auto MASK_BYTE_BITS_01234 = 0x1F;

static constexpr auto SHIFT_BITS_0 = 0;
static constexpr auto SHIFT_BITS_3 = 3;
static constexpr auto SHIFT_BITS_5 = 5;

static constexpr auto ASCII_OF_SPACE = 32;

constexpr auto pimPath = "/xyz/openbmc_project/inventory";
constexpr auto pimIntf = "xyz.openbmc_project.Inventory.Manager";
constexpr auto ipzVpdInf = "com.ibm.ipzvpd.";
constexpr auto kwdVpdInf = "com.ibm.ipzvpd.VINI";
constexpr auto vsysInf = "com.ibm.ipzvpd.VSYS";
constexpr auto kwdCCIN = "CC";
constexpr auto kwdRG = "RG";
constexpr auto locationCodeInf = "com.ibm.ipzvpd.Location";
constexpr auto pldmServiceName = "xyz.openbmc_project.PLDM";
constexpr auto pimServiceName = "xyz.openbmc_project.Inventory.Manager";
constexpr auto biosConfigMgrObjPath =
    "/xyz/openbmc_project/bios_config/manager";
constexpr auto biosConfigMgrService = "xyz.openbmc_project.BIOSConfig.Manager";
constexpr auto biosConfigMgrInterface =
    "xyz.openbmc_project.BIOSConfig.Manager";
constexpr auto systemVpdInvPath =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard";

static constexpr auto BD_YEAR_END = 4;
static constexpr auto BD_MONTH_END = 7;
static constexpr auto BD_DAY_END = 10;
static constexpr auto BD_HOUR_END = 13;

constexpr uint8_t UNEXP_LOCATION_CODE_MIN_LENGTH = 4;
constexpr uint8_t EXP_LOCATION_CODE_MIN_LENGTH = 17;
static constexpr auto SE_KWD_LENGTH = 7;
static constexpr auto INVALID_NODE_NUMBER = -1;

static constexpr auto CMD_BUFFER_LENGTH = 256;

// To be explicitly used for string comparision.
static constexpr auto STR_CMP_SUCCESS = 0;
} // namespace constants
} // namespace vpd
