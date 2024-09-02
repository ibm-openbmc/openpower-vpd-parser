#include "config.h"

#include "manager.hpp"

#include "backup_restore.hpp"
#include "constants.hpp"
#include "exceptions.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "parser_factory.hpp"
#include "parser_interface.hpp"
#include "types.hpp"
#include "utility/dbus_utility.hpp"
#include "utility/json_utility.hpp"
#include "utility/vpd_specific_utility.hpp"

#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/message.hpp>

namespace vpd
{
Manager::Manager(
    const std::shared_ptr<boost::asio::io_context>& ioCon,
    const std::shared_ptr<sdbusplus::asio::dbus_interface>& iFace,
    const std::shared_ptr<sdbusplus::asio::connection>& asioConnection) :
    m_ioContext(ioCon),
    m_interface(iFace), m_asioConnection(asioConnection)
{
    try
    {
#ifdef IBM_SYSTEM
        m_worker = std::make_shared<Worker>(INVENTORY_JSON_DEFAULT);

        // Set up minimal things that is needed before bus name is claimed.
        m_worker->performInitialSetup();

        // set async timer to detect if system VPD is published on D-Bus.
        SetTimerToDetectSVPDOnDbus();

        // set async timer to detect if VPD collection is done.
        SetTimerToDetectVpdCollectionStatus();

        // Instantiate GpioMonitor class
        m_gpioMonitor = std::make_shared<GpioMonitor>(
            m_worker->getSysCfgJsonObj(), m_ioContext);

#endif
        // Register methods under com.ibm.VPD.Manager interface
        iFace->register_method(
            "WriteKeyword",
            [this](const types::Path i_vpdPath,
                   const types::WriteVpdParams i_paramsToWriteData) -> int {
            return this->updateKeyword(i_vpdPath, i_paramsToWriteData);
        });

        iFace->register_method(
            "ReadKeyword",
            [this](const types::Path i_fruPath,
                   const types::ReadVpdParams i_paramsToReadData)
                -> types::DbusVariantType {
            return this->readKeyword(i_fruPath, i_paramsToReadData);
        });

        iFace->register_method(
            "CollectFRUVPD",
            [this](const sdbusplus::message::object_path& i_dbusObjPath) {
            this->collectSingleFruVpd(i_dbusObjPath);
        });

        iFace->register_method(
            "deleteFRUVPD",
            [this](const sdbusplus::message::object_path& i_dbusObjPath) {
            this->deleteSingleFruVpd(i_dbusObjPath);
        });

        iFace->register_method(
            "GetExpandedLocationCode",
            [this](const std::string& i_unexpandedLocationCode,
                   uint16_t& i_nodeNumber) -> std::string {
            return this->getExpandedLocationCode(i_unexpandedLocationCode,
                                                 i_nodeNumber);
        });

        iFace->register_method("GetFRUsByExpandedLocationCode",
                               [this](const std::string& i_expandedLocationCode)
                                   -> types::ListOfPaths {
            return this->getFrusByExpandedLocationCode(i_expandedLocationCode);
        });

        iFace->register_method(
            "GetFRUsByUnexpandedLocationCode",
            [this](const std::string& i_unexpandedLocationCode,
                   uint16_t& i_nodeNumber) -> types::ListOfPaths {
            return this->getFrusByUnexpandedLocationCode(
                i_unexpandedLocationCode, i_nodeNumber);
        });

        iFace->register_method(
            "GetHardwarePath",
            [this](const sdbusplus::message::object_path& i_dbusObjPath)
                -> std::string { return this->getHwPath(i_dbusObjPath); });

        iFace->register_method("PerformVPDRecollection",
                               [this]() { this->performVPDRecollection(); });

        // Indicates FRU VPD collection for the system has not started.
        iFace->register_property(
            "CollectionStatus", std::string("NotStarted"),
            sdbusplus::asio::PropertyPermission::readWrite);
    }
    catch (const std::exception& e)
    {
        logging::logMessage("VPD-Manager service failed. " +
                            std::string(e.what()));
        throw;
    }
}

#ifdef IBM_SYSTEM
void Manager::SetTimerToDetectSVPDOnDbus()
{
    static boost::asio::steady_timer timer(*m_ioContext);

    // timer for 2 seconds
    auto asyncCancelled = timer.expires_after(std::chrono::seconds(2));

    (asyncCancelled == 0) ? std::cout << "Timer started" << std::endl
                          : std::cout << "Timer re-started" << std::endl;

    timer.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            throw std::runtime_error(
                "Timer to detect system VPD collection status was aborted");
        }

