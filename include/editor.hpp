#pragma once

#include "constants.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <fstream>

namespace vpd
{
/* @brief Class to modify the VPD.
 *
 * It writes the new data to asked Record-Keyword, on hardware file path
 * and on cache if required.
 *
 */
class editor
{
  public:
    editor() = delete;
    editor(const editor&) = delete;
    editor& operator=(const editor&) = delete;
    editor(editor&&) = delete;
    editor& operator=(editor&&) = delete;
    ~editor() {}

    /* @brief Construct editor class.
     *
     * param[in] path - vpd file path
     * param[in] inventoryPath - objectPath
     * param[in] data - new data to write
     * param[in] keyword - keyword's name whose data going to update
     * param[in] record - record's name containing this keyword
     */

    editor(const std::string& path,
           const sdbusplus::message::object_path& inventoryPath,
           const nlohmann::json& json, const types::BinaryVector& data,
           const std::string& keyword, const std::string& record) :
        m_vpdFilePath(path),
        m_objPath(inventoryPath), m_jsonFile(json),
        m_thisRecord(data, keyword, record){};

    /* @brief API to modify VPD data.
     *
     * This API modifies the VPD's requested record-keyword with new data
     * with help of helper functions.
     *
     * @param[in] isCacheUpdateRequired
     */
    void modifyVpd(const bool& isCacheUpdateRequired);

  private:
    /* @brief Get the 2 bytes data.
     *
     * @returns 2 Bytes value read.
     */
    types::LE2ByteData
        get2BytesValue(types::BinaryVector::const_iterator iterator);

    /* @brief ECC check for VHDR.
     *
     * It calls helper function to validate the ECC data at ECC offset for VHDR.
     *
     * @returns true if ECC check passes.
     *          false if ECC check fails.
     */
    bool eccCheckForVHDR();

    /* @brief Validates VHDR.
     *
     * It checks for 'VHDR' record name at defined byte to confirm it is
     * VPD's header.
     *
     * @throw DataException - if VHDR record name not found OR ECC check fails.
     */
    void validateHeader();

    /* @brief ECC check for VTOC.
     *
     * It calls helper function to validate the ECC data at ECC offset for VTOC.
     *
     * @returns true if ECC check passes.
     *          false if ECC check fails.
     */
    bool eccCheckForVtoc();

    /* @brief validates VTOC.
     *
     * It checks for 'VTOC' record name at defined byte to confirm it is
     * VPD's TOC.
     *
     * @throw DataException - if VTOC record name not found OR ECC check fails.
     */
    void validateVtoc();

    /* @brief process through VTOC.
     *
     * This API process through the VTOC and checks every Record's ECC
     * Also, it looks for requested RECORD to modify, once found it collects
     * all the required fields.
     * RecordOffset
     * RecordLength
     * RecordEccOffset
     * RecordECCLength
     *
     * @throw DataException- if data offset,length is not valid.
     * @throw EccException - if ECC check fails.
     */
    void processVtoc();

    /* @brief Get the Keyword information.
     *
     * This API goes through the Record and finds out requested Keyword offset
     * and length.
     *
     * @throw DataException - if keyword not found
     */
    void getTheKwdOffsetAndLength();

    /* @brief It writes the new data to the VPD.
     *
     * This API writes the new data to the VPD on HW.
     */
    void writeNewDataToVpd();

    /* @brief Updates the New ECC as per New Data.
     *
     * @throw EccException - if ECC update fails.
     */
    void updateRecordEcc();

    /* @brief Updates the Cache for the reqeusted record-keyword.
     *
     * Cache will be updated only when requested explicitly.
     */
    void updateCache();

    /* @brief Updates EI for the reqeusted record-keyword.
     *
     * If requested keyword is part of ExtraInterfaces, need to update
     * it there also.
     */
    void updateEI(const nlohmann::json& singleEEPROM,
                  const std::string& objPath);

    /* @brief Updates CI for the reqeusted record-keyword.
     *
     * If requested keyword is part of commonInterfaces, need to update
     * it there also.
     */
    void updateCI(const std::string& objectPath);

    // VPD file path on HW
    const std::string m_vpdFilePath;

    // FRU inventory path on d-bus
    const std::string m_objPath{};

    // stream to perform operation on file
    std::fstream m_vpdFileStream;

    // file to store parsed json
    const nlohmann::json m_jsonFile;

    // VPD start offset
    size_t m_vpdStartOffset;

    // VPD in vector of bytes
    types::BinaryVector m_vpdBytes;

    // structure to hold informations about VPD Record-keyword to update
    struct dataModificationInfos
    {
        types::BinaryVector newData;
        const std::string keywordName;
        const std::string recordName;

        types::EccOffset recordEccOffset;
        types::RecordOffset recordOffset;
        types::KwDataOffset kwdDataOffset;

        size_t recordEccLength;
        types::RecordLength recordDataLength;
        types::KwSize kwdDataLength;

        // Initialized constructor
        dataModificationInfos(const types::BinaryVector& dataToWrite,
                              const std::string& keyword,
                              const std::string& record) :
            newData(dataToWrite),
            keywordName(keyword), recordName(record), recordEccOffset(0),
            recordOffset(0), kwdDataOffset(0), recordEccLength(0),
            recordDataLength(0), kwdDataLength(0)
        {}
    } m_thisRecord;
};
} // namespace vpd
