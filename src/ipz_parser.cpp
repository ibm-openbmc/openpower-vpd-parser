#include "config.h"

#include "ipz_parser.hpp"

#include "constants.hpp"
#include "exceptions.hpp"
#include "utils.hpp"

#include <nlohmann/json.hpp>
#include <typeindex>

#include "vpdecc/vpdecc.h"

namespace vpd
{

/**
 * @brief Encoding scheme of a VPD keyword's data
 */
enum KwdEncoding
{
    ASCII, /**< data encoded in ascii */
    RAW,   /**< raw data */
    // Keywords needing custom decoding
    MAC,  /**< The keyword B1 needs to be decoded specially */
    DATE, /**< Special decoding of MB meant for Build Date */
    UUID  /**< Special decoding of UD meant for UUID */
};

/** @brief OpenPOWER VPD keywords we're interested in */
enum OpenPowerKeyword
{
    DR, /**< FRU name/description */
    PN, /**< FRU part number */
    SN, /**< FRU serial number */
    CC, /**< Customer Card Identification Number (CCIN) */
    HW, /**< FRU version */
    B1, /**< MAC Address */
    VN, /**< FRU manufacturer name */
    MB, /**< FRU manufacture date */
    MM, /**< FRU model */
    UD, /**< System UUID */
    VS, /**< OpenPower serial number */
    VP  /**< OpenPower part number */
};

static const std::unordered_map<std::string, KwdEncoding> supportedKeywords = {
    {"DR", KwdEncoding::ASCII}, {"PN", KwdEncoding::ASCII},
    {"SN", KwdEncoding::ASCII}, {"CC", KwdEncoding::ASCII},
    {"HW", KwdEncoding::RAW},   {"B1", KwdEncoding::MAC},
    {"VN", KwdEncoding::ASCII}, {"MB", KwdEncoding::DATE},
    {"MM", KwdEncoding::ASCII}, {"UD", KwdEncoding::UUID},
    {"VP", KwdEncoding::ASCII}, {"VS", KwdEncoding::ASCII},
};

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
    VHDR_RECORD_LENGTH = 44
}; // enum Length

static constexpr auto toHex(size_t aChar)
{
    constexpr auto map = "0123456789abcdef";
    return map[aChar];
}

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

#ifdef ECC_CHECK
bool IpzVpdParser::vhdrEccCheck()
{
    auto vpdPtr = m_vpdVector.cbegin();

    auto l_status =
        vpdecc_check_data(const_cast<uint8_t*>(&vpdPtr[Offset::VHDR_RECORD]),
                          Length::VHDR_RECORD_LENGTH,
                          const_cast<uint8_t*>(&vpdPtr[Offset::VHDR_ECC]),
                          Length::VHDR_ECC_LENGTH);
    if (l_status == VPD_ECC_CORRECTABLE_DATA)
    {
        try
        {
            if (m_vpdFileStream.is_open())
            {
                m_vpdFileStream.seekp(m_vpdStartOffset + Offset::VHDR_RECORD,
                                      std::ios::beg);
                m_vpdFileStream.write(reinterpret_cast<const char*>(
                                          &m_vpdVector[Offset::VHDR_RECORD]),
                                      Length::VHDR_RECORD_LENGTH);
            }
            else
            {
                logging::logMessage("File not open");
                return false;
            }
        }
        catch (const std::fstream::failure& e)
        {
            logging::logMessage(
                "Error while operating on file with exception: " +
                std::string(e.what()));
            return false;
        }
    }
    else if (l_status != VPD_ECC_OK)
    {
        return false;
    }

    return true;
}

