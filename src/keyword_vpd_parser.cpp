#include "keyword_vpd_parser.hpp"

#include "constants.hpp"

#include <iostream>
#include <numeric>
#include <string>

namespace vpd
{

/* @brief Keyword VPD parser function*/
std::variant<types::KeywordVpdMap, types::ParsedVPD> KeywordVpdParser::parse()
{
    vpdIterator = vpdVector.begin();

    checkNextBytesValidity(sizeof(constants::KW_VPD_START_TAG));
    std::advance(vpdIterator, sizeof(constants::KW_VPD_START_TAG));

    size_t dataSize = getKwDataSize();

    checkNextBytesValidity(constants::TWO_BYTES + dataSize);

    // +TWO_BYTES is the size byte
    std::advance(vpdIterator, constants::TWO_BYTES + dataSize);

    // Check for invalid vendor defined large resource type
    if (*vpdIterator != constants::KW_VPD_START_TAG)
    {
        if (*vpdIterator != constants::ALT_KW_VPD_START_TAG)
        {
            throw(DataException("Invalid Keyword Vpd Start Tag"));
        }
    }
    auto kwValMap = parser();

    // Do these validations before returning parsed data.
    // Check for small resource type end tag
    if (*vpdIterator != constants::KW_VAL_PAIR_END_TAG)
    {
        throw(DataException("Invalid Small resource type End"));
    }
    validateChecksum();
    // Check vpd end Tag.
    if (*vpdIterator != constants::KW_VPD_END_TAG)
    {
        throw(DataException("Invalid Small resource type."));
    }

    return kwValMap;
}

KeywordVpdMap KeywordVpdParser::parser()
{
    int totalSize = getKwDataSize();
    if (totalSize == 0)
    {
        throw(DataException("Data size is 0, badly formed keyword VPD"));
    }
    KeywordVpdMap kwValMap;
    checkSumStart = vpdIterator;

    checkNextBytesValidity(constants::TWO_BYTES + 1);
    std::advance(vpdIterator, constants::TWO_BYTES + 1);

    // Parse the keyword-value and store the pairs in map
    while (totalSize > 0)
    {
        checkNextBytesValidity(constants::TWO_BYTES);
        std::string keywordName(vpdIterator,
                                vpdIterator + constants::TWO_BYTES);
        std::advance(vpdIterator, constants::TWO_BYTES);

        size_t kwSize = *vpdIterator;
        checkNextBytesValidity(1);
        vpdIterator++;

        checkNextBytesValidity(kwSize);
        std::vector<uint8_t> valueBytes(vpdIterator, vpdIterator + kwSize);
        std::advance(vpdIterator, kwSize);

        totalSize -= constants::TWO_BYTES + kwSize + 1;

        kwValMap.emplace(
            std::make_pair(std::move(keywordName), std::move(valueBytes)));
    }

    checkSumEnd = vpdIterator;

    return kwValMap;
}

void KeywordVpdParser::validateChecksum()
{
    uint8_t checkSum = 0;

    // Checksum calculation
    checkSum = std::accumulate(checkSumStart, checkSumEnd, checkSum);
    checkSum = ~checkSum + 1;

    if (checkSum != *(vpdIterator + 1))
    {
        throw(DataException("Invalid Checksum"));
    }

    checkNextBytesValidity(constants::TWO_BYTES);
    std::advance(vpdIterator, constants::TWO_BYTES);
}

void KeywordVpdParser::checkNextBytesValidity(uint8_t numberOfBytes)
{

    if ((std::distance(vpdVector.begin(), vpdIterator + numberOfBytes)) >
        std::distance(vpdVector.begin(), vpdVector.end()))
    {
        throw(DataException("Truncated VPD data"));
    }
}

} // namespace vpd
