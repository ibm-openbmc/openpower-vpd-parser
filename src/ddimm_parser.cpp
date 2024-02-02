#include "ddimm_parser.hpp"
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

bool DdimmVpdParser::checkValidValue(uint8_t l_ByteValue, uint8_t shift,
                                      uint8_t minValue, uint8_t maxValue)
{
    l_ByteValue = l_ByteValue >> shift;
    if ((l_ByteValue > maxValue) || (l_ByteValue < minValue))
    {
        logging::logMessage("Non valid Value encountered value[" + l_ByteValue
                            + "] range [" + minValue + ".." + maxValue + "] found ");
        return false;
    }
    else
    {
        return true;
    }
}

uint8_t DdimmVpdParser::getDDR5DensityPerDie(uint8_t l_ByteValue)
{
    uint8_t l_densityPerDie = SDRAM_DENSITY_PER_DIE_UNDEFINED;
    if (l_ByteValue < constants::VALUE_5)
    {
        l_densityPerDie = l_ByteValue * constants::VALUE_4;
    }
    else
    {
        switch (l_ByteValue)
        {
            case VALUE_5:
                l_densityPerDie = SDRAM_DENSITY_PER_DIE_24GB;
                break;

            case VALUE_6:
                l_densityPerDie = SDRAM_DENSITY_PER_DIE_32GB;
                break;

            case VALUE_7:
                l_densityPerDie = SDRAM_DENSITY_PER_DIE_48GB;
                break;

            case VALUE_8:
                l_densityPerDie = SDRAM_DENSITY_PER_DIE_64GB;
                break;

            default:
                logging::logMessage("default value encountered for density per die");
                l_densityPerDie = SDRAM_DENSITY_PER_DIE_UNDEFINED;
                break;
        }
    }
    return l_densityPerDie;
}

uint8_t DdimmVpdParser::getDDR5DiePerPackage(uint8_t l_ByteValue)
{
    uint8_t l_DiePerPackage = constants::VALUE_0;
    if (l_ByteValue < constants::VALUE_2)
    {
        l_DiePerPackage = l_ByteValue + constants::VALUE_1;
    }
    else
    {
        l_DiePerPackage = pow(constants::VALUE_2,
                              (l_ByteValue - constants::VALUE_1));
    }
    return l_DiePerPackage;
}