bool IpzVpdParser::vtocEccCheck()
{
    auto vpdPtr = m_vpdVector.cbegin();

    std::advance(vpdPtr, Offset::VTOC_PTR);

    // The offset to VTOC could be 1 or 2 bytes long
    auto vtocOffset = readUInt16LE(vpdPtr);

    // Get the VTOC Length
    std::advance(vpdPtr, Offset::VTOC_PTR + sizeof(types::RecordOffset));
    auto vtocLength = readUInt16LE(vpdPtr);

    // Get the ECC Offset
    std::advance(vpdPtr, sizeof(types::RecordLength));
    auto vtocECCOffset = readUInt16LE(vpdPtr);

    // Get the ECC length
    std::advance(vpdPtr, sizeof(types::ECCOffset));
    auto vtocECCLength = readUInt16LE(vpdPtr);

    // Reset pointer to start of the vpd,
    // so that Offset will point to correct address
    vpdPtr = m_vpdVector.cbegin();
    auto l_status = vpdecc_check_data(
        const_cast<uint8_t*>(&m_vpdVector[vtocOffset]), vtocLength,
        const_cast<uint8_t*>(&m_vpdVector[vtocECCOffset]), vtocECCLength);
    if (l_status == VPD_ECC_CORRECTABLE_DATA)
    {
        try
        {
            if (m_vpdFileStream.is_open())
            {
                m_vpdFileStream.seekp(m_vpdStartOffset + vtocOffset,
                                      std::ios::beg);
                m_vpdFileStream.write(
                    reinterpret_cast<const char*>(&m_vpdVector[vtocOffset]),
                    vtocLength);
            }
            else
            {
                logging::logMessage("File not open");
                return false;
            }
        }
        catch (const std::fstream::failure& e)
        {
            logging::logMessage(
                "Error while operating on file with exception " +
                std::string(e.what()));
            return false;
        }
    }
    else if (l_status != VPD_ECC_OK)
    {
        return false;
    }

    return true;
}

bool IpzVpdParser::recordEccCheck(types::BinaryVector::const_iterator iterator)
{
    auto recordOffset = readUInt16LE(iterator);

    std::advance(iterator, sizeof(types::RecordOffset));
    auto recordLength = readUInt16LE(iterator);

    if (recordOffset == 0 || recordLength == 0)
    {
        throw(DataException("Invalid record offset or length"));
    }

    std::advance(iterator, sizeof(types::RecordLength));
    auto eccOffset = readUInt16LE(iterator);

    std::advance(iterator, sizeof(types::ECCOffset));
    auto eccLength = readUInt16LE(iterator);

    if (eccLength == 0 || eccOffset == 0)
    {
        throw(EccException("Invalid ECC length or offset."));
    }

    auto vpdPtr = m_vpdVector.cbegin();

    if (vpdecc_check_data(
            const_cast<uint8_t*>(&vpdPtr[recordOffset]), recordLength,
            const_cast<uint8_t*>(&vpdPtr[eccOffset]), eccLength) == VPD_ECC_OK)
    {
        return true;
    }

    return false;
}
#endif

void IpzVpdParser::checkHeader(types::BinaryVector::const_iterator itrToVPD)
{
    if (m_vpdVector.empty() || (Length::RECORD_MIN > m_vpdVector.size()))
    {
        throw(DataException("Malformed VPD"));
    }

    std::advance(itrToVPD, Offset::VHDR);
    auto stop = std::next(itrToVPD, Length::RECORD_NAME);

    std::string record(itrToVPD, stop);
    if ("VHDR" != record)
    {
        throw(DataException("VHDR record not found"));
    }

#ifdef ECC_CHECK
    if (!vhdrEccCheck())
    {
        throw(EccException("ERROR: VHDR ECC check Failed"));
    }
#endif
}

auto IpzVpdParser::readTOC(types::BinaryVector::const_iterator& itrToVPD)
{
    // The offset to VTOC could be 1 or 2 bytes long
    uint16_t vtocOffset =
        readUInt16LE((itrToVPD + Offset::VTOC_PTR)); // itrToVPD);

    // Got the offset to VTOC, skip past record header and keyword header
    // to get to the record name.
    std::advance(itrToVPD, vtocOffset + sizeof(types::RecordId) +
                               sizeof(types::RecordSize) +
                               // Skip past the RT keyword, which contains
                               // the record name.
                               Length::KW_NAME + sizeof(types::KwSize));

    std::string record(itrToVPD, std::next(itrToVPD, Length::RECORD_NAME));
    if ("VTOC" != record)
    {
        throw(DataException("VTOC record not found"));
    }

#ifdef ECC_CHECK
    if (vtocEccCheck())
    {
        throw(EccException("ERROR: VTOC ECC check Failed"));
    }
#endif

    // VTOC record name is good, now read through the TOC, stored in the PT
    // PT keyword; vpdBuffer is now pointing at the first character of the
    // name 'VTOC', jump to PT data.
    // Skip past record name and KW name, 'PT'
    std::advance(itrToVPD, Length::RECORD_NAME + Length::KW_NAME);

    // Note size of PT
    auto ptLen = *itrToVPD;

    // Skip past PT size
    std::advance(itrToVPD, sizeof(types::KwSize));

    // length of PT keyword
    return ptLen;
}

