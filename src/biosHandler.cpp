#include "biosHandler.hpp"

#include "constants.hpp"
#include "logger.hpp"

#include <sdbusplus/bus/match.hpp>
#include <utility/dbus_utility.hpp>

namespace vpd
{
void BiosHandler::checkAndListenPldmService()
{
    // Setup a call back match on NameOwnerChanged to determine when PLDM is up.
    static std::shared_ptr<sdbusplus::bus::match_t> l_nameOwnerMatch =
        std::make_shared<sdbusplus::bus::match_t>(
            *m_asioConn,
            sdbusplus::bus::match::rules::nameOwnerChanged(
                constants::pldmServiceName),
            [this](sdbusplus::message_t& l_msg) {
        if (l_msg.is_method_error())
        {
            logging::logMessage(
                "Error in reading PLDM name owner changed signal.");
            return;
        }

        std::string l_name;
        std::string l_newOwner;
        std::string l_oldOwner;

        l_msg.read(l_name, l_oldOwner, l_newOwner);

        if (!l_newOwner.empty() &&
            (l_name.compare(constants::pldmServiceName) ==
             constants::STR_CMP_SUCCESS))
        {
            //  We don't need the match anymore
            l_nameOwnerMatch.reset();
            backUpOrRestoreBiosAttributes();
        }
    });

    // Based on PLDM service status reset owner match registered above and
    // trigger BIOS attribute sync.
    if (dbusUtility::isServiceRunning(constants::pldmServiceName))
    {
        l_nameOwnerMatch.reset();
        backUpOrRestoreBiosAttributes();
    }
}

void BiosHandler::backUpOrRestoreBiosAttributes()
{
    auto l_fcoValInVpd = dbusUtility::readDbusProperty(
        constants::pimServiceName, constants::systemVpdInvPath,
        "com.ibm.ipzvpd.VSYS", "RG");

    auto l_mmmValInVpd = dbusUtility::readDbusProperty(
        constants::pimServiceName, constants::systemVpdInvPath,
        "com.ibm.ipzvpd.UTIL", "D0");

    auto l_keepAndClearValInVpd = dbusUtility::readDbusProperty(
        constants::pimServiceName, constants::systemVpdInvPath,
        "com.ibm.ipzvpd.UTIL", "D1");

    // TODO: Implemet APIs to read the vlaues from BIOS table.

    // defined just for readability.
    static constexpr auto fourEmptySpace = "    ";

    // Check if field core override value is default in VPD.
    if (l_fcoValInVpd == fourEmptySpace)
    {
        // TODO: Save the data to VPD.
    }
    else
    {
        // TODO: Save data to BIOS.
    }

    // Check if memory mirror mode value is default in VPD.
    if (l_mmmValInVpd.at(0) == 0)
    {
        // TODO: Save mmm to VPD.
    }
    else
    {
        // TODO: save to BIOS.
    }

    // TODO: Sync keep And Clear in BIOS. No default handling required here as
    // the default value of VPD keyword is also a valid value which will be
    // compared while syncing the value.

    // Start listener now that we have done the restore.
    listenBiosAttributes();
}

void BiosHandler::listenBiosAttributes()
{
    static std::shared_ptr<sdbusplus::bus::match_t> l_biosMatch =
        std::make_shared<sdbusplus::bus::match_t>(
            *m_asioConn,
            sdbusplus::bus::match::rules::propertiesChanged(
                "/xyz/openbmc_project/bios_config/manager",
                "xyz.openbmc_project.BIOSConfig.Manager"),
            [this](sdbusplus::message_t& l_msg) {
        biosAttribsCallback(l_msg);
    });
}

void BiosHandler::biosAttribsCallback(sdbusplus::message_t& i_msg)
{
    if (i_msg.is_method_error())
    {
        logging::logMessage("Error in reading BIOS attribute signal. ");
        return;
    }

    std::string l_objPath;
    types::BiosBaseTableType l_propMap;
    i_msg.read(l_objPath, l_propMap);

    for (auto l_property : l_propMap)
    {
        if (l_property.first != "BaseBIOSTable")
        {
            continue;
        }

        auto l_attributeList = std::get<0>(l_property.second);

        for (const auto& l_anAttribute : l_attributeList)
        {
            if (auto l_val = std::get_if<std::string>(
                    &(std::get<5>(std::get<1>(l_anAttribute)))))
            {
                std::string l_attributeName = std::get<0>(l_anAttribute);
                if (l_attributeName == "hb_memory_mirror_mode")
                {
                    // TODO: Save MMM to VPD.
                }

                if (l_attributeName == "pvm_keep_and_clear")
                {
                    // TODO: Save keep and clear to VPD.
                }

                if (l_attributeName == "pvm_create_default_lpar")
                {
                    // TODO: Save lpar to VPD.
                }

                if (l_attributeName == "pvm_clear_nvram")
                {
                    // TODO: Save clear nvram to VPD.
                }

                continue;
            }

            if (auto l_val = std::get_if<int64_t>(
                    &(std::get<5>(std::get<1>(l_anAttribute)))))
            {
                std::string l_attributeName = std::get<0>(l_anAttribute);
                if (l_attributeName == "hb_field_core_override")
                {
                    // TODO: Save FCO to VPD.
                }
                continue;
            }
        }
    }
}
} // namespace vpd