        if (ec)
        {
            throw std::runtime_error(
                "Timer to detect System VPD collection failed");
        }

        if (m_worker->isSystemVPDOnDBus())
        {
            // cancel the timer
            timer.cancel();

            // Triggering FRU VPD collection. Setting status to "In Progress".
            m_interface->set_property("CollectionStatus",
                                      std::string("InProgress"));
            m_worker->collectFrusFromJson();
        }
    });
}

void Manager::SetTimerToDetectVpdCollectionStatus()
{
    static constexpr auto MAX_RETRY = 5;

    static boost::asio::steady_timer l_timer(*m_ioContext);
    static uint8_t l_timerRetry = 0;

    auto l_asyncCancelled = l_timer.expires_after(std::chrono::seconds(3));

    (l_asyncCancelled == 0)
        ? std::cout << "Collection Timer started" << std::endl
        : std::cout << "Collection Timer re-started" << std::endl;

    l_timer.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            throw std::runtime_error(
                "Timer to detect thread collection status was aborted");
        }

        if (ec)
        {
            throw std::runtime_error(
                "Timer to detect thread collection failed");
        }

        if (m_worker->isAllFruCollectionDone())
        {
            // cancel the timer
            l_timer.cancel();
            m_interface->set_property("CollectionStatus",
                                      std::string("Completed"));

            const nlohmann::json& l_sysCfgJsonObj =
                m_worker->getSysCfgJsonObj();
            if (jsonUtility::isBackupAndRestoreRequired(l_sysCfgJsonObj))
            {
                BackupAndRestore l_backupAndRestoreObj(l_sysCfgJsonObj);
                l_backupAndRestoreObj.backupAndRestore();
            }
        }
        else
        {
            if (l_timerRetry == MAX_RETRY)
            {
                l_timer.cancel();
                logging::logMessage("Taking too long. Some logic needed here?");
            }
            else
            {
                l_timerRetry++;
                logging::logMessage(
                    "Waiting... FRU VPD collection is in progress");
                SetTimerToDetectVpdCollectionStatus();
            }
        }
    });
}
#endif