types::RecordOffsetList
    IpzVpdParser::readPT(types::BinaryVector::const_iterator& itrToPT,
                         auto ptLength)
{
    types::RecordOffsetList recordOffsets;

    auto end = itrToPT;
    std::advance(end, ptLength);

    // Look at each entry in the PT keyword. In the entry,
    // we care only about the record offset information.
    while (itrToPT < end)
    {
        // Skip record name and record type
        std::advance(itrToPT, Length::RECORD_NAME + sizeof(types::RecordType));

        // Get record offset
        recordOffsets.push_back(readUInt16LE(itrToPT));

#ifdef ECC_CHECK
        std::string recordName(itrToPT, itrToPT + Length::RECORD_NAME);

        try
        {
            // Verify the ECC for this Record
            if (!recordEccCheck(itrToPT))
            {
                throw(EccException("ERROR: ECC check failed"));
            }
        }
        catch (const EccException& ex)
        {
            logging::logMessage(ex.what());

            /*TODO: uncomment when PEL code goes in */

            /*std::string errMsg =
                std::string{ex.what()} + " Record: " + recordName;

            inventory::PelAdditionalData additionalData{};
            additionalData.emplace("DESCRIPTION", errMsg);
            additionalData.emplace("CALLOUT_INVENTORY_PATH", inventoryPath);
            createPEL(additionalData, PelSeverity::WARNING,
                      errIntfForEccCheckFail, nullptr);*/
        }
        catch (const DataException& ex)
        {
            logging::logMessage(ex.what());

            /*TODO: uncomment when PEL code goes in */

            /*std::string errMsg =
                std::string{ex.what()} + " Record: " + recordName;

            inventory::PelAdditionalData additionalData{};
            additionalData.emplace("DESCRIPTION", errMsg);
            additionalData.emplace("CALLOUT_INVENTORY_PATH", inventoryPath);
            createPEL(additionalData, PelSeverity::WARNING,
                      errIntfForInvalidVPD, nullptr);*/
        }
#endif

        // Jump record size, record length, ECC offset and ECC length
        std::advance(itrToPT,
                     sizeof(types::RecordOffset) + sizeof(types::RecordLength) +
                         sizeof(types::ECCOffset) + sizeof(types::ECCLength));
    }

    return recordOffsets;
}

