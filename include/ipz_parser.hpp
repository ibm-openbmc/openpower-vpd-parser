#pragma once

#include "logger.hpp"
#include "parser_interface.hpp"
#include "types.hpp"

#include <fstream>
#include <string_view>

namespace vpd
{
/**
 * @brief Concrete class to implement IPZ VPD parsing.
 *
 * The class inherits ParserInterface interface class and overrides the parser
 * functionality to implement parsing logic for IPZ VPD format.
 */
class IpzVpdParser : public ParserInterface
{
  public:
    // Deleted APIs
    IpzVpdParser() = delete;
    IpzVpdParser(const IpzVpdParser&) = delete;
    IpzVpdParser& operator=(const IpzVpdParser&) = delete;
    IpzVpdParser(IpzVpdParser&&) = delete;
    IpzVpdParser& operator=(IpzVpdParser&&) = delete;

    /**
     * @brief Constructor.
     *
     * @param[in] vpdVector - VPD data.
     * @param[in] vpdFilePath - Path to VPD EEPROM.
     * @param[in] vpdStartOffset - Offset from where VPD starts in the file.
     * Defaulted to 0.
     */
    IpzVpdParser(const types::BinaryVector& vpdVector,
                 const std::string& vpdFilePath, size_t vpdStartOffset = 0) :
        m_vpdVector(vpdVector),
        m_vpdFilePath(vpdFilePath), m_vpdStartOffset(vpdStartOffset)
    {
        try
        {
            m_vpdFileStream.open(vpdFilePath, std::ios::in | std::ios::out |
                                                  std::ios::binary);
        }
        catch (const std::fstream::failure& e)
        {
            logging::logMessage(e.what());
        }
    }

    /**
     * @brief Defaul destructor.
     */
    ~IpzVpdParser() = default;

    /**
     * @brief API to parse IPZ VPD file.
     *
     * Note: Caller needs to check validity of the map returned. Throws
     * exception in certain situation, needs to be handled accordingly.
     *
     * @return parsed VPD data.
     */
    virtual types::VPDMapVariant parse() override;

    /**
     * @brief API to check validity of VPD header.
     *
     * Note: The API throws exception in case of any failure or malformed VDP.
     *
     * @param[in] itrToVPD - Iterator to the beginning of VPD file.
     * Don't change the parameter to reference as movement of passsed iterator
     * to an offset is not required after header check.
     */
    void checkHeader(types::BinaryVector::const_iterator itrToVPD);

  private:
    /**
     * @brief Check ECC of VPD header.
     *
     * @return true/false based on check result.
     */
    bool vhdrEccCheck();

    /**
     * @brief Check ECC of VTOC.
     *
     * @return true/false based on check result.
     */
    bool vtocEccCheck();

    /**
     * @brief Check ECC of a record.
     *
     * Note: Throws exception in case of failure. Caller need to handle as
     * required.
     *
     * @param[in] iterator - Iterator to the record.
     * @return success/failre
     */
    bool recordEccCheck(types::BinaryVector::const_iterator iterator);

    /**
     * @brief API to read VTOC record.
     *
     * The API reads VTOC record and returns the length of PT keyword.
     *
     * Note: Throws exception in case of any error. Caller need to handle as
     * required.
     *
     * @param[in] itrToVPD - Iterator to begining of VPD data.
     * @return Length of PT keyword.
     */
    auto readTOC(types::BinaryVector::const_iterator& itrToVPD);

    /**
     * @brief API to read PT record.
     *
     * Note: Throws exception in case ECC check fails.
     *
     * @param[in] itrToPT - Iterator to PT record in VPD vector.
     * @param[in] ptLength - length of the PT record.
     * @return List of record's offset.
     */
    types::RecordOffsetList readPT(types::BinaryVector::const_iterator& itrToPT,
                                   auto ptLength);

    /**
     * @brief API to read keyword data based on its encoding type.
     *
     * @param[in] kwdName - Name of the keyword.
     * @param[in] kwdDataLength - Length of keyword data.
     * @param[in] itrToKwdData - Iterator to start of keyword data.
     * @return keyword data, empty otherwise.
     */
    std::string readKwData(std::string_view kwdName, std::size_t kwdDataLength,
                           types::BinaryVector::const_iterator itrToKwdData);

    /**
     * @brief API to read keyword and its value under a record.
     *
     * @param[in] iterator - pointer to the start of keywords under the record.
     * @return keyword-value map of keywords under that record.
     */
    types::IPZVpdMap::mapped_type
        readKeywords(types::BinaryVector::const_iterator& itrToKwds);

    /**
     * @brief API to process a record.
     *
     * @param[in] recordOffset - Offset of the record in VPD.
     */
    void processRecord(auto recordOffset);

    // Holds VPD data.
    const types::BinaryVector& m_vpdVector;

    // stores parsed VPD data.
    types::IPZVpdMap m_parsedVPDMap{};

    // Holds the VPD file path
    const std::string& m_vpdFilePath;

    // Stream to the VPD file. Required to correct ECC
    std::fstream m_vpdFileStream;

    // VPD start offset. Required for ECC correction.
    size_t m_vpdStartOffset = 0;
};
} // namespace vpd