int Manager::updateKeyword(const types::Path i_vpdPath,
                           const types::WriteVpdParams i_paramsToWriteData)
{
    if (i_vpdPath.empty())
    {
        logging::logMessage("Given VPD path is empty.");
        return -1;
    }

    nlohmann::json l_sysCfgJsonObj{};

    if (m_worker.get() != nullptr)
    {
        l_sysCfgJsonObj = m_worker->getSysCfgJsonObj();
    }

    types::Path l_fruPath = i_vpdPath;
    types::Path l_inventoryObjPath;
    types::Path l_redundantFruPath;

    if (!l_sysCfgJsonObj.empty())
    {
        try
        {
            // Get hardware path from system config JSON.
            const types::Path& l_tempPath =
                jsonUtility::getFruPathFromJson(l_sysCfgJsonObj, i_vpdPath);

            if (!l_tempPath.empty())
            {
                // Save the FRU path to update on hardware
                l_fruPath = l_tempPath;

                // Get inventory object path from system config JSON
                l_inventoryObjPath = jsonUtility::getInventoryObjPathFromJson(
                    l_sysCfgJsonObj, i_vpdPath);

                // Get redundant hardware path if present in system config JSON
                l_redundantFruPath =
                    jsonUtility::getRedundantEepromPathFromJson(l_sysCfgJsonObj,
                                                                i_vpdPath);
            }
        }
        catch (const std::exception& e)
        {
            return -1;
        }
    }

    // Update keyword's value on hardware
    int l_bytesUpdatedOnHardware = updateKeywordOnHardware(
        l_fruPath, l_sysCfgJsonObj, i_paramsToWriteData);

    if (l_bytesUpdatedOnHardware == -1)
    {
        return l_bytesUpdatedOnHardware;
    }

    // If inventory D-bus object path is present, perform update
    if (!l_inventoryObjPath.empty())
    {
        types::Record l_recordName;
        std::string l_interfaceName;
        std::string l_propertyName;
        types::DbusVariantType l_keywordValue;

        if (const types::IpzData* l_ipzData =
                std::get_if<types::IpzData>(&i_paramsToWriteData))
        {
            l_recordName = std::get<0>(*l_ipzData);
            l_interfaceName = constants::ipzVpdInf + l_recordName;
            l_propertyName = std::get<1>(*l_ipzData);

            try
            {
                // Read keyword's value from hardware to write the same on
                // D-bus.
                l_keywordValue =
                    readKeyword(l_fruPath, types::ReadVpdParams(std::make_tuple(
                                               l_recordName, l_propertyName)));
            }
            catch (const std::exception& l_exception)
            {
                // TODO: Log PEL
                // Unable to read keyword's value from hardware.
                return -1;
            }
        }
        else
        {
            // Input parameter type provided isn't compatible to perform update.
            return -1;
        }

        // Create D-bus object map
        types::ObjectMap l_dbusObjMap = {std::make_pair(
            l_inventoryObjPath,
            types::InterfaceMap{std::make_pair(
                l_interfaceName, types::PropertyMap{std::make_pair(
                                     l_propertyName, l_keywordValue)})})};

        // Call PIM's Notify method to perform update
        if (!dbusUtility::callPIM(std::move(l_dbusObjMap)))
        {
            // Call to PIM's Notify method failed.
            return -1;
        }
    }

    // Update keyword's value on redundant hardware if present
    if (!l_redundantFruPath.empty())
    {
        int l_bytesUpdatedOnRedundantHw = updateKeywordOnHardware(
            l_redundantFruPath, l_sysCfgJsonObj, i_paramsToWriteData);

        if (l_bytesUpdatedOnRedundantHw == -1)
        {
            return l_bytesUpdatedOnRedundantHw;
        }
    }

    // TODO: Check if revert is required when any of the writes fails.
    // TODO: Handle error logging

    // All updates are successful.
    return l_bytesUpdatedOnHardware;
}

types::DbusVariantType
    Manager::readKeyword(const types::Path i_fruPath,
                         const types::ReadVpdParams i_paramsToReadData)
{
    try
    {
        nlohmann::json l_jsonObj{};

        if (m_worker.get() != nullptr)
        {
            l_jsonObj = m_worker->getSysCfgJsonObj();
        }

        std::error_code ec;

        // Check if given path is filesystem path
        if (!std::filesystem::exists(i_fruPath, ec) && (ec))
        {
            throw std::runtime_error("Given file path " + i_fruPath +
                                     " not found.");
        }

        logging::logMessage("Performing VPD read on " + i_fruPath);

        std::shared_ptr<vpd::Parser> l_parserObj =
            std::make_shared<vpd::Parser>(i_fruPath, l_jsonObj);

        std::shared_ptr<vpd::ParserInterface> l_vpdParserInstance =
            l_parserObj->getVpdParserInstance();

        return (
            l_vpdParserInstance->readKeywordFromHardware(i_paramsToReadData));
    }
    catch (const std::exception& e)
    {
        logging::logMessage(
            e.what() + std::string(". VPD manager read operation failed for ") +
            i_fruPath);
        throw types::DeviceError::ReadFailure();
    }
}

