#include "config.h"

#include "tool_ipz_parser.hpp"

#include "tool_constants.hpp"

#include <nlohmann/json.hpp>

#include <typeindex>

namespace vpd
{

// Offset of different entries in VPD data.
enum Offset
{
    VHDR = 17,
    VHDR_TOC_ENTRY = 29,
    VTOC_PTR = 35,
    VTOC_REC_LEN = 37,
    VTOC_ECC_OFF = 39,
    VTOC_ECC_LEN = 41,
    VTOC_DATA = 13,
    VHDR_ECC = 0,
    VHDR_RECORD = 11
};

// Length of some specific entries w.r.t VPD data.
enum Length
{
    RECORD_NAME = 4,
    KW_NAME = 2,
    RECORD_OFFSET = 2,
    RECORD_MIN = 44,
    RECORD_LENGTH = 2,
    RECORD_ECC_OFFSET = 2,
    VHDR_ECC_LENGTH = 11,
    VHDR_RECORD_LENGTH = 44,
    RECORD_TYPE = 2,
    SKIP_A_RECORD_IN_PT = 14,
    JUMP_TO_RECORD_NAME = 6
}; // enum Length

/**
 * @brief API to read 2 bytes LE data.
 *
 * @param[in] iterator - iterator to VPD vector.
 * @return read bytes.
 */
static uint16_t readUInt16LE(types::BinaryVector::const_iterator iterator)
{
    uint16_t lowByte = *iterator;
    uint16_t highByte = *(iterator + 1);
    lowByte |= (highByte << 8);
    return lowByte;
}

bool IpzVpdParser::vhdrEccCheck()
{
    // TODO: Implement after including ECC code
    return true;
}

bool IpzVpdParser::vtocEccCheck()
{
    // TODO: Implement after including ECC code
    return true;
}

bool IpzVpdParser::recordEccCheck(types::BinaryVector::const_iterator iterator)
{
    // TODO: Implement after including ECC code
    (void)iterator;
    return true;
}

void IpzVpdParser::checkHeader(types::BinaryVector::const_iterator itrToVPD)
{
    // TODO: Implement after including ECC code
    (void)itrToVPD;
}

auto IpzVpdParser::readTOC(types::BinaryVector::const_iterator& itrToVPD)
{
    // TODO: Implement after including ECC code
    (void)itrToVPD;
    return 1;
}

types::RecordOffsetList
    IpzVpdParser::readPT(types::BinaryVector::const_iterator& itrToPT,
                         auto ptLength)
{
    types::RecordOffsetList recordOffsets;
    // TODO: Implement after including ECC code
    (void)itrToPT;
    (void)ptLength;
    return recordOffsets;
}

types::IPZVpdMap::mapped_type
    IpzVpdParser::readKeywords(types::BinaryVector::const_iterator& itrToKwds)
{
    types::IPZVpdMap::mapped_type kwdValueMap{};
    while (true)
    {
        // Note keyword name
        std::string kwdName(itrToKwds, itrToKwds + Length::KW_NAME);
        if (constants::LAST_KW == kwdName)
        {
            // We're done
            break;
        }
        // Check if the Keyword is '#kw'
        char kwNameStart = *itrToKwds;

        // Jump past keyword name
        std::advance(itrToKwds, Length::KW_NAME);

        std::size_t kwdDataLength;
        std::size_t lengthHighByte;

        if (constants::POUND_KW == kwNameStart)
        {
            // Note keyword data length
            kwdDataLength = *itrToKwds;
            lengthHighByte = *(itrToKwds + 1);
            kwdDataLength |= (lengthHighByte << 8);

            // Jump past 2Byte keyword length
            std::advance(itrToKwds, sizeof(types::PoundKwSize));
        }
        else
        {
            // Note keyword data length
            kwdDataLength = *itrToKwds;

            // Jump past keyword length
            std::advance(itrToKwds, sizeof(types::KwSize));
        }

        // support all the Keywords
        auto stop = std::next(itrToKwds, kwdDataLength);
        std::string kwdata(itrToKwds, stop);
        kwdValueMap.emplace(std::move(kwdName), std::move(kwdata));

        // Jump past keyword data length
        std::advance(itrToKwds, kwdDataLength);
    }

    return kwdValueMap;
}

void IpzVpdParser::processRecord(auto recordOffset)
{
    // Jump to record name
    auto recordNameOffset = recordOffset + sizeof(types::RecordId) +
                            sizeof(types::RecordSize) +
                            // Skip past the RT keyword, which contains
                            // the record name.
                            Length::KW_NAME + sizeof(types::KwSize);

    // Get record name
    auto itrToVPDStart = m_vpdVector.cbegin();
    std::advance(itrToVPDStart, recordNameOffset);

    std::string recordName(itrToVPDStart, itrToVPDStart + Length::RECORD_NAME);

    // proceed to find contained keywords and their values.
    std::advance(itrToVPDStart, Length::RECORD_NAME);

    // Reverse back to RT Kw, in ipz vpd, to Read RT KW & value
    std::advance(itrToVPDStart, -(Length::KW_NAME + sizeof(types::KwSize) +
                                  Length::RECORD_NAME));

    // Add entry for this record (and contained keyword:value pairs)
    // to the parsed vpd output.
    m_parsedVPDMap.emplace(std::move(recordName),
                           std::move(readKeywords(itrToVPDStart)));
}

types::VPDMapVariant IpzVpdParser::parse()
{
    try
    {
        auto itrToVPD = m_vpdVector.cbegin();

        // Check vaidity of VHDR record
        checkHeader(itrToVPD);

        // Read the table of contents
        auto ptLen = readTOC(itrToVPD);

        // Read the table of contents record, to get offsets
        // to other records.
        auto recordOffsets = readPT(itrToVPD, ptLen);
        for (const auto& offset : recordOffsets)
        {
            processRecord(offset);
        }

        return m_parsedVPDMap;
    }
    catch (const std::exception& e)
    {
        // std::cerr(e.what());
        throw e;
    }
}

types::BinaryVector IpzVpdParser::getKeywordValueFromRecord(
    const types::Record& i_recordName, const types::Keyword& i_keywordName,
    const types::RecordOffset& i_recordDataOffset)
{
    auto l_iterator = m_vpdVector.cbegin();

    // Go to the record name in the given record's offset
    std::ranges::advance(l_iterator,
                         i_recordDataOffset + Length::JUMP_TO_RECORD_NAME,
                         m_vpdVector.cend());

    // Check if the record is present in the given record's offset
    if (i_recordName !=
        std::string(l_iterator,
                    std::ranges::next(l_iterator, Length::RECORD_NAME,
                                      m_vpdVector.cend())))
    {
        throw std::runtime_error(
            "Given record is not present in the offset provided");
    }

    std::ranges::advance(l_iterator, Length::RECORD_NAME, m_vpdVector.cend());

    std::string l_kwName = std::string(
        l_iterator,
        std::ranges::next(l_iterator, Length::KW_NAME, m_vpdVector.cend()));

    // Iterate through the keywords until the last keyword PF is found.
    while (l_kwName != constants::LAST_KW)
    {
        // First character required for #D keyword check
        char l_kwNameStart = *l_iterator;

        std::ranges::advance(l_iterator, Length::KW_NAME, m_vpdVector.cend());

        // Get the keyword's data length
        auto l_kwdDataLength = 0;

        if (constants::POUND_KW == l_kwNameStart)
        {
            l_kwdDataLength = readUInt16LE(l_iterator);
            std::ranges::advance(l_iterator, sizeof(types::PoundKwSize),
                                 m_vpdVector.cend());
        }
        else
        {
            l_kwdDataLength = *l_iterator;
            std::ranges::advance(l_iterator, sizeof(types::KwSize),
                                 m_vpdVector.cend());
        }

        if (l_kwName == i_keywordName)
        {
            // Return keyword's value to the caller
            return types::BinaryVector(
                l_iterator, std::ranges::next(l_iterator, l_kwdDataLength,
                                              m_vpdVector.cend()));
        }

        // next keyword search
        std::ranges::advance(l_iterator, l_kwdDataLength, m_vpdVector.cend());

        // next keyword name
        l_kwName = std::string(
            l_iterator,
            std::ranges::next(l_iterator, Length::KW_NAME, m_vpdVector.cend()));
    }

    // Keyword not found
    throw std::runtime_error("Given keyword not found.");
}

types::RecordData IpzVpdParser::getRecordDetailsFromVTOC(
    const types::Record& i_recordName, const types::RecordOffset& i_vtocOffset)
{
    // Get VTOC's PT keyword value.
    const auto l_vtocPTKwValue = getKeywordValueFromRecord("VTOC", "PT",
                                                           i_vtocOffset);

    // Parse through VTOC PT keyword value to find the record which we are
    // interested in.
    auto l_vtocPTItr = l_vtocPTKwValue.cbegin();

    types::RecordData l_recordData;

    while (l_vtocPTItr < l_vtocPTKwValue.cend())
    {
        if (i_recordName ==
            std::string(l_vtocPTItr, l_vtocPTItr + Length::RECORD_NAME))
        {
            // Record found in VTOC PT keyword. Get offset
            std::ranges::advance(l_vtocPTItr,
                                 Length::RECORD_NAME + Length::RECORD_TYPE,
                                 l_vtocPTKwValue.cend());
            const auto l_recordOffset = readUInt16LE(l_vtocPTItr);

            std::ranges::advance(l_vtocPTItr, Length::RECORD_OFFSET,
                                 l_vtocPTKwValue.cend());
            const auto l_recordLength = readUInt16LE(l_vtocPTItr);

            std::ranges::advance(l_vtocPTItr, Length::RECORD_LENGTH,
                                 l_vtocPTKwValue.cend());
            const auto l_eccOffset = readUInt16LE(l_vtocPTItr);

            std::ranges::advance(l_vtocPTItr, Length::RECORD_ECC_OFFSET,
                                 l_vtocPTKwValue.cend());
            const auto l_eccLength = readUInt16LE(l_vtocPTItr);

            l_recordData = std::make_tuple(l_recordOffset, l_recordLength,
                                           l_eccOffset, l_eccLength);
            break;
        }

        std::ranges::advance(l_vtocPTItr, Length::SKIP_A_RECORD_IN_PT,
                             l_vtocPTKwValue.cend());
    }

    return l_recordData;
}

types::DbusVariantType IpzVpdParser::readKeywordFromHardware(
    const types::ReadVpdParams i_paramsToReadData)
{
    // Extract record and keyword from i_paramsToReadData
    types::Record l_record;
    types::Keyword l_keyword;

    if (const types::IpzType* l_ipzData =
            std::get_if<types::IpzType>(&i_paramsToReadData))
    {
        l_record = std::get<0>(*l_ipzData);
        l_keyword = std::get<1>(*l_ipzData);
    }
    else
    {
        // std::cerr(
        // "Input parameter type provided isn't compatible with the given VPD
        // type.");
        // throw types::DbusInvalidArgument();
    }

    // Read keyword's value from vector
    auto l_itrToVPD = m_vpdVector.cbegin();

    if (l_record == "VHDR")
    {
// Disable providing a way to read keywords from VHDR for the time being.
#if 0
        std::ranges::advance(l_itrToVPD, Offset::VHDR_RECORD,
                             m_vpdVector.cend());

        return types::DbusVariantType{getKeywordValueFromRecord(
            l_record, l_keyword, Offset::VHDR_RECORD)};
#endif

        // std::cerr("Read cannot be performed on VHDR record.");
        // throw types::DbusInvalidArgument();
    }

    // Get VTOC offset
    std::ranges::advance(l_itrToVPD, Offset::VTOC_PTR, m_vpdVector.cend());
    auto l_vtocOffset = readUInt16LE(l_itrToVPD);

    if (l_record == "VTOC")
    {
        // Disable providing a way to read keywords from VTOC for the time
        // being.
#if 0
        return types::DbusVariantType{
            getKeywordValueFromRecord(l_record, l_keyword, l_vtocOffset)};
#endif

        // std::cerr("Read cannot be performed on VTOC record.");
        // throw types::DbusInvalidArgument();
    }

    // Get record offset from VTOC's PT keyword value.
    auto l_recordData = getRecordDetailsFromVTOC(l_record, l_vtocOffset);
    const auto l_recordOffset = std::get<0>(l_recordData);

    if (l_recordOffset == 0)
    {
        throw std::runtime_error("Record not found in VTOC PT keyword.");
    }

    // Get the given keyword's value
    return types::DbusVariantType{
        getKeywordValueFromRecord(l_record, l_keyword, l_recordOffset)};
}

void IpzVpdParser::updateRecordECC(const auto& i_recordDataOffset,
                                   const auto& i_recordDataLength,
                                   const auto& i_recordECCOffset,
                                   size_t i_recordECCLength,
                                   types::BinaryVector& io_vpdVector)
{
    // TODO: Implement after including ECC code
    (void)i_recordDataOffset;
    (void)i_recordDataLength;
    (void)i_recordECCOffset;
    (void)i_recordECCLength;
    (void)io_vpdVector;
}

int IpzVpdParser::setKeywordValueInRecord(
    const types::Record& i_recordName, const types::Keyword& i_keywordName,
    const types::BinaryVector& i_keywordData,
    const types::RecordOffset& i_recordDataOffset,
    types::BinaryVector& io_vpdVector)
{
    // TODO: Implement after including ECC code
    (void)i_recordName;
    (void)i_keywordName;
    (void)i_keywordData;
    (void)i_recordDataOffset;
    (void)io_vpdVector;
    return 0;
}

int IpzVpdParser::writeKeywordOnHardware(
    const types::WriteVpdParams i_paramsToWriteData)
{
    int l_sizeWritten = -1;
    (void)i_paramsToWriteData;
    return l_sizeWritten;
}
} // namespace vpd
