#pragma once

#include <iostream>
namespace vpd
{
namespace constants
{
static constexpr uint8_t IPZ_DATA_START_AT = 11;
static constexpr uint8_t IPZ_DATA_START_TAG = 0x84;
static constexpr uint8_t IPZ_RECORD_END_TAG = 0x78;

static constexpr uint8_t KW_VPD_DATA_START_AT = 0;
static constexpr uint8_t KW_VPD_START_TAG = 0x82;
static constexpr uint8_t ALT_KW_VPD_START_TAG = 0x90;
static constexpr uint8_t KW_VPD_END_TAG = 0x78;
static constexpr uint8_t KW_VAL_PAIR_END_TAG = 0x79;

static constexpr auto MEMORY_VPD_DATA_START_AT = 416;
static constexpr auto MEMORY_VPD_START_TAG = "11S";
static constexpr auto FORMAT_11S_LEN = 3;

static constexpr auto SPD_BYTE_2 = 2;
static constexpr auto SPD_BYTE_3 = 3;
static constexpr auto SPD_BYTE_4 = 4;
static constexpr auto SPD_BYTE_6 = 6;
static constexpr auto SPD_BYTE_12 = 12;
static constexpr auto SPD_BYTE_13 = 13;
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

} // namespace constants
} // namespace vpd