void Manager::collectSingleFruVpd(
    const sdbusplus::message::object_path& i_dbusObjPath)
{
    try
    {
        // TODO: Check if CM flag enabled for the given D-bus object path in
        // system config JSON.

        // Check if BMC reaches Ready state
        const auto l_dbusValue = dbusUtility::readDbusProperty(
            constants::bmcStateService, constants::bmcZeroStateObject,
            constants::bmcStateInterface, constants::currentBMCStateProperty);

        if (const auto l_currentBMCSate =
                std::get_if<std::string>(&l_dbusValue))
        {
            if (*l_currentBMCSate != constants::bmcReadyState)
            {
                throw std::runtime_error(
                    "BMC not ready. Single FRU VPD collection failed for " +
                    std::string(i_dbusObjPath));
            }
        }

        // Get system config JSON object from worker class
        nlohmann::json l_sysCfgJsonObj{};

        if (m_worker.get() != nullptr)
        {
            l_sysCfgJsonObj = m_worker->getSysCfgJsonObj();
        }

        // Check if system config JSON is present
        if (l_sysCfgJsonObj.empty())
        {
            throw std::runtime_error(
                "System config JSON object not present. Single FRU VPD collection failed for " +
                std::string(i_dbusObjPath));
        }

        // Get FRU path for the given D-bus object path from JSON
        const std::string& l_fruPath =
            jsonUtility::getFruPathFromJson(l_sysCfgJsonObj, i_dbusObjPath);

        if (l_fruPath.empty())
        {
            throw std::runtime_error(
                "D-bus object path not present in JSON. Single FRU VPD collection failed for " +
                std::string(i_dbusObjPath));
        }

        // Parse VPD
        types::VPDMapVariant l_parsedVpd = m_worker->parseVpdFile(l_fruPath);

        // If l_parsedVpd is pointing to std::monostate
        if (l_parsedVpd.index() == 0)
        {
            throw std::runtime_error("VPD parsing failed for " +
                                     std::string(i_dbusObjPath));
        }

        // Get D-bus object map from worker class
        types::ObjectMap l_dbusObjectMap;
        m_worker->populateDbus(l_parsedVpd, l_dbusObjectMap, l_fruPath);

        if (l_dbusObjectMap.empty())
        {
            throw std::runtime_error(
                "Failed to create D-bus object map. Single FRU VPD collection failed for " +
                std::string(i_dbusObjPath));
        }

        // Call PIM's Notify method
        if (!dbusUtility::callPIM(move(l_dbusObjectMap)))
        {
            throw std::runtime_error(
                "Notify PIM failed. Single FRU VPD collection failed for " +
                std::string(i_dbusObjPath));
        }
    }
    catch (const std::exception& l_error)
    {
        // TODO: Log PEL
        logging::logMessage(std::string(l_error.what()));
    }
}

void Manager::deleteSingleFruVpd(
    const sdbusplus::message::object_path& i_dbusObjPath)
{
    // Dummy code to supress unused variable warning. To be removed.
    logging::logMessage(std::string(i_dbusObjPath));
}

bool Manager::isValidUnexpandedLocationCode(
    const std::string& i_unexpandedLocationCode)
{
    if ((i_unexpandedLocationCode.length() <
         constants::UNEXP_LOCATION_CODE_MIN_LENGTH) ||
        ((i_unexpandedLocationCode.compare(0, 4, "Ufcs") !=
          constants::STR_CMP_SUCCESS) &&
         (i_unexpandedLocationCode.compare(0, 4, "Umts") !=
          constants::STR_CMP_SUCCESS)) ||
        ((i_unexpandedLocationCode.length() >
          constants::UNEXP_LOCATION_CODE_MIN_LENGTH) &&
         (i_unexpandedLocationCode.find("-") != 4)))
    {
        return false;
    }

    return true;
}

std::string Manager::getExpandedLocationCode(
    const std::string& i_unexpandedLocationCode,
    [[maybe_unused]] const uint16_t i_nodeNumber)
{
    if (!isValidUnexpandedLocationCode(i_unexpandedLocationCode))
    {
        phosphor::logging::elog<types::DbusInvalidArgument>(
            types::InvalidArgument::ARGUMENT_NAME("LOCATIONCODE"),
            types::InvalidArgument::ARGUMENT_VALUE(
                i_unexpandedLocationCode.c_str()));
    }

    const nlohmann::json& l_sysCfgJsonObj = m_worker->getSysCfgJsonObj();
    if (!l_sysCfgJsonObj.contains("frus"))
    {
        logging::logMessage("Missing frus tag in system config JSON");
    }

    const nlohmann::json& l_listOfFrus =
        l_sysCfgJsonObj["frus"].get_ref<const nlohmann::json::object_t&>();

    for (const auto& l_frus : l_listOfFrus.items())
    {
        for (const auto& l_aFru : l_frus.value())
        {
            if (l_aFru["extraInterfaces"].contains(
                    constants::locationCodeInf) &&
                l_aFru["extraInterfaces"][constants::locationCodeInf].value(
                    "LocationCode", "") == i_unexpandedLocationCode)
            {
                return std::get<std::string>(dbusUtility::readDbusProperty(
                    l_aFru["serviceName"], l_aFru["inventoryPath"],
                    constants::locationCodeInf, "LocationCode"));
            }
        }
    }
    phosphor::logging::elog<types::DbusInvalidArgument>(
        types::InvalidArgument::ARGUMENT_NAME("LOCATIONCODE"),
        types::InvalidArgument::ARGUMENT_VALUE(
            i_unexpandedLocationCode.c_str()));
}

