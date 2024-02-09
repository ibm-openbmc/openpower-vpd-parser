#include "editor.hpp"

#include "vpdecc/vpdecc.h"

#include "exceptions.hpp"
#include "logger.hpp"
#include "utils.hpp"

namespace vpd
{
using namespace types;
using namespace constants;

types::LE2ByteData editor::get2BytesValue(BinaryVector::const_iterator iterator)
{
    types::LE2ByteData lowByte = *iterator;
    types::LE2ByteData highByte = *(iterator + 1);
    lowByte |= (highByte << 8);

    return lowByte;
}

static void enableRebootGuard()
{
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method = bus.new_method_call(
            "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
            "org.freedesktop.systemd1.Manager", "StartUnit");
        method.append("reboot-guard-enable.service", "replace");
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::string errMsg =
            "Bus call to enable BMC reboot failed for reason: ";
        errMsg += e.what();

        throw std::runtime_error(errMsg);
    }
}

static void disableRebootGuard()
{
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method = bus.new_method_call(
            "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
            "org.freedesktop.systemd1.Manager", "StartUnit");
        method.append("reboot-guard-disable.service", "replace");
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::string errMsg =
            "Bus call to disable BMC reboot failed for reason: ";
        errMsg += e.what();

        throw std::runtime_error(errMsg);
    }
}

