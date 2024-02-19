#include "config.h"

#include "worker.hpp"

#include "configuration.hpp"
#include "constants.hpp"
#include "exceptions.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "parser_factory.hpp"
#include "parser_interface.hpp"
#include "utils.hpp"

#include <filesystem>
#include <fstream>
#include <future>
#include <typeindex>

namespace vpd
{

Worker::Worker(std::string pathToConfigJson) :
    m_configJsonPath(pathToConfigJson)
{
    // Implies the processing is based on some config JSON
    if (!m_configJsonPath.empty())
    {
        // Check if symlink is already there to confirm fresh boot/factory
        // reset.
        if (std::filesystem::exists(INVENTORY_JSON_SYM_LINK))
        {
            logging::logMessage("Sym Link already present");
            m_configJsonPath = INVENTORY_JSON_SYM_LINK;
            m_isSymlinkPresent = true;
        }

        try
        {
            m_parsedJson = utils::getParsedJson(m_configJsonPath);

            // check for mandatory fields at this point itself.
            if (!m_parsedJson.contains("frus"))
            {
                throw std::runtime_error("Mandatory tag(s) missing from JSON");
            }
        }
        catch (const std::exception& ex)
        {
            throw(JsonException(ex.what(), m_configJsonPath));
        }
    }
    else
    {
        logging::logMessage("Processing in not based on any config JSON");
    }
}

#ifdef IBM_SYSTEM
static bool isChassisPowerOn()
{
    auto powerState = utils::readDbusProperty(
        "xyz.openbmc_project.State.Chassis",
        "/xyz/openbmc_project/state/chassis0",
        "xyz.openbmc_project.State.Chassis", "CurrentPowerState");

    if (auto curPowerState = std::get_if<std::string>(&powerState))
    {
        if ("xyz.openbmc_project.State.Chassis.PowerState.On" == *curPowerState)
        {
            logging::logMessage("VPD cannot be read in power on state.");
            return true;
        }
        return false;
    }

    throw std::runtime_error("Dbus call to get chassis power state failed");
}

void Worker::performInitialSetup()
{
    try
    {
        if (isChassisPowerOn())
        {
            // Nothing needs to be done. Service restarted for some reason.
            return;
        }

        // Check if sysmlink to inventory JSON already exist.
        if (m_isSymlinkPresent)
        {
            types::VPDMapVariant parsedVpdMap;
            fillVPDMap(SYSTEM_VPD_FILE_PATH, parsedVpdMap);

            if (isSystemVPDOnDBus())
            {
                // TODO
                //  Restore system VPD logic should initiate from here.
            }

            publishSystemVPD(parsedVpdMap);
        }
        else
        {
            // implies it is a fresh boot/factory reset.
            // Create the directory for hosting the symlink
            std::filesystem::create_directories(VPD_SYMLIMK_PATH);
            setDeviceTreeAndJson();
        }
    }
    catch (const std::exception& ex)
    {
        if (typeid(ex) == std::type_index(typeid(DataException)))
        {
            // TODO:Catch logic to be implemented once PEL code goes in.
        }
        else if (typeid(ex) == std::type_index(typeid(EccException)))
        {
            // TODO:Catch logic to be implemented once PEL code goes in.
        }
        else if (typeid(ex) == std::type_index(typeid(JsonException)))
        {
            // TODO:Catch logic to be implemented once PEL code goes in.
        }

        logging::logMessage(ex.what());
        throw;
    }
}
#endif

static std::string readFitConfigValue()
{
    std::vector<std::string> output = utils::executeCmd("/sbin/fw_printenv");
    std::string fitConfigValue;

    for (const auto& entry : output)
    {
        auto pos = entry.find("=");
        auto key = entry.substr(0, pos);
        if (key != "fitconfig")
        {
            continue;
        }

        if (pos + 1 < entry.size())
        {
            fitConfigValue = entry.substr(pos + 1);
        }
    }

    return fitConfigValue;
}

bool Worker::isSystemVPDOnDBus() const
{
    const std::string& mboardPath =
        m_parsedJson["frus"][SYSTEM_VPD_FILE_PATH].at(0).value("inventoryPath",
                                                               "");

    if (mboardPath.empty())
    {
        throw JsonException("System vpd file path missing in JSON",
                            INVENTORY_JSON_SYM_LINK);
    }

    std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board.Motherboard"};

    types::MapperGetObject objectMap =
        utils::getObjectMap(PIM_PATH_PREFIX + mboardPath, interfaces);

    if (objectMap.empty())
    {
        return false;
    }
    return true;
}

std::string Worker::getIMValue(const types::IPZVpdMap& parsedVpd) const
{
    if (parsedVpd.empty())
    {
        throw std::runtime_error("Empty VPD map. Can't Extract IM value");
    }

    const auto& itrToVSBP = parsedVpd.find("VSBP");
    if (itrToVSBP == parsedVpd.end())
    {
        throw DataException("VSBP record missing.");
    }

    const auto& itrToIM = (itrToVSBP->second).find("IM");
    if (itrToIM == (itrToVSBP->second).end())
    {
        throw DataException("IM keyword missing.");
    }

    types::BinaryVector imVal;
    std::copy(itrToIM->second.begin(), itrToIM->second.end(),
              back_inserter(imVal));

    std::ostringstream imData;
    for (auto& aByte : imVal)
    {
        imData << std::setw(2) << std::setfill('0') << std::hex
               << static_cast<int>(aByte);
    }

    return imData.str();
}

std::string Worker::getHWVersion(const types::IPZVpdMap& parsedVpd) const
{
    if (parsedVpd.empty())
    {
        throw std::runtime_error("Empty VPD map. Can't Extract IM value");
    }

    const auto& itrToVINI = parsedVpd.find("VINI");
    if (itrToVINI == parsedVpd.end())
    {
        throw DataException("VINI record missing.");
    }

    const auto& itrToHW = (itrToVINI->second).find("HW");
    if (itrToHW == (itrToVINI->second).end())
    {
        throw DataException("HW keyword missing.");
    }

    types::BinaryVector hwVal;
    std::copy(itrToHW->second.begin(), itrToHW->second.end(),
              back_inserter(hwVal));

    // The planar pass only comes from the LSB of the HW keyword,
    // where as the MSB is used for other purposes such as signifying clock
    // termination.
    hwVal[0] = 0x00;

    std::ostringstream hwString;
    for (auto& aByte : hwVal)
    {
        hwString << std::setw(2) << std::setfill('0') << std::hex
                 << static_cast<int>(aByte);
    }

    return hwString.str();
}

void Worker::fillVPDMap(const std::string& vpdFilePath,
                        types::VPDMapVariant& vpdMap)
{
    logging::logMessage(std::string("Parsing file = ") + vpdFilePath);

    if (vpdFilePath.empty())
    {
        throw std::runtime_error("Invalid file path passed to fillVPDMap API.");
    }

    if (!std::filesystem::exists(vpdFilePath))
    {
        throw std::runtime_error("Can't Find physical file");
    }

    try
    {
        std::shared_ptr<Parser> vpdParser =
            std::make_shared<Parser>(vpdFilePath, m_parsedJson);
        vpdMap = vpdParser->parse();
    }
    catch (const std::exception& ex)
    {
        if (typeid(ex) == std::type_index(typeid(DataException)))
        {
            // TODO: Do what needs to be done in case of Data exception.
            // Uncomment when PEL implementation goes in.
            /* string errorMsg =
                 "VPD file is either empty or invalid. Parser failed for [";
             errorMsg += m_vpdFilePath;
             errorMsg += "], with error = " + std::string(ex.what());

             additionalData.emplace("DESCRIPTION", errorMsg);
             additionalData.emplace("CALLOUT_INVENTORY_PATH",
                                    INVENTORY_PATH + baseFruInventoryPath);
             createPEL(additionalData, pelSeverity, errIntfForInvalidVPD,
             nullptr);*/

            // throw generic error from here to inform main caller about
            // failure.
            logging::logMessage(ex.what());
            throw std::runtime_error(
                "Data Exception occurred for file path = " + vpdFilePath);
        }

        if (typeid(ex) == std::type_index(typeid(EccException)))
        {
            // TODO: Do what needs to be done in case of ECC exception.
            // Uncomment when PEL implementation goes in.
            /* additionalData.emplace("DESCRIPTION", "ECC check failed");
             additionalData.emplace("CALLOUT_INVENTORY_PATH",
                                    INVENTORY_PATH + baseFruInventoryPath);
             createPEL(additionalData, pelSeverity, errIntfForEccCheckFail,
                       nullptr);
             */

            logging::logMessage(ex.what());
            // Need to decide once all error handling is implemented.
            // utils::dumpBadVpd(vpdFilePath,vpdVector);

            // throw generic error from here to inform main caller about
            // failure.
            throw std::runtime_error("Ecc Exception occurred for file path = " +
                                     vpdFilePath);
        }
    }
}

void Worker::getSystemJson(std::string& systemJson,
                           const types::VPDMapVariant& parsedVpdMap)
{
    if (auto pVal = std::get_if<types::IPZVpdMap>(&parsedVpdMap))
    {
        std::string hwKWdValue = getHWVersion(*pVal);
        if (hwKWdValue.empty())
        {
            throw DataException("HW value fetched is empty.");
        }

        const std::string& imKwdValue = getIMValue(*pVal);
        if (imKwdValue.empty())
        {
            throw DataException("IM value fetched is empty.");
        }

        auto itrToIM = config::systemType.find(imKwdValue);
        if (itrToIM == config::systemType.end())
        {
            throw DataException("IM keyword does not map to any system type");
        }

        const types::HWVerList hwVersionList = itrToIM->second.second;
        if (!hwVersionList.empty())
        {
            transform(hwKWdValue.begin(), hwKWdValue.end(), hwKWdValue.begin(),
                      ::toupper);

            auto itrToHW = std::find_if(hwVersionList.begin(),
                                        hwVersionList.end(),
                                        [&hwKWdValue](const auto& aPair) {
                return aPair.first == hwKWdValue;
            });

            if (itrToHW != hwVersionList.end())
            {
                systemJson += imKwdValue + "_" + (*itrToHW).second + ".json";
                return;
            }
        }
        systemJson += itrToIM->second.first + ".json";
        return;
    }

    throw DataException("Invalid VPD type returned from Parser");
}

static void setEnvAndReboot(const std::string& key, const std::string& value)
{
    // set env and reboot and break.
    utils::executeCmd("/sbin/fw_setenv", key, value);
    logging::logMessage("Rebooting BMC to pick up new device tree");

    // make dbus call to reboot
    auto bus = sdbusplus::bus::new_default_system();
    auto method = bus.new_method_call(
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "Reboot");
    bus.call_noreply(method);
}

void Worker::setDeviceTreeAndJson()
{
    // JSON is madatory for processing of this API.
    if (m_parsedJson.empty())
    {
        throw std::runtime_error("JSON is empty");
    }

    types::VPDMapVariant parsedVpdMap;
    fillVPDMap(SYSTEM_VPD_FILE_PATH, parsedVpdMap);

    // Do we have the entry for device tree in parsed JSON?
    if (m_parsedJson.find("devTree") == m_parsedJson.end())
    {
        // Implies it is default JSON.
        std::string systemJson{JSON_ABSOLUTE_PATH_PREFIX};
        getSystemJson(systemJson, parsedVpdMap);

        if (!systemJson.compare(JSON_ABSOLUTE_PATH_PREFIX))
        {
            // TODO: Log a PEL saying that "System type not supported"
            throw DataException("Error in getting system JSON.");
        }

        // create a new symlink based on the system
        std::filesystem::create_symlink(systemJson, INVENTORY_JSON_SYM_LINK);

        // re-parse the JSON once appropriate JSON has been selected.
        try
        {
            m_parsedJson = nlohmann::json::parse(INVENTORY_JSON_SYM_LINK);
        }
        catch (const nlohmann::json::parse_error& ex)
        {
            throw(JsonException("Json parsing failed", systemJson));
        }
    }

    auto devTreeFromJson = m_parsedJson["devTree"].value("path", "");
    if (devTreeFromJson.empty())
    {
        throw JsonException("Mandatory value for device tree missing from JSON",
                            INVENTORY_JSON_SYM_LINK);
    }

    auto fitConfigVal = readFitConfigValue();

    if (fitConfigVal.find(devTreeFromJson) != std::string::npos)
    {
        // fitconfig is updated and correct JSON is set.

        // proceed to publish system VPD.
        publishSystemVPD(parsedVpdMap);
        return;
    }

    // Set fitconfig even if it is read as empty.
    setEnvAndReboot("fitconfig", devTreeFromJson);
    exit(EXIT_SUCCESS);
}

void Worker::populateIPZVPDpropertyMap(
    types::InterfaceMap& interfacePropMap,
    const types::IPZKwdValueMap& keyordValueMap,
    const std::string& interfaceName)
{
    types::PropertyMap propertyValueMap;
    for (const auto& kwdVal : keyordValueMap)
    {
        auto kwd = kwdVal.first;

        if (kwd[0] == '#')
        {
            kwd = std::string("PD_") + kwd[1];
        }
        else if (isdigit(kwd[0]))
        {
            kwd = std::string("N_") + kwd;
        }

        types::BinaryVector value(kwdVal.second.begin(), kwdVal.second.end());
        propertyValueMap.emplace(move(kwd), move(value));
    }

    if (!propertyValueMap.empty())
    {
        interfacePropMap.emplace(interfaceName, propertyValueMap);
    }
}

void Worker::populateKwdVPDpropertyMap(const types::KeywordVpdMap& keyordVPDMap,
                                       types::InterfaceMap& interfaceMap)
{
    for (const auto& kwdValMap : keyordVPDMap)
    {
        types::PropertyMap propertyValueMap;
        auto kwd = kwdValMap.first;

        if (kwd[0] == '#')
        {
            kwd = std::string("PD_") + kwd[1];
        }
        else if (isdigit(kwd[0]))
        {
            kwd = std::string("N_") + kwd;
        }

        if (auto keywordValue = get_if<types::BinaryVector>(&kwdValMap.second))
        {
            types::BinaryVector value((*keywordValue).begin(),
                                      (*keywordValue).end());
            propertyValueMap.emplace(move(kwd), move(value));
        }
        else if (auto keywordValue = get_if<std::string>(&kwdValMap.second))
        {
            types::BinaryVector value((*keywordValue).begin(),
                                      (*keywordValue).end());
            propertyValueMap.emplace(move(kwd), move(value));
        }
        else if (auto keywordValue = get_if<size_t>(&kwdValMap.second))
        {
            if (kwd == "MemorySizeInKB")
            {
                types::PropertyMap memProp;
                memProp.emplace(move(kwd), ((*keywordValue)));
                interfaceMap.emplace("xyz.openbmc_project.Inventory.Item.Dimm",
                                     move(memProp));
                continue;
            }
            else
            {
                logging::logMessage("Unknown Keyword =" + kwd +
                                    " found in keyword VPD map");
                continue;
            }
        }
        else
        {
            logging::logMessage(
                "Unknown variant type found in keyword VPD map.");
            continue;
        }

        if (!propertyValueMap.empty())
        {
            interfaceMap.emplace(constants::kwdVpdInf, move(propertyValueMap));
        }
    }
}

void Worker::populateInterfaces(const nlohmann::json& interfaceJson,
                                types::InterfaceMap& interfaceMap,
                                const types::VPDMapVariant& parsedVpdMap)
{
    for (const auto& interfacesPropPair : interfaceJson.items())
    {
        const std::string& interface = interfacesPropPair.key();
        types::PropertyMap propertyMap;

        for (const auto& propValuePair : interfacesPropPair.value().items())
        {
            const std::string property = propValuePair.key();

            if (propValuePair.value().is_boolean())
            {
                propertyMap.emplace(property,
                                    propValuePair.value().get<bool>());
            }
            else if (propValuePair.value().is_string())
            {
                if (property.compare("LocationCode") == 0 &&
                    interface.compare("com.ibm.ipzvpd.Location") == 0)
                {
                    std::string value = utils::getExpandedLocationCode(
                        propValuePair.value().get<std::string>(), parsedVpdMap);
                }
                else
                {
                    propertyMap.emplace(
                        property, propValuePair.value().get<std::string>());
                }
            }
            else if (propValuePair.value().is_array())
            {
                try
                {
                    propertyMap.emplace(
                        property,
                        propValuePair.value().get<types::BinaryVector>());
                }
                catch (const nlohmann::detail::type_error& e)
                {
                    std::cerr << "Type exception: " << e.what() << "\n";
                }
            }
            else if (propValuePair.value().is_number())
            {
                // For now assume the value is a size_t.  In the future it would
                // be nice to come up with a way to get the type from the JSON.
                propertyMap.emplace(property,
                                    propValuePair.value().get<size_t>());
            }
            else if (propValuePair.value().is_object())
            {
                const std::string& record =
                    propValuePair.value().value("recordName", "");
                const std::string& keyword =
                    propValuePair.value().value("keywordName", "");
                const std::string& encoding =
                    propValuePair.value().value("encoding", "");

                if (auto ipzVpdMap =
                        std::get_if<types::IPZVpdMap>(&parsedVpdMap))
                {
                    if (!record.empty() && !keyword.empty() &&
                        (*ipzVpdMap).count(record) &&
                        (*ipzVpdMap).at(record).count(keyword))
                    {
                        auto encoded = utils::encodeKeyword(
                            ((*ipzVpdMap).at(record).at(keyword)), encoding);
                        propertyMap.emplace(property, encoded);
                    }
                }
                else if (auto kwdVpdMap =
                             std::get_if<types::KeywordVpdMap>(&parsedVpdMap))
                {
                    if (!keyword.empty() && (*kwdVpdMap).count(keyword))
                    {
                        if (auto kwValue = std::get_if<types::BinaryVector>(
                                &(*kwdVpdMap).at(keyword)))
                        {
                            auto encodedValue = utils::encodeKeyword(
                                std::string((*kwValue).begin(),
                                            (*kwValue).end()),
                                encoding);

                            propertyMap.emplace(property, encodedValue);
                        }
                        else if (auto kwValue = std::get_if<std::string>(
                                     &(*kwdVpdMap).at(keyword)))
                        {
                            auto encodedValue = utils::encodeKeyword(
                                std::string((*kwValue).begin(),
                                            (*kwValue).end()),
                                encoding);

                            propertyMap.emplace(property, encodedValue);
                        }
                        else if (auto uintValue = std::get_if<size_t>(
                                     &(*kwdVpdMap).at(keyword)))
                        {
                            propertyMap.emplace(property, *uintValue);
                        }
                        else
                        {
                            logging::logMessage(
                                "Unknown keyword found, Keywrod = " + keyword);
                        }
                    }
                }
            }
        }
        utils::insertOrMerge(interfaceMap, interface, move(propertyMap));
    }
}

bool Worker::isCPUIOGoodOnly(const std::string& pgKeyword)
{
    // TODO implementation
    (void)pgKeyword;
    return false;
}

void Worker::primeInventory(const types::IPZVpdMap& ipzVpdMap,
                            types::ObjectMap primeObjects)
{
    // TODO implementation
    (void)ipzVpdMap;
    (void)primeObjects;
}

void Worker::processEmbeddedAndSynthesizedFrus(const nlohmann::json& singleFru,
                                               types::InterfaceMap& interfaces)
{
    // embedded property(true or false) says whether the subfru is embedded
    // into the parent fru (or) not. VPD sets Present property only for
    // embedded frus. If the subfru is not an embedded FRU, the subfru may
    // or may not be physically present. Those non embedded frus will always
    // have Present=false irrespective of its physical presence or absence.
    // Eg: nvme drive in nvme slot is not an embedded FRU. So don't set
    // Present to true for such sub frus.
    // Eg: ethernet port is embedded into bmc card. So set Present to true
    // for such sub frus. Also donot populate present property for embedded
    // subfru which is synthesized. Currently there is no subfru which are
    // both embedded and synthesized. But still the case is handled here.

    // Check if its required to handle presence for this FRU.
    if (singleFru.value("handlePresence", true))
    {
        types::PropertyMap presProp;
        presProp.emplace("Present", true);
        utils::insertOrMerge(interfaces, "xyz.openbmc_project.Inventory.Item",
                             move(presProp));
    }
}

void Worker::processExtraInterfaces(const nlohmann::json& singleFru,
                                    types::InterfaceMap& interfaces,
                                    const types::VPDMapVariant& parsedVpdMap)
{
    populateInterfaces(singleFru["extraInterfaces"], interfaces, parsedVpdMap);
    if (auto ipzVpdMap = std::get_if<types::IPZVpdMap>(&parsedVpdMap))
    {
        if (singleFru["extraInterfaces"].contains(
                "xyz.openbmc_project.Inventory.Item.Cpu"))
        {
            auto itrToRec = (*ipzVpdMap).find("CP00");
            if (itrToRec != (*ipzVpdMap).end())
            {
                return;
            }

            std::string pgKeywordValue;
            utils::getKwVal(itrToRec->second, "PG", pgKeywordValue);
            if (!pgKeywordValue.empty())
            {
                if (isCPUIOGoodOnly(pgKeywordValue))
                {
                    interfaces["xyz.openbmc_project.Inventory.Item"]
                              ["PrettyName"] = "IO Module";
                }
            }
        }
    }
}

void Worker::processCopyRecordFlag(const nlohmann::json& singleFru,
                                   const types::VPDMapVariant& parsedVpdMap,
                                   types::InterfaceMap& interfaces)
{
    if (auto ipzVpdMap = std::get_if<types::IPZVpdMap>(&parsedVpdMap))
    {
        for (const auto& record : singleFru["copyRecords"])
        {
            const std::string& recordName = record;
            if ((*ipzVpdMap).find(recordName) != (*ipzVpdMap).end())
            {
                populateIPZVPDpropertyMap(interfaces,
                                          (*ipzVpdMap).at(recordName),
                                          constants::ipzVpdInf + recordName);
            }
        }
    }
}

void Worker::processInheritFlag(const types::VPDMapVariant& parsedVpdMap,
                                types::InterfaceMap& interfaces)
{
    if (auto ipzVpdMap = std::get_if<types::IPZVpdMap>(&parsedVpdMap))
    {
        for (const auto& [recordName, kwdValueMap] : *ipzVpdMap)
        {
            populateIPZVPDpropertyMap(interfaces, kwdValueMap,
                                      constants::ipzVpdInf + recordName);
        }
    }
    else if (auto kwdVpdMap = std::get_if<types::KeywordVpdMap>(&parsedVpdMap))
    {
        populateKwdVPDpropertyMap(*kwdVpdMap, interfaces);
    }

    if (m_parsedJson.contains("commonInterfaces"))
    {
        populateInterfaces(m_parsedJson["commonInterfaces"], interfaces,
                           parsedVpdMap);
    }
}

bool Worker::processFruWithCCIN(const nlohmann::json& singleFru,
                                const types::VPDMapVariant& parsedVpdMap)
{
    if (auto ipzVPDMap = std::get_if<types::IPZVpdMap>(&parsedVpdMap))
    {
        auto itrToRec = (*ipzVPDMap).find("VINI");
        if (itrToRec != (*ipzVPDMap).end())
        {
            return false;
        }

        std::string ccinFromVpd;
        utils::getKwVal(itrToRec->second, "CC", ccinFromVpd);
        if (ccinFromVpd.empty())
        {
            return true;
        }

        transform(ccinFromVpd.begin(), ccinFromVpd.end(), ccinFromVpd.begin(),
                  ::toupper);

        std::vector<std::string> ccinList;
        for (std::string ccin : singleFru["ccin"])
        {
            transform(ccin.begin(), ccin.end(), ccin.begin(), ::toupper);
            ccinList.push_back(ccin);
        }

        if (ccinList.empty() && (find(ccinList.begin(), ccinList.end(),
                                      ccinFromVpd) == ccinList.end()))
        {
            return false;
        }
    }
    return true;
}

void Worker::populateDbus(const types::VPDMapVariant& parsedVpdMap,
                          types::ObjectMap& objectInterfaceMap,
                          const std::string& vpdFilePath)
{
    if (vpdFilePath.empty())
    {
        throw std::runtime_error(
            "Invalid parameter passed to populateDbus API.");
    }

    // JSON config is mandatory for processing of "if". Add "else" for any
    // processing without config JSON.
    if (!m_parsedJson.empty())
    {
        types::InterfaceMap interfaces;

        for (const auto& aFru : m_parsedJson["frus"][vpdFilePath])
        {
            const auto& inventoryPath = aFru["inventoryPath"];
            sdbusplus::message::object_path fruObjectPath(inventoryPath);
            if (aFru.contains("ccin"))
            {
                if (!processFruWithCCIN(aFru, parsedVpdMap))
                {
                    continue;
                }
            }

            if (aFru.value("inherit", true))
            {
                processInheritFlag(parsedVpdMap, interfaces);
            }

            // If specific record needs to be copied.
            if (aFru.contains("copyRecords"))
            {
                processCopyRecordFlag(aFru, parsedVpdMap, interfaces);
            }

            if (aFru.contains("extraInterfaces"))
            {
                // Process extra interfaces w.r.t a FRU.
                processExtraInterfaces(aFru, interfaces, parsedVpdMap);
            }

            // Process FRUS which are embedded in the parent FRU and whose VPD
            // will be synthesized.
            if ((aFru.value("embedded", true)) &&
                (!aFru.value("synthesized", false)))
            {
                processEmbeddedAndSynthesizedFrus(aFru, interfaces);
            }

            objectInterfaceMap.emplace(std::move(fruObjectPath),
                                       std::move(interfaces));
        }
    }
}

void Worker::publishSystemVPD(const types::VPDMapVariant& parsedVpdMap)
{
    types::ObjectMap objectInterfaceMap;

    if (auto ipzVpdMap = std::get_if<types::IPZVpdMap>(&parsedVpdMap))
    {
        populateDbus(parsedVpdMap, objectInterfaceMap, SYSTEM_VPD_FILE_PATH);
        types::ObjectMap primeObjects;
        primeInventory(*ipzVpdMap, primeObjects);
        objectInterfaceMap.insert(primeObjects.begin(), primeObjects.end());

        // Notify PIM
        if (!utils::callPIM(move(objectInterfaceMap)))
        {
            throw std::runtime_error("Call to PIM failed for system VPD");
        }
    }
    else
    {
        throw DataException("Invalid format of parsed VPD map.");
    }
}

bool Worker::processPreAction(const std::string& i_vpdFilePath,
                              const std::string& i_flagToProcess)
{
    if (i_vpdFilePath.empty() || i_flagToProcess.empty())
    {
        logging::logMessage(
            "Invalid input parameter. Abort processing pre action");
        return false;
    }

    if ((!utils::executePreAction(m_parsedJson, i_vpdFilePath,
                                  i_flagToProcess)) &&
        (i_flagToProcess.compare("collection") == constants::STR_CMP_SUCCESS))
    {
        // TODO: Need a way to delete inventory object from Dbus and persisted
        // data section in case any FRU is not present or there is any
        // problem in collecting it. Once it has been deleted, it can be
        // re-created in the flow of priming the inventory. This needs to be
        // done either here or in the exception section of "parseAndPublishVPD"
        // API. Any failure in the process of collecting FRU will land up in the
        // excpetion of "parseAndPublishVPD".

        // If the FRU is not there, clear the VINI/CCIN data.
        // Enity manager probes for this keyword to look for this
        // FRU, now if the data is persistent on BMC and FRU is
        // removed this can lead to ambiguity. Hence clearing this
        // Keyword if FRU is absent.
        const auto& inventoryPath =
            m_parsedJson["frus"][i_vpdFilePath].at(0).value("inventoryPath",
                                                            "");

        if (!inventoryPath.empty())
        {
            types::ObjectMap l_pimObjMap{
                {inventoryPath,
                 {{constants::kwdVpdInf,
                   {{constants::kwdCCIN, types::BinaryVector{}}}}}}};

            if (!utils::callPIM(std::move(l_pimObjMap)))
            {
                logging::logMessage("Call to PIM failed for file " +
                                    i_vpdFilePath);
            }
        }
        else
        {
            logging::logMessage("Inventory path is empty in Json for file " +
                                i_vpdFilePath);
        }

        return false;
    }
    return true;
}

types::VPDMapVariant Worker::parseVpdFile(const std::string& i_vpdFilePath)
{
    if (i_vpdFilePath.empty())
    {
        throw std::runtime_error(
            "Empty VPD file path passed to Worker::parseVpdFile. Abort processing");
    }

    bool l_isPostFailActionRequired = false;

    // check if the FRU qualifies for pre/post handling.
    if ((m_parsedJson["frus"][i_vpdFilePath].at(0)).contains("preAction"))
    {
        if (processPreAction(i_vpdFilePath, "collection"))
        {
            l_isPostFailActionRequired = true;
        }
        else
        {
            throw std::runtime_error("Pre-Action failed for path " +
                                     i_vpdFilePath +
                                     " Aborting collection for this FRU");
        }
    }

    if (!std::filesystem::exists(i_vpdFilePath))
    {
        if (l_isPostFailActionRequired)
        {
            if (!utils::executePostFailAction(m_parsedJson, i_vpdFilePath,
                                              "collection"))
            {
                throw std::runtime_error("Post fail action failed for path " +
                                         i_vpdFilePath +
                                         " Aborting collection for this FRU");
            }
        }

        throw std::runtime_error("Could not find file path " + i_vpdFilePath +
                                 "Skipping parser trigger for the EEPROM");
    }

    std::shared_ptr<Parser> vpdParser = std::make_shared<Parser>(i_vpdFilePath,
                                                                 m_parsedJson);
    return vpdParser->parse();
}

std::tuple<bool, std::string>
    Worker::parseAndPublishVPD(const std::string& i_vpdFilePath)
{
    try
    {
        const types::VPDMapVariant& parsedVpdMap = parseVpdFile(i_vpdFilePath);

        types::ObjectMap objectInterfaceMap;
        populateDbus(parsedVpdMap, objectInterfaceMap, i_vpdFilePath);

        logging::logMessage("Dbus sucessfully populated for FRU " +
                            i_vpdFilePath);
        // Notify PIM
        /*    if (!utils::callPIM(move(objectInterfaceMap)))
            {
                throw std::runtime_error("Call to PIM failed for system
           VPD");
            }*/
    }
    catch (const std::exception& ex)
    {
        // handle all the exceptions internally. Return only true/false
        // based on status of execution.
        if (typeid(ex) == std::type_index(typeid(DataException)))
        {
            // TODO: Add custom handling
            logging::logMessage(ex.what());
        }
        else if (typeid(ex) == std::type_index(typeid(EccException)))
        {
            // TODO: Add custom handling
            logging::logMessage(ex.what());
        }
        else if (typeid(ex) == std::type_index(typeid(JsonException)))
        {
            // TODO: Add custom handling
            logging::logMessage(ex.what());
        }
        else
        {
            logging::logMessage(ex.what());
        }

        // TODO: before returning, we should prime the inventry for FRUs which
        // are not present/processing had some error.
        return std::make_tuple(false, i_vpdFilePath);
    }
    return std::make_tuple(true, i_vpdFilePath);
}

void Worker::collectFrusFromJson()
{
    // A parsed JSON file should be present to pick FRUs EEPROM paths
    if (m_parsedJson.empty())
    {
        throw std::runtime_error(
            "A config JSON is required for processing of FRUs");
    }

    const nlohmann::json& listOfFrus =
        m_parsedJson["frus"].get_ref<const nlohmann::json::object_t&>();

    // future object to carry return value.
    std::vector<std::future<std::tuple<bool, std::string>>> listOfFutureObject;

    for (const auto& itemFRUS : listOfFrus.items())
    {
        const std::string& vpdFilePath = itemFRUS.key();

        // skip processing of system VPD again as it has been already collected.
        if (vpdFilePath == SYSTEM_VPD_FILE_PATH)
        {
            continue;
        }

        logging::logMessage("Parsing triggered for FRU = " + vpdFilePath);

        listOfFutureObject.push_back(
            std::async(&Worker::parseAndPublishVPD, this, vpdFilePath));
    }

    for (auto& aFutureObj : listOfFutureObject)
    {
        std::tuple<bool, std::string> threadInfo = aFutureObj.get();

        if (!std::get<0>(threadInfo))
        {
            logging::logMessage("Processing failed for = " +
                                std::get<1>(threadInfo));
        }
        else if (std::get<0>(threadInfo))
        {
            logging::logMessage("Processing passed for = " +
                                std::get<1>(threadInfo));
        }
    }
}
} // namespace vpd