types::ListOfPaths Manager::getFrusByUnexpandedLocationCode(
    const std::string& i_unexpandedLocationCode,
    [[maybe_unused]] const uint16_t i_nodeNumber)
{
    types::ListOfPaths l_inventoryPaths;

    if (!isValidUnexpandedLocationCode(i_unexpandedLocationCode))
    {
        phosphor::logging::elog<types::DbusInvalidArgument>(
            types::InvalidArgument::ARGUMENT_NAME("LOCATIONCODE"),
            types::InvalidArgument::ARGUMENT_VALUE(
                i_unexpandedLocationCode.c_str()));
    }

    const nlohmann::json& l_sysCfgJsonObj = m_worker->getSysCfgJsonObj();
    if (!l_sysCfgJsonObj.contains("frus"))
    {
        logging::logMessage("Missing frus tag in system config JSON");
    }

    const nlohmann::json& l_listOfFrus =
        l_sysCfgJsonObj["frus"].get_ref<const nlohmann::json::object_t&>();

    for (const auto& l_frus : l_listOfFrus.items())
    {
        for (const auto& l_aFru : l_frus.value())
        {
            if (l_aFru["extraInterfaces"].contains(
                    constants::locationCodeInf) &&
                l_aFru["extraInterfaces"][constants::locationCodeInf].value(
                    "LocationCode", "") == i_unexpandedLocationCode)
            {
                l_inventoryPaths.push_back(
                    l_aFru.at("inventoryPath")
                        .get_ref<const nlohmann::json::string_t&>());
            }
        }
    }

    if (l_inventoryPaths.empty())
    {
        phosphor::logging::elog<types::DbusInvalidArgument>(
            types::InvalidArgument::ARGUMENT_NAME("LOCATIONCODE"),
            types::InvalidArgument::ARGUMENT_VALUE(
                i_unexpandedLocationCode.c_str()));
    }

    return l_inventoryPaths;
}

std::string
    Manager::getHwPath(const sdbusplus::message::object_path& i_dbusObjPath)
{
    // Dummy code to supress unused variable warning. To be removed.
    logging::logMessage(std::string(i_dbusObjPath));

    return std::string{};
}