auto DdimmVpdParser::getDdr5BasedDDimmSize(BinaryVector::const_iterator iterator)
{
    size_t dimmSize = 0;

    do
    {
        if (!checkValidValue(iterator[constants::SPD_BYTE_235] &
                                 constants::MASK_BYTE_BITS_01,
                             constants::SHIFT_BITS_0, constants::VALUE_1,
                             constants::VALUE_3) ||
            !checkValidValue(iterator[constants::SPD_BYTE_235] &
                                 constants::MASK_BYTE_BITS_345,
                             constants::SHIFT_BITS_3, constants::VALUE_1,
                             constants::VALUE_3))
        {
            logging::logMessage("Capacity calculation failed for channels per DIMM. DDIMM Byte 235 value [" +
                                 iterator[constants::SPD_BYTE_235] + "]");
            break;
        }
        uint8_t l_channelsPerDDimm =
            (((iterator[constants::SPD_BYTE_235] & constants::MASK_BYTE_BITS_01)
                  ? constants::VALUE_1
                  : constants::VALUE_0) +
             ((iterator[constants::SPD_BYTE_235] &
               constants::MASK_BYTE_BITS_345)
                  ? constants::VALUE_1
                  : constants::VALUE_0));

        if (!checkValidValue(iterator[constants::SPD_BYTE_235] &
                                 constants::MASK_BYTE_BITS_012,
                             constants::SHIFT_BITS_0, constants::VALUE_1,
                             constants::VALUE_3))
        {
           logging::logMessage("Capacity calculation failed for bus width per channel. DDIMM Byte 235 value [" +
                                iterator[constants::SPD_BYTE_235] + "]");
            break;
        }
        uint8_t l_busWidthPerChannel =
            (iterator[constants::SPD_BYTE_235] & constants::MASK_BYTE_BITS_012)
                ? PRIMARY_BUS_WIDTH_32_BITS
                : PRIMARY_BUS_WIDTH_UNUSED;

        if (!checkValidValue(iterator[constants::SPD_BYTE_4] &
                                 constants::MASK_BYTE_BITS_567,
                             constants::SHIFT_BITS_5, constants::VALUE_0,
                             constants::VALUE_5))
        {
            logging::logMessage("Capacity calculation failed for die per package. DDIMM Byte 4 value [" +
                                iterator[constants::SPD_BYTE_4] + "]");
            break;
        }
        uint8_t l_diePerPackage = getDDR5DiePerPackage(
            (iterator[constants::SPD_BYTE_4] & constants::MASK_BYTE_BITS_567) >>
            constants::VALUE_5);

        if (!checkValidValue(iterator[constants::SPD_BYTE_4] &
                                 constants::MASK_BYTE_BITS_01234,
                             constants::SHIFT_BITS_0, constants::VALUE_1,
                             constants::VALUE_8))
        {
            logging::logMessage("Capacity calculation failed for SDRAM Density per Die. DDIMM Byte 4 value [" +
                                 iterator[constants::SPD_BYTE_4] + "]");
            break;
        }
        uint8_t l_densityPerDie = getDDR5DensityPerDie(
            iterator[constants::SPD_BYTE_4] & constants::MASK_BYTE_BITS_01234);

        uint8_t l_ranksPerChannel = ((iterator[constants::SPD_BYTE_234] &
                                      constants::MASK_BYTE_BITS_345) >>
                                     constants::VALUE_3) +
                                    (iterator[constants::SPD_BYTE_234] &
                                     constants::MASK_BYTE_BITS_012) +
                                    constants::VALUE_2;

        if (!checkValidValue(iterator[constants::SPD_BYTE_6] &
                                 constants::MASK_BYTE_BITS_567,
                             constants::SHIFT_BITS_5, constants::VALUE_0,
                             constants::VALUE_3))
        {
            logging::logMessage("Capacity calculation failed for dram width DDIMM Byte 6 value [" +
                                 iterator[constants::SPD_BYTE_6] + "]");
            break;
        }
        uint8_t l_dramWidth = VALUE_4 *
                              (VALUE_1 << ((iterator[constants::SPD_BYTE_6] &
                                            constants::MASK_BYTE_BITS_567) >>
                                           constants::VALUE_5));

        dimmSize = (l_channelsPerDDimm * l_busWidthPerChannel *
                    l_diePerPackage * l_densityPerDie * l_ranksPerChannel) /
                   (8 * l_dramWidth);

    } while (false);

    return constants::CONVERT_MB_TO_KB * dimmSize;
}

size_t DdimmVpdParser::getDDimmSize(BinaryVector::const_iterator iterator)
{
    size_t dimmSize = 0;
    if ((iterator[constants::SPD_BYTE_2] & constants::SPD_BYTE_MASK) ==
             constants::SPD_DRAM_TYPE_DDR5)
    {
        dimmSize = getDdr5BasedDDimmSize(iterator);
    }
    else
    {
        logging::logMessage("Error: DDIMM is neither DDR4 nor DDR5. DDIMM Byte 2 value [" +
                            iterator[constants::SPD_BYTE_2] + "]");
    }
    return dimmSize;
}

types::DdimmVpdMap DdimmVpdParser::readKeywords(BinaryVector::const_iterator iterator)
{
    types::DdimmVpdMap kwdValueMap{};

    // collect Dimm size value
    auto dimmSize = getDDimmSize(iterator);
    if (!dimmSize)
    {
        throw(DataException("Error: Calculated dimm size is 0."));
    }

    kwdValueMap.emplace("MemorySizeInKB", dimmSize);
    // point the iterator to DIMM data and skip "11S"
    advance(iterator, MEMORY_VPD_DATA_START + 3);
    BinaryVector partNumber(iterator, iterator + PART_NUM_LEN);

    advance(iterator, PART_NUM_LEN);
    BinaryVector serialNumber(iterator, iterator + SERIAL_NUM_LEN);

    advance(iterator, SERIAL_NUM_LEN);
    BinaryVector ccin(iterator, iterator + CCIN_LEN);

    kwdValueMap.emplace("FN", partNumber);
    kwdValueMap.emplace("PN", move(partNumber));
    kwdValueMap.emplace("SN", move(serialNumber));
    kwdValueMap.emplace("CC", move(ccin));

    return kwdValueMap;
}

types::VPDMapVariant DdimmVpdParser::parse()
{
    try{
        // Read the data and return the map
        auto iterator = m_vpdVector.cbegin();
        auto vpdDataMap = readKeywords(iterator);

        return vpdDataMap;
    }
    catch (const std::exception& e)
    {
        logging::logMessage(e.what());
        throw e;
    }
}

} // namespace vpd