bool editor::eccCheckForVHDR()
{
    auto vpdPtr = m_vpdBytes.cbegin();

    auto l_status =
        vpdecc_check_data(const_cast<uint8_t*>(&vpdPtr[Offset::VHDR_RECORD]),
                          Length::VHDR_RECORD_LENGTH,
                          const_cast<uint8_t*>(&vpdPtr[Offset::VHDR_ECC]),
                          Length::VHDR_ECC_LENGTH);
    if (l_status == VPD_ECC_CORRECTABLE_DATA)
    {
        try
        {
            if (!m_vpdFileStream.is_open())
            {
                logging::logMessage("File not open");
                return false;
            }
            m_vpdFileStream.seekp(m_vpdStartOffset + Offset::VHDR_RECORD,
                                  std::ios::beg);
            m_vpdFileStream.write(
                reinterpret_cast<const char*>(&m_vpdBytes[Offset::VHDR_RECORD]),
                Length::VHDR_RECORD_LENGTH);
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

void editor::validateHeader()
{
    auto vpdItr = m_vpdBytes.cbegin();
    std::advance(vpdItr, Offset::VHDR);
    std::string recordName(vpdItr, vpdItr + Length::RECORD_NAME);
    if (recordName.compare("VHDR"))
        throw DataException("VHDR record not found");

    // Check ECC
    if (!eccCheckForVHDR())
        throw EccException("ERROR: VHDR ECC check Failed");
}

bool editor::eccCheckForVtoc()
{
    auto vpdItr = m_vpdBytes.cbegin();
    std::advance(vpdItr, Offset::VTOC_PTR);
    auto vtocOffset = get2BytesValue(vpdItr);

    std::advance(vpdItr, sizeof(RecordOffset));
    auto vtocLength = get2BytesValue(vpdItr);

    std::advance(vpdItr, sizeof(RecordLength));
    auto vtocEccOffset = get2BytesValue(vpdItr);

    std::advance(vpdItr, sizeof(EccOffset));
    auto vtocEccLength = get2BytesValue(vpdItr);

    vpdItr = m_vpdBytes.cbegin();
    auto l_status = vpdecc_check_data(
        const_cast<uint8_t*>(&vpdItr[vtocOffset]), vtocLength,
        const_cast<uint8_t*>(&vpdItr[vtocEccOffset]), vtocEccLength);

    if (l_status == VPD_ECC_CORRECTABLE_DATA)
    {
        try
        {
            if (!m_vpdFileStream.is_open())
            {
                logging::logMessage("File not open");
                return false;
            }
            m_vpdFileStream.seekp(m_vpdStartOffset + vtocOffset, std::ios::beg);
            m_vpdFileStream.write(
                reinterpret_cast<const char*>(&m_vpdBytes[vtocOffset]),
                vtocLength);
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

void editor::validateVtoc()
{
    auto vpdItr = m_vpdBytes.cbegin();
    std::advance(vpdItr, Offset::VTOC_PTR);
    auto vtocOffset = get2BytesValue(vpdItr);

    vpdItr = m_vpdBytes.cbegin();
    std::advance(vpdItr, vtocOffset + sizeof(RecordId) + sizeof(RecordSize) +
                             Length::KW_NAME + sizeof(KwSize));
    std::string recordName(vpdItr, vpdItr + Length::RECORD_NAME);

    if (recordName.compare("VTOC"))
        throw DataException("VTOC record not found");

    // Check ECC
    if (!eccCheckForVtoc())
        throw EccException("ERROR: VTOC ECC check Failed");
}

void editor::processVtoc()
{
    auto iterator = m_vpdBytes.cbegin();
    std::advance(iterator, Offset::VTOC_PTR);
    RecordOffset vtocOffset = get2BytesValue(iterator);

    // jump to Length of PT kwd
    iterator = m_vpdBytes.cbegin();
    std::advance(iterator, vtocOffset + sizeof(RecordId) + sizeof(RecordSize) +
                               Length::KW_NAME + sizeof(KwSize) +
                               Length::RECORD_NAME + Length::KW_NAME);
    uint8_t ptLength = *iterator;
    std::advance(iterator, sizeof(KwSize));
    auto end = std::next(iterator, ptLength);

    // Look at each entry in the PT keyword for the record name
    while (iterator < end)
    {
        std::string recordName(iterator, iterator + Length::RECORD_NAME);

        std::advance(iterator, Length::RECORD_NAME + sizeof(RecordType));
        auto recordOffset = get2BytesValue(iterator);

        std::advance(iterator, Length::RECORD_OFFSET);
        auto recordDataLength = get2BytesValue(iterator);

        if (recordOffset == 0 || recordDataLength == 0)
        {
            throw(DataException("Invalid Record length or offset."));
        }

        std::advance(iterator, Length::RECORD_LENGTH);
        auto recordEccOffset = get2BytesValue(iterator);

        std::advance(iterator, Length::RECORD_ECC_OFFSET);
        auto eccLength = get2BytesValue(iterator);

        if (eccLength == 0 || recordEccOffset == 0)
        {
            throw(EccException("Invalid ECC length or offset."));
        }

        if (!recordName.compare(m_thisRecord.recordName))
        {
            m_thisRecord.recordOffset = recordOffset;
            m_thisRecord.recordDataLength = recordDataLength;
            m_thisRecord.recordEccOffset = recordEccOffset;
            m_thisRecord.recordEccLength = eccLength;
        }

        auto vpdItr = m_vpdBytes.cbegin();
        if (vpdecc_check_data(const_cast<uint8_t*>(&vpdItr[recordOffset]),
                              recordDataLength,
                              const_cast<uint8_t*>(&vpdItr[recordEccOffset]),
                              eccLength) != VPD_ECC_OK)
        {
            throw EccException("Ecc check failed for record");
        }

        // Jump to next record
        std::advance(iterator, sizeof(EccLength));
    }

    if (!m_thisRecord.recordOffset || !m_thisRecord.recordDataLength ||
        !m_thisRecord.recordEccOffset || !m_thisRecord.recordEccLength)
    {
        throw DataException("Record Not found");
    }
}

void editor::getTheKwdOffsetAndLength()
{
    constexpr auto skipBeg = sizeof(RecordId) + sizeof(RecordSize) +
                             Length::KW_NAME + sizeof(KwSize) +
                             Length::RECORD_NAME;

    auto iterator = m_vpdBytes.cbegin();
    std::advance(iterator, m_thisRecord.recordOffset + skipBeg);

    auto end = iterator;
    std::advance(end, m_thisRecord.recordDataLength);
    std::size_t dataLength = 0;

    while (iterator < end)
    {
        // Note keyword name
        std::string thisKwName(iterator, iterator + Length::KW_NAME);

        // Check if the Keyword starts with '#'
        char kwNameStart = *iterator;
        std::advance(iterator, Length::KW_NAME);

        // if keyword starts with #
        if (constants::POUND_KW == kwNameStart)
        {
            // Note existing keyword data length
            dataLength = get2BytesValue(iterator);

            // Jump past 2Byte keyword length + data
            std::advance(iterator, sizeof(types::PoundKwSize));
        }
        else
        {
            // Note existing keyword data length
            dataLength = *iterator;

            // Jump past keyword length and data
            std::advance(iterator, sizeof(KwSize));
        }

        if (m_thisRecord.keywordName == thisKwName)
        {
            m_thisRecord.kwdDataOffset = std::distance(m_vpdBytes.cbegin(),
                                                       iterator);
            m_thisRecord.kwdDataLength = dataLength;
            return;
        }

        // jump the data of current kwd to point to next kwd name
        std::advance(iterator, dataLength);
    }

    throw DataException("Keyword not found");
}

void editor::writeNewDataToVpd()
{
    auto dataLength = m_thisRecord.newData.size() <= m_thisRecord.kwdDataLength
                          ? m_thisRecord.newData.size()
                          : m_thisRecord.kwdDataLength;
    auto dataStart = m_thisRecord.newData.cbegin();
    auto dataEnd = dataStart;
    std::advance(dataEnd, dataLength);

    auto iteratoreToWriteToVpd = m_vpdBytes.begin();
    std::advance(iteratoreToWriteToVpd, m_thisRecord.kwdDataOffset);

    std::copy(dataStart, dataEnd, iteratoreToWriteToVpd);

    m_vpdFileStream.seekp(m_vpdStartOffset + m_thisRecord.kwdDataOffset,
                          std::ios::beg);

    dataStart = m_thisRecord.newData.cbegin();
    dataEnd = dataStart;
    std::advance(dataEnd, dataLength);
    std::copy(dataStart, dataEnd,
              std::ostreambuf_iterator<char>(m_vpdFileStream));
}

void editor::updateRecordEcc()
{
    auto itrToRecordData = m_vpdBytes.cbegin();
    std::advance(itrToRecordData, m_thisRecord.recordOffset);

    auto itrToRecordEcc = m_vpdBytes.cbegin();
    std::advance(itrToRecordEcc, m_thisRecord.recordEccOffset);

    auto l_status = vpdecc_create_ecc(const_cast<uint8_t*>(&itrToRecordData[0]),
                                      m_thisRecord.recordDataLength,
                                      const_cast<uint8_t*>(&itrToRecordEcc[0]),
                                      &m_thisRecord.recordEccLength);
    if (l_status != VPD_ECC_OK)
    {
        throw EccException("Ecc update failed");
    }

    auto end = itrToRecordEcc;
    std::advance(end, m_thisRecord.recordEccLength);

    m_vpdFileStream.seekp(m_vpdStartOffset + m_thisRecord.recordEccOffset,
                          std::ios::beg);
    std::copy(itrToRecordEcc, end,
              std::ostreambuf_iterator<char>(m_vpdFileStream));
}

void editor::updateCI(const std::string& objectPath)
{
    types::ObjectMap allObjects;
    for (const auto& [interface, iFaceValue] :
         m_jsonFile["commonInterfaces"].items())
    {
        for (const auto& [property, value] : iFaceValue.items())
        {
            if (value.type() != nlohmann::json::value_t::object)
                continue;
            if ((value.value("recordName", "") == m_thisRecord.recordName) &&
                (value.value("keywordName", "") == m_thisRecord.keywordName))
            {
                types::PropertyMap propMap;
                types::InterfaceMap interfaces;

                std::string kwdData(m_thisRecord.newData.begin(),
                                    m_thisRecord.newData.end());

                propMap.emplace(property, std::move(kwdData));
                interfaces.emplace(interface, std::move(propMap));
                allObjects.emplace(objectPath, std::move(interfaces));
            }
        }
    }
    utils::callPIM(std::move(allObjects));
}

void editor::updateEI(const nlohmann::json& singleEEPROM,
                      const std::string& objPath)
{
    types::ObjectMap allObjects;

    for (const auto& [extraInterface, iFaceValue] :
         singleEEPROM["extraInterfaces"].items())
    {
        if (iFaceValue == NULL)
            continue;

        for (const auto& [property, value] : iFaceValue.items())
        {
            if (value.type() != nlohmann::json::value_t::object)
                continue;

            if ((value.value("recordName", "") == m_thisRecord.recordName) &&
                (value.value("keywordName", "") == m_thisRecord.keywordName))
            {
                types::PropertyMap propMap;
                types::InterfaceMap interfaces;

                std::string kwdData(m_thisRecord.newData.begin(),
                                    m_thisRecord.newData.end());
                utils::encodeKeyword(kwdData, value.value("encoding", ""));

                propMap.emplace(property, std::move(kwdData));
                interfaces.emplace(extraInterface, std::move(propMap));
                allObjects.emplace(objPath, std::move(interfaces));
            }
        }
    }
    utils::callPIM(std::move(allObjects));
}

void editor::updateCache()
{
    if (!m_jsonFile["frus"].contains(m_vpdFilePath))
    {
        std::cerr << m_vpdFilePath << " not present in json file \n";
        return;
    }
    const std::vector<nlohmann::json>& allEEPROMs =
        m_jsonFile["frus"][m_vpdFilePath]
            .get_ref<const nlohmann::json::array_t&>();

    types::ObjectMap allObjects;

    for (const auto& thisEEPROM : allEEPROMs)
    {
        types::PropertyMap properties;
        types::InterfaceMap interfaces;

        // by default inherit property is true
        bool isInherit = true;
        if (thisEEPROM.contains("inherit"))
            isInherit = thisEEPROM["inherit"].get<bool>();

        if (isInherit)
        {
            properties.emplace(
                utils::getDbusNameForThisKw(m_thisRecord.keywordName),
                m_thisRecord.newData);
            interfaces.emplace(
                (std::string{"com.ibm.ipzvpd."} + m_thisRecord.recordName),
                std::move(properties));
            allObjects.emplace((thisEEPROM["inventoryPath"].get<std::string>()),
                               std::move(interfaces));

            updateCI(thisEEPROM["inventoryPath"]
                         .get_ref<const nlohmann::json::string_t&>());
        }
        updateEI(thisEEPROM, thisEEPROM["inventoryPath"]
                                 .get_ref<const nlohmann::json::string_t&>());

        if (thisEEPROM.contains("copyRecords"))
        {
            if (find(thisEEPROM["copyRecords"].begin(),
                     thisEEPROM["copyRecords"].end(),
                     m_thisRecord.recordName) !=
                thisEEPROM["copyRecords"].end())
            {
                properties.emplace(m_thisRecord.keywordName,
                                   m_thisRecord.newData);
                interfaces.emplace(
                    (std::string{"com.ibm.ipzvpd."} + m_thisRecord.recordName),
                    std::move(properties));
                allObjects.emplace(
                    (thisEEPROM["inventoryPath"].get<std::string>()),
                    std::move(interfaces));
            }
        }
    }
    // Notify PIM
    utils::callPIM(std::move(allObjects));
}

void editor::modifyVpd(const bool& isCacheUpdateRequired)
{
    std::cout << "DBG Entered Modify vpd:\n";

    try
    {
        // To avoid any data corruption,restrict the BMC from rebooting when VPD
        // is being written.
        enableRebootGuard();

        // Read VPD offset if applicable.
        if (!m_jsonFile.empty())
        {
            m_vpdStartOffset = utils::getVPDOffset(m_jsonFile, m_vpdFilePath);
        }

        m_vpdFileStream.exceptions(std::ifstream::badbit |
                                   std::ifstream::failbit);

        utils::getVpdDataInVector(m_vpdFileStream, m_vpdFilePath, m_vpdBytes,
                                  m_vpdStartOffset);

        if (!m_vpdBytes.empty())
        {
            // Validate Header part
            validateHeader();
            // Validate VTOC data
            validateVtoc();

            // Process VTOC to validate ECC for Records and get the offset and
            // Length for Record and ECC.
            processVtoc();

            // Check Record's fields are initialised
            // and Find the keyword offset and data length
            getTheKwdOffsetAndLength();

            // Write new data to the VPD
            writeNewDataToVpd();

            // Update the Record's ECC for new data
            updateRecordEcc();

            // Update the cache
            if (isCacheUpdateRequired)
            {
                // update the cache once data has been updated
                updateCache();
            }

            // Once VPD data and Ecc update is done, disable BMC boot guard.
            disableRebootGuard();
            return;
        }
        else
        {
            throw Exception("Invalid cast");
        }
    }
    catch (const Exception& e)
    {
        // Disable reboot guard.
        disableRebootGuard();
        throw Exception(e.what());
    }

    std::cout << "DBG Existing Modify VPd:\n";
}
} // namespace vpd
