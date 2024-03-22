#include "ddimm_parser.hpp"

#include "constants.hpp"
#include "exceptions.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>

namespace vpd
{

static constexpr auto SDRAM_DENSITY_PER_DIE_24GB = 24;
static constexpr auto SDRAM_DENSITY_PER_DIE_32GB = 32;
static constexpr auto SDRAM_DENSITY_PER_DIE_48GB = 48;
static constexpr auto SDRAM_DENSITY_PER_DIE_64GB = 64;
static constexpr auto SDRAM_DENSITY_PER_DIE_UNDEFINED = 0;

static constexpr auto PRIMARY_BUS_WIDTH_32_BITS = 32;
static constexpr auto PRIMARY_BUS_WIDTH_UNUSED = 0;

bool DdimmVpdParser::checkValidValue(uint8_t i_ByteValue, uint8_t i_shift,
                                     uint8_t i_minValue, uint8_t i_maxValue)
{
    bool l_isValid = true;
    uint8_t l_ByteValue = i_ByteValue >> i_shift;
    if ((l_ByteValue > i_maxValue) || (l_ByteValue < i_minValue))
    {
        logging::logMessage("Non valid Value encountered value[" +
                            std::to_string(l_ByteValue) + "] range [" +
                            std::to_string(i_minValue) + ".." +
                            std::to_string(i_maxValue) + "] found ");
        return false;
    }
    return l_isValid;
}

uint8_t DdimmVpdParser::getDdr5DensityPerDie(uint8_t i_ByteValue)
{
    uint8_t l_densityPerDie = SDRAM_DENSITY_PER_DIE_UNDEFINED;
    if (i_ByteValue < constants::VALUE_5)
    {
        l_densityPerDie = i_ByteValue * constants::VALUE_4;
    }
    else
    {
        switch (i_ByteValue)
        {
            case constants::VALUE_5:
                l_densityPerDie = SDRAM_DENSITY_PER_DIE_24GB;
                break;

            case constants::VALUE_6:
                l_densityPerDie = SDRAM_DENSITY_PER_DIE_32GB;
                break;

            case constants::VALUE_7:
                l_densityPerDie = SDRAM_DENSITY_PER_DIE_48GB;
                break;

            case constants::VALUE_8:
                l_densityPerDie = SDRAM_DENSITY_PER_DIE_64GB;
                break;

            default:
                logging::logMessage(
                    "default value encountered for density per die");
                l_densityPerDie = SDRAM_DENSITY_PER_DIE_UNDEFINED;
                break;
        }
    }
    return l_densityPerDie;
}

uint8_t DdimmVpdParser::getDdr5DiePerPackage(uint8_t i_ByteValue)
{
    uint8_t l_DiePerPackage = constants::VALUE_0;
    if (i_ByteValue < constants::VALUE_2)
    {
        l_DiePerPackage = i_ByteValue + constants::VALUE_1;
    }
    else
    {
        l_DiePerPackage = pow(constants::VALUE_2,
                              (i_ByteValue - constants::VALUE_1));
    }
    return l_DiePerPackage;
}

size_t DdimmVpdParser::getDdr5BasedDdimmSize(
    types::BinaryVector::const_iterator i_iterator)
{
    size_t l_dimmSize = 0;

    do
    {
        if (!checkValidValue(i_iterator[constants::SPD_BYTE_235] &
                                 constants::MASK_BYTE_BITS_01,
                             constants::SHIFT_BITS_0, constants::VALUE_1,
                             constants::VALUE_3) ||
            !checkValidValue(i_iterator[constants::SPD_BYTE_235] &
                                 constants::MASK_BYTE_BITS_345,
                             constants::SHIFT_BITS_3, constants::VALUE_1,
                             constants::VALUE_3))
        {
            logging::logMessage(
                "Capacity calculation failed for channels per DIMM. DDIMM Byte "
                "235 value [" +
                std::to_string(i_iterator[constants::SPD_BYTE_235]) + "]");
            break;
        }
        uint8_t l_channelsPerDdimm = (((i_iterator[constants::SPD_BYTE_235] &
                                        constants::MASK_BYTE_BITS_01)
                                           ? constants::VALUE_1
                                           : constants::VALUE_0) +
                                      ((i_iterator[constants::SPD_BYTE_235] &
                                        constants::MASK_BYTE_BITS_345)
                                           ? constants::VALUE_1
                                           : constants::VALUE_0));

        if (!checkValidValue(i_iterator[constants::SPD_BYTE_235] &
                                 constants::MASK_BYTE_BITS_012,
                             constants::SHIFT_BITS_0, constants::VALUE_1,
                             constants::VALUE_3))
        {
            logging::logMessage(
                "Capacity calculation failed for bus width per channel. DDIMM "
                "Byte 235 value [" +
                std::to_string(i_iterator[constants::SPD_BYTE_235]) + "]");
            break;
        }
        uint8_t l_busWidthPerChannel = (i_iterator[constants::SPD_BYTE_235] &
                                        constants::MASK_BYTE_BITS_012)
                                           ? PRIMARY_BUS_WIDTH_32_BITS
                                           : PRIMARY_BUS_WIDTH_UNUSED;

        if (!checkValidValue(i_iterator[constants::SPD_BYTE_4] &
                                 constants::MASK_BYTE_BITS_567,
                             constants::SHIFT_BITS_5, constants::VALUE_0,
                             constants::VALUE_5))
        {
            logging::logMessage(
                "Capacity calculation failed for die per package. DDIMM Byte 4 "
                "value [" +
                std::to_string(i_iterator[constants::SPD_BYTE_4]) + "]");
            break;
        }
        uint8_t l_diePerPackage =
            getDdr5DiePerPackage((i_iterator[constants::SPD_BYTE_4] &
                                  constants::MASK_BYTE_BITS_567) >>
                                 constants::VALUE_5);

        if (!checkValidValue(i_iterator[constants::SPD_BYTE_4] &
                                 constants::MASK_BYTE_BITS_01234,
                             constants::SHIFT_BITS_0, constants::VALUE_1,
                             constants::VALUE_8))
        {
            logging::logMessage(
                "Capacity calculation failed for SDRAM Density per Die. DDIMM "
                "Byte 4 value [" +
                std::to_string(i_iterator[constants::SPD_BYTE_4]) + "]");
            break;
        }
        uint8_t l_densityPerDie =
            getDdr5DensityPerDie(i_iterator[constants::SPD_BYTE_4] &
                                 constants::MASK_BYTE_BITS_01234);

        uint8_t l_ranksPerChannel = ((i_iterator[constants::SPD_BYTE_234] &
                                      constants::MASK_BYTE_BITS_345) >>
                                     constants::VALUE_3) +
                                    (i_iterator[constants::SPD_BYTE_234] &
                                     constants::MASK_BYTE_BITS_012) +
                                    constants::VALUE_2;

        if (!checkValidValue(i_iterator[constants::SPD_BYTE_6] &
                                 constants::MASK_BYTE_BITS_567,
                             constants::SHIFT_BITS_5, constants::VALUE_0,
                             constants::VALUE_3))
        {
            logging::logMessage(
                "Capacity calculation failed for dram width DDIMM Byte 6 value "
                "[" +
                std::to_string(i_iterator[constants::SPD_BYTE_6]) + "]");
            break;
        }
        uint8_t l_dramWidth = constants::VALUE_4 *
                              (constants::VALUE_1
                               << ((i_iterator[constants::SPD_BYTE_6] &
                                    constants::MASK_BYTE_BITS_567) >>
                                   constants::VALUE_5));

        l_dimmSize = (l_channelsPerDdimm * l_busWidthPerChannel *
                      l_diePerPackage * l_densityPerDie * l_ranksPerChannel) /
                     (8 * l_dramWidth);

    } while (false);

    return constants::CONVERT_GB_TO_KB * l_dimmSize;
}

size_t
    DdimmVpdParser::getDdimmSize(types::BinaryVector::const_iterator i_iterator)
{
    size_t l_dimmSize = 0;
    if ((i_iterator[constants::SPD_BYTE_2] & constants::SPD_BYTE_MASK) ==
        constants::SPD_DRAM_TYPE_DDR5)
    {
        l_dimmSize = getDdr5BasedDdimmSize(i_iterator);
    }
    else
    {
        logging::logMessage("Error: DDIMM is not DDR5. DDIMM Byte 2 value [" +
                            std::to_string(i_iterator[constants::SPD_BYTE_2]) +
                            "]");
    }
    return l_dimmSize;
}

void DdimmVpdParser::readKeywords(
    types::BinaryVector::const_iterator i_iterator)
{
    // collect DDIMM size value
    auto l_dimmSize = getDdimmSize(i_iterator);
    if (!l_dimmSize)
    {
        throw(DataException("Error: Calculated dimm size is 0."));
    }

    m_parsedVpdMap.emplace("MemorySizeInKB", l_dimmSize);
    // point the i_iterator to DIMM data and skip "11S"
    advance(i_iterator, constants::DDIMM_11S_BARCODE_START +
                            constants::DDIMM_11S_FORMAT_LEN);
    types::BinaryVector l_partNumber(i_iterator,
                                     i_iterator + constants::PART_NUM_LEN);

    advance(i_iterator, constants::PART_NUM_LEN);
    types::BinaryVector l_serialNumber(i_iterator,
                                       i_iterator + constants::SERIAL_NUM_LEN);

    advance(i_iterator, constants::SERIAL_NUM_LEN);
    types::BinaryVector l_ccin(i_iterator, i_iterator + constants::CCIN_LEN);

    m_parsedVpdMap.emplace("FN", l_partNumber);
    m_parsedVpdMap.emplace("PN", move(l_partNumber));
    m_parsedVpdMap.emplace("SN", move(l_serialNumber));
    m_parsedVpdMap.emplace("CC", move(l_ccin));
}

types::VPDMapVariant DdimmVpdParser::parse()
{
    try
    {
        // Read the data and return the map
        auto l_iterator = m_vpdVector.cbegin();
        readKeywords(l_iterator);
        return m_parsedVpdMap;
    }
    catch (const std::exception& exp)
    {
        logging::logMessage(exp.what());
        throw exp;
    }
}

} // namespace vpd
