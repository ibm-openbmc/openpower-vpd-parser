#include "keyword_vpd_parser.hpp"

#include "constants.hpp"
#include "exceptions.hpp"

#include <iostream>
#include <numeric>
#include <string>

namespace vpd
{

/* @brief Keyword VPD parser function*/
std::variant<types::ParsedVPD, types::KeywordVpdMap> KeywordVpdParser::parse()
{
    m_vpdIterator = m_keywordVpdVector.begin();

    checkNextBytesValidity(sizeof(constants::KW_VPD_START_TAG));
    std::advance(m_vpdIterator, sizeof(constants::KW_VPD_START_TAG));

    size_t dataSize = getKwDataSize();

    checkNextBytesValidity(constants::TWO_BYTES + dataSize);

    // +TWO_BYTES is the size byte
    std::advance(m_vpdIterator, constants::TWO_BYTES + dataSize);

    // Check for invalid vendor defined large resource type
    if (*m_vpdIterator != constants::KW_VPD_START_TAG)
    {
        if (*m_vpdIterator != constants::ALT_KW_VPD_START_TAG)
        {
            throw(DataException("Invalid Keyword Vpd Start Tag"));
        }
    }
    auto kwValMap = parser();

    // Do these validations before returning parsed data.
    // Check for small resource type end tag
    if (*m_vpdIterator != constants::KW_VAL_PAIR_END_TAG)
    {
        throw(DataException("Invalid Small resource type End"));
    }
    validateChecksum();
    // Check vpd end Tag.
    if (*m_vpdIterator != constants::KW_VPD_END_TAG)
    {
        throw(DataException("Invalid Small resource type."));
    }

    return kwValMap;
}

types::KeywordVpdMap KeywordVpdParser::parser()
{
    int totalSize = getKwDataSize();
    if (totalSize == 0)
    {
        throw(DataException("Data size is 0, badly formed keyword VPD"));
    }
    types::KeywordVpdMap kwValMap;
    m_checkSumStart = m_vpdIterator;

    checkNextBytesValidity(constants::TWO_BYTES + 1);
    std::advance(m_vpdIterator, constants::TWO_BYTES + 1);

    // Parse the keyword-value and store the pairs in map
    while (totalSize > 0)
    {
        checkNextBytesValidity(constants::TWO_BYTES);
        std::string keywordName(m_vpdIterator,
                                m_vpdIterator + constants::TWO_BYTES);
        std::advance(m_vpdIterator, constants::TWO_BYTES);

        size_t kwSize = *m_vpdIterator;
        checkNextBytesValidity(1);
        m_vpdIterator++;

        checkNextBytesValidity(kwSize);
        std::vector<uint8_t> valueBytes(m_vpdIterator, m_vpdIterator + kwSize);
        std::advance(m_vpdIterator, kwSize);

        totalSize -= constants::TWO_BYTES + kwSize + 1;

        kwValMap.emplace(
            std::make_pair(std::move(keywordName), std::move(valueBytes)));
    }

    m_checkSumEnd = m_vpdIterator;

    return kwValMap;
}

void KeywordVpdParser::validateChecksum()
{
    uint8_t checkSum = 0;

    // Checksum calculation
    checkSum = std::accumulate(m_checkSumStart, m_checkSumEnd, checkSum);
    checkSum = ~checkSum + 1;

    if (checkSum != *(m_vpdIterator + 1))
    {
        throw(DataException("Invalid Checksum"));
    }

    checkNextBytesValidity(constants::TWO_BYTES);
    std::advance(m_vpdIterator, constants::TWO_BYTES);
}

void KeywordVpdParser::checkNextBytesValidity(uint8_t numberOfBytes)
{

    if ((std::distance(m_keywordVpdVector.begin(),
                       m_vpdIterator + numberOfBytes)) >
        std::distance(m_keywordVpdVector.begin(), m_keywordVpdVector.end()))
    {
        throw(DataException("Truncated VPD data"));
    }
}

} // namespace vpd
