#include "config.h"

#include "manager.hpp"

#include "exception.hpp"
#include "logger.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace vpd
{
Manager::Manager(const std::shared_ptr<boost::asio::io_context>& ioCon,
                 const std::shared_ptr<sdbusplus::asio::dbus_interface>& iFace,
                 const std::shared_ptr<sdbusplus::asio::connection>& conn) :
    m_ioContext(ioCon),
    m_interface(iFace), m_conn(conn)
{
    logging::logMessage("Create VPD Manager");
    selectDeviceTreeAndJson();
}

bool Manager::selectDeviceTreeAndJson()
{
    try
    {
        auto jsonToParse = INVENTORY_JSON_DEFAULT;
        bool isSymlinkPresent = false;

        // Check if symlink is already there to confirm fresh boot/factory
        // reset.
        if (fs::exists(INVENTORY_JSON_SYM_LINK))
        {
            jsonToParse = INVENTORY_JSON_SYM_LINK;
            isSymlinkPresent = true;
        }
        // implies it is a fresh boot/factory reset.
        else
        {
            // Create the directory for hosting the symlink
            fs::create_directories(VPD_SYMLIMK_PATH);
        }

        ifstream inventoryJson(jsonToParse);
        if (!inventoryJson)
        {
            throw(JsonException("Failed to access Json path", jsonToParse));
        }

        try
        {
            m_parsedJson = json::parse(inventoryJson);
        }
        catch (const json::parse_error& ex)
        {
            throw(JsonException("Json parsing failed", jsonToParse));
        }

        const string& mboardPath = m_parsedJson["frus"][filePath].at(0).value(
            "inventoryPath", jsonToParse);
        if (mboardPath.empty())
        {
            throw JsonException("Motherboard path missing in JSON", )
        }

        // Let this check be after we parse the JSON, so that we have a parsed
        // JSON after every service start.
        if (isSymlinkPresent && motherboardPathOnDBus(mboardPath))
        {
            // As symlink is also present and motherboard path is also
            // populated, constructor of manager is called, it implies, this is
            // a situation where VPD-manager service got restarted due to some
            // reason. Dont process for selection of device tree or JSON
            // selection in this case.
            logger::logMessage("Servcie restarted for some reason.");
            return;
        }

        // Do we have the entry for device tree?
        if (m_parsedJson.find("devTree") == js.end())
        {
            // Implies it is default JSON.
            std::string systemJson{JSON_ABSOLUTE_PATH_PREFIX};
            getSystemJson(systemJson);

            if (!systemJson.compare(JSON_ABSOLUTE_PATH_PREFIX))
            {
                throw DataException("Error in getting system JSON.");
            }

            // create a new symlink based on the system
            fs::create_symlink(systemJson, INVENTORY_JSON_SYM_LINK);

            try
            {
                m_parsedJson = json::parse(INVENTORY_JSON_SYM_LINK);
            }
            catch (const json::parse_error& ex)
            {
                throw(JsonException("Json parsing failed", systemJson));
            }
        }

        auto devTreeFromJson = m_parsedJson["devTree"]["value"];
        auto fitConfigVal = readFitConfigValue();

        if (fitConfigVal.find(devTreeFromJson) != string::npos)
        {
            // fitconfig is updated and correct JSON is set.
            return;
        }

        setEnvAndReboot("fitconfig", devTreeFromJson);
        exit(EXIT_SUCCESS);
    }
    catch (const JsonException& ex)
    {
        // TODO:Catch logic to be implemented once PEL code goes in.
    }
    catch (const EccException& ex)
    {
        // TODO:Catch logic to be implemented once PEL code goes in.
    }
    catch (const DataException& ex)
    {
        // TODO:Catch logic to be implemented once PEL code goes in.
    }
}

bool Manager::motherboardPathOnDBus(std::string_view mboardPath)
{
    std::vector<std::string> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board.Motherboard"};

    types::MapperGetObject objectMap = getObjectMap(mboardPath, interfaces);
    if (objectMap.empty())
    {
        return false;
    }
    return true;
}

string Manager::readFitConfigValue()
{
    std::vector<string> output = executeCmd("/sbin/fw_printenv");
    string fitConfigValue;

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

void Manager::setEnvAndReboot(const string& key, const string& value)
{
    // set env and reboot and break.
    executeCmd("/sbin/fw_setenv", key, value);
    logging::logMessage("Rebooting BMC to pick up new device tree");

    // make dbus call to reboot
    auto bus = sdbusplus::bus::new_default_system();
    auto method = bus.new_method_call(
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "Reboot");
    bus.call_noreply(method);
}

void Manager::getVPDMap(std::string_view vpdFilePath, VPDMapVariant& vpdMap)
{
    // TODO: implement the API to get a parsed VPD map.
}

std::string Manager::getHW(const types::ParsedVPD& vpdMap)
{
    // TODO: Implement to get HW value
}

std::string Manager::getIM(const types::ParsedVPD& vpdMap)
{
    // TODO: Impkement to get IM value
}

void Manager::getSystemJson(std::string& systemJson)
{
    types::SystemTypeMap systemType{
        {"50001001", {"0001"}},
        {"50001000", {"0001"}},
        {"50001002", {}},
        {"50003000", {"000A", "000B", "000C", "0014"}},
        {"50004000", {}}};

    std::variant<types::ParsedVPD, types::KeywordVpdMap> vpdMap;
    getVPDMap(SYSTEM_VPD_FILE_PATH, vpdMap);
    if (auto pVal = get_if<types::ParsedVPD>(&parseResult)
    {
        std::string hwKWdValue = getHW(*pVal);
        const std::string& imKwdValue = getIM(*pVal);

        if (auto itrToIM = systemType.find(imKwdValue))
        {
            if (itrToIM == systemType.end())
            {
                logging::logMessage(
                    "IM keyword does not map to any system type");
                return;
            }

            const auto hwVersionList = itrToIM->second;
            if (!hwVersionList.empty())
            {
                transform(hwKWdValue.begin(), hwKWdValue.end(),
                          hwKWdValue.begin(), ::toupper);

                if (auto itrToHW = hwVersionList.find(hwKWdValue);
                    itrToHW == hwVersionList.end())
                {
                    systemJson += imKwdValue + "_v2.json";
                    return;
                }
            }

            systemJson += imKwdValue + ".json";
        }       
    }
    else
    {
        logging::logMessage("Invalid VPD type returned from Parser");
    }
}
} // namespace vpd