std::tuple<std::string, uint16_t> Manager::getUnexpandedLocationCode(
    const std::string& i_expandedLocationCode)
{
    /**
     * Location code should always start with U and fulfil minimum length
     * criteria.
     */
    if (i_expandedLocationCode[0] != 'U' ||
        i_expandedLocationCode.length() <
            constants::EXP_LOCATION_CODE_MIN_LENGTH)
    {
        phosphor::logging::elog<types::DbusInvalidArgument>(
            types::InvalidArgument::ARGUMENT_NAME("LOCATIONCODE"),
            types::InvalidArgument::ARGUMENT_VALUE(
                i_expandedLocationCode.c_str()));
    }

    std::string l_fcKwd;

    auto l_fcKwdValue = dbusUtility::readDbusProperty(
        "xyz.openbmc_project.Inventory.Manager",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard",
        "com.ibm.ipzvpd.VCEN", "FC");

    if (auto l_kwdValue = std::get_if<types::BinaryVector>(&l_fcKwdValue))
    {
        l_fcKwd.assign(l_kwdValue->begin(), l_kwdValue->end());
    }

    // Get the first part of expanded location code to check for FC or TM.
    std::string l_firstKwd = i_expandedLocationCode.substr(1, 4);

    std::string l_unexpandedLocationCode{};
    uint16_t l_nodeNummber = constants::INVALID_NODE_NUMBER;

    // Check if this value matches the value of FC keyword.
    if (l_fcKwd.substr(0, 4) == l_firstKwd)
    {
        /**
         * Period(.) should be there in expanded location code to seggregate FC,
         * node number and SE values.
         */
        size_t l_nodeStartPos = i_expandedLocationCode.find('.');
        if (l_nodeStartPos == std::string::npos)
        {
            phosphor::logging::elog<types::DbusInvalidArgument>(
                types::InvalidArgument::ARGUMENT_NAME("LOCATIONCODE"),
                types::InvalidArgument::ARGUMENT_VALUE(
                    i_expandedLocationCode.c_str()));
        }

        size_t l_nodeEndPos = i_expandedLocationCode.find('.',
                                                          l_nodeStartPos + 1);
        if (l_nodeEndPos == std::string::npos)
        {
            phosphor::logging::elog<types::DbusInvalidArgument>(
                types::InvalidArgument::ARGUMENT_NAME("LOCATIONCODE"),
                types::InvalidArgument::ARGUMENT_VALUE(
                    i_expandedLocationCode.c_str()));
        }

        // Skip 3 bytes for '.ND'
        l_nodeNummber = std::stoi(i_expandedLocationCode.substr(
            l_nodeStartPos + 3, (l_nodeEndPos - l_nodeStartPos - 3)));

        /**
         * Confirm if there are other details apart FC, node number and SE in
         * location code
         */
        if (i_expandedLocationCode.length() >
            constants::EXP_LOCATION_CODE_MIN_LENGTH)
        {
            l_unexpandedLocationCode =
                i_expandedLocationCode[0] + "fcs" +
                i_expandedLocationCode.substr(l_nodeEndPos + 1 +
                                                  constants::SE_KWD_LENGTH,
                                              std::string::npos);
        }
        else
        {
            l_unexpandedLocationCode = "Ufcs";
        }
    }
    else
    {
        std::string l_tmKwd;
        // Read TM keyword value.
        auto l_tmKwdValue = dbusUtility::readDbusProperty(
            "xyz.openbmc_project.Inventory.Manager",
            "/xyz/openbmc_project/inventory/system/chassis/motherboard",
            "com.ibm.ipzvpd.VSYS", "TM");

        if (auto l_kwdValue = std::get_if<types::BinaryVector>(&l_tmKwdValue))
        {
            l_tmKwd.assign(l_kwdValue->begin(), l_kwdValue->end());
        }

        // Check if the substr matches to TM keyword value.
        if (l_tmKwd.substr(0, 4) == l_firstKwd)
        {
            /**
             * System location code will not have node number and any other
             * details.
             */
            l_unexpandedLocationCode = "Umts";
        }
        // The given location code is neither "fcs" or "mts".
        else
        {
            phosphor::logging::elog<types::DbusInvalidArgument>(
                types::InvalidArgument::ARGUMENT_NAME("LOCATIONCODE"),
                types::InvalidArgument::ARGUMENT_VALUE(
                    i_expandedLocationCode.c_str()));
        }
    }

    return std::make_tuple(l_unexpandedLocationCode, l_nodeNummber);
}

types::ListOfPaths Manager::getFrusByExpandedLocationCode(
    const std::string& i_expandedLocationCode)
{
    std::tuple<std::string, uint16_t> l_locationAndNodePair =
        getUnexpandedLocationCode(i_expandedLocationCode);

    return getFrusByUnexpandedLocationCode(std::get<0>(l_locationAndNodePair),
                                           std::get<1>(l_locationAndNodePair));
}

void Manager::performVPDRecollection() {}

int Manager::updateKeywordOnHardware(
    const types::Path& i_fruPath, const nlohmann::json& i_sysCfgJsonObj,
    const types::WriteVpdParams i_paramsToWriteData)
{
    try
    {
        std::shared_ptr<Parser> l_parserObj =
            std::make_shared<Parser>(i_fruPath, i_sysCfgJsonObj);

        std::shared_ptr<ParserInterface> l_vpdParserInstance =
            l_parserObj->getVpdParserInstance();

        return (
            l_vpdParserInstance->writeKeywordOnHardware(i_paramsToWriteData));
    }
    catch (const std::exception& l_error)
    {
        // TODO : Log PEL
        return -1;
    }
}
} // namespace vpd