std::string
    IpzVpdParser::readKwData(std::string_view kwdName,
                             std::size_t kwdDataLength,
                             types::BinaryVector::const_iterator itrToKwdData)
{
    switch (supportedKeywords.find(kwdName.data())->second)
    {
        case KwdEncoding::ASCII:
        {
            return std::string(itrToKwdData,
                               std::next(itrToKwdData, kwdDataLength));
        }

        case KwdEncoding::RAW:
        {
            std::string data(itrToKwdData,
                             std::next(itrToKwdData, kwdDataLength));
            std::string result{};
            std::for_each(data.cbegin(), data.cend(), [&result](size_t aChar) {
                result += toHex(aChar >> 4);
                result += toHex(aChar & 0x0F);
            });
            return result;
        }

        case KwdEncoding::DATE:
        {
            // MB is BuildDate, represent as
            // 1997-01-01-08:30:00
            // <year>-<month>-<day>-<hour>:<min>:<sec>
            std::string data(itrToKwdData,
                             std::next(itrToKwdData, constants::MB_LEN_BYTES));
            std::string result;
            result.reserve(constants::MB_LEN_BYTES);
            auto strItr = data.cbegin();
            std::advance(strItr, 1);
            std::for_each(strItr, data.cend(), [&result](size_t c) {
                result += toHex(c >> 4);
                result += toHex(c & 0x0F);
            });

            result.insert(constants::MB_YEAR_END, 1, '-');
            result.insert(constants::MB_MONTH_END, 1, '-');
            result.insert(constants::MB_DAY_END, 1, '-');
            result.insert(constants::MB_HOUR_END, 1, ':');
            result.insert(constants::MB_MIN_END, 1, ':');

            return result;
        }

        case KwdEncoding::MAC:
        {
            // B1 is MAC address, represent as AA:BB:CC:DD:EE:FF
            std::string data(
                itrToKwdData,
                std::next(itrToKwdData, constants::MAC_ADDRESS_LEN_BYTES));
            std::string result{};
            auto strItr = data.cbegin();
            size_t firstDigit = *strItr;
            result += toHex(firstDigit >> 4);
            result += toHex(firstDigit & 0x0F);
            std::advance(strItr, 1);
            std::for_each(strItr, data.cend(), [&result](size_t c) {
                result += ":";
                result += toHex(c >> 4);
                result += toHex(c & 0x0F);
            });
            return result;
        }

        case KwdEncoding::UUID:
        {
            // UD, the UUID info, represented as
            // 123e4567-e89b-12d3-a456-426655440000
            //<time_low>-<time_mid>-<time hi and version>
            //-<clock_seq_hi_and_res clock_seq_low>-<48 bits node id>
            std::string data(
                itrToKwdData,
                std::next(itrToKwdData, constants::UUID_LEN_BYTES));
            std::string result{};
            std::for_each(data.cbegin(), data.cend(), [&result](size_t c) {
                result += toHex(c >> 4);
                result += toHex(c & 0x0F);
            });
            result.insert(constants::UUID_TIME_LOW_END, 1, '-');
            result.insert(constants::UUID_TIME_MID_END, 1, '-');
            result.insert(constants::UUID_TIME_HIGH_END, 1, '-');
            result.insert(constants::UUID_CLK_SEQ_END, 1, '-');

            return result;
        }
        default:
            break;
    }

    return {};
}

/**
 * @brief API to read keyword and its value under a record.
 *
 * @param[in] iterator - pointer to the start of keywords under the record.
 * @return keyword-value map of keywords under that record.
 */
static types::IPZVpdMap::mapped_type
    readKeywords(types::BinaryVector::const_iterator& itrToKwds)
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

        // Pointing to keyword data now
#ifndef ECC_CHECK
        if (supportedKeywords.end() != supportedKeywords.find(kwdName))
        {
            // Keyword is of interest to us
            std::string kwdValue =
                readKwData(kwdName, kwdDataLength, itrToKwds);
            kwdValueMap.emplace(std::move(kwdName), std::move(kwdValue));
        }
#else
        // support all the Keywords
        auto stop = std::next(itrToKwds, kwdDataLength);
        std::string kwdata(itrToKwds, stop);
        kwdValueMap.emplace(std::move(kwdName), std::move(kwdata));
#endif
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

#ifndef ECC_CHECK
    if (recordName == "VINI" || recordName == "OPFR" || recordName == "OSYS")
    {
#endif
        // If it's a record we're interested in, proceed to find
        // contained keywords and their values.
        std::advance(itrToVPDStart, Length::RECORD_NAME);

#ifdef ECC_CHECK

        // Reverse back to RT Kw, in ipz vpd, to Read RT KW & value
        std::advance(itrToVPDStart, -(Length::KW_NAME + sizeof(types::KwSize) +
                                      Length::RECORD_NAME));
#endif
        // Add entry for this record (and contained keyword:value pairs)
        // to the parsed vpd output.
        m_parsedVPDMap.emplace(std::move(recordName),
                               std::move(readKeywords(itrToVPDStart)));

#ifndef ECC_CHECK
    }
#endif
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
        logging::logMessage(e.what());
        throw e;
    }
}

} // namespace vpd