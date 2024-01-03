#include "parser_factory.hpp"

#include "constants.hpp"
#include "exceptions.hpp"
#include "ipz_parser.hpp"
#include "keyword_vpd_parser.hpp"

namespace vpd
{

/**
 * @brief Type of VPD formats.
 */
enum vpdType
{
    IPZ_VPD,                /**< IPZ VPD type */
    KEYWORD_VPD,            /**< Keyword VPD type */
    DDR4_DDIMM_MEMORY_VPD,  /**< DDR4 DDIMM Memory VPD type */
    DDR5_DDIMM_MEMORY_VPD,  /**< DDR5 DDIMM Memory VPD type */
    DDR4_ISDIMM_MEMORY_VPD, /**< DDR4 ISDIMM Memory VPD type */
    DDR5_ISDIMM_MEMORY_VPD, /**< DDR5 ISDIMM Memory VPD type */
    INVALID_VPD_FORMAT      /**< Invalid VPD type */
};

/**
 * @brief API to get the type of VPD.
 *
 * @param[in] vpdVector - VPD file content
 *
 * @return Type of VPD data, "INVALID_VPD_FORMAT" in case of unknown type.
 */
static vpdType vpdTypeCheck(const types::BinaryVector& vpdVector)
{
    if (vpdVector[constants::IPZ_DATA_START] == constants::IPZ_DATA_START_TAG)
    {
        return vpdType::IPZ_VPD;
    }

    if (vpdVector[constants::KW_VPD_DATA_START] == constants::KW_VPD_START_TAG)
    {
        return vpdType::KEYWORD_VPD;
    }

    if ((vpdVector[constants::SPD_BYTE_2] & constants::SPD_BYTE_MASK) ==
        constants::SPD_DRAM_TYPE_DDR5)
    {
        // ISDIMM Memory VPD format
        return vpdType::DDR5_ISDIMM_MEMORY_VPD;
    }

    if ((vpdVector[constants::SPD_BYTE_2] & constants::SPD_BYTE_MASK) ==
        constants::SPD_DRAM_TYPE_DDR4)
    {
        // ISDIMM Memory VPD format
        return vpdType::DDR4_ISDIMM_MEMORY_VPD;
    }

    // Read first 3 Bytes to check the 11S bar code format
    std::string is11SFormat;
    for (uint8_t i = 0; i < constants::FORMAT_11S_LEN; i++)
    {
        is11SFormat += vpdVector[constants::MEMORY_VPD_DATA_START + i];
    }

    if (((vpdVector[constants::SPD_BYTE_3] &
          constants::SPD_BYTE_BIT_0_3_MASK) ==
         constants::SPD_MODULE_TYPE_DDIMM) &&
        (is11SFormat.compare(constants::MEMORY_VPD_START_TAG) == 0))
    {
        // DDIMM Memory VPD format
        if ((vpdVector[constants::SPD_BYTE_2] & constants::SPD_BYTE_MASK) ==
            constants::SPD_DRAM_TYPE_DDR5)
        {
            return vpdType::DDR5_DDIMM_MEMORY_VPD;
        }

        if ((vpdVector[constants::SPD_BYTE_2] & constants::SPD_BYTE_MASK) ==
            constants::SPD_DRAM_TYPE_DDR4)
        {
            return vpdType::DDR4_DDIMM_MEMORY_VPD;
        }
    }

    return vpdType::INVALID_VPD_FORMAT;
}

std::shared_ptr<ParserInterface>
    ParserFactory::getParser(const types::BinaryVector& vpdVector,
                             const std::string& vpdFilePath,
                             size_t vpdStartOffset)
{
    if (vpdVector.empty())
    {
        throw std::runtime_error("Empty VPD vector passed to parser factory");
    }

    vpdType type = vpdTypeCheck(vpdVector);

    switch (type)
    {
        case vpdType::IPZ_VPD:
        {
            std::cout << "IPZ parser selected for VPD file path " << vpdFilePath
                      << std::endl;
            return std::make_shared<IpzVpdParser>(vpdVector, vpdFilePath,
                                                  vpdStartOffset);
        }

        case vpdType::KEYWORD_VPD:
        {
            std::cout << "KWD vpd parser selected for VPD file path "
                      << vpdFilePath << std::endl;
            return std::make_shared<KeywordVpdParser>(vpdVector);
        }

        case vpdType::DDR4_DDIMM_MEMORY_VPD:
        case vpdType::DDR5_DDIMM_MEMORY_VPD:
        {
            // TODO:
            // return shared pointer to class object.
        }

        case vpdType::DDR4_ISDIMM_MEMORY_VPD:
        case vpdType::DDR5_ISDIMM_MEMORY_VPD:
        {
            // TODO:
            // return shared pointer to class object.
        }

        default:
            throw DataException("Unable to determine VPD format");
    }
}
} // namespace vpd