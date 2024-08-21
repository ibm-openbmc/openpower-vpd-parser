#include "bios_handler.hpp"

#include "constants.hpp"
#include "logger.hpp"

#include <sdbusplus/bus/match.hpp>
#include <utility/dbus_utility.hpp>

namespace vpd
{
// Template declaration to define APIs.
template class BiosHandler<IbmBiosHandler>;

template <typename T>
void BiosHandler<T>::checkAndListenPldmService()
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
            // TODO: Restore BIOS attribute from here.
            //  We don't need the match anymore
            l_nameOwnerMatch.reset();
            m_specificBiosHandler->backUpOrRestoreBiosAttributes();

            // Start listener now that we have done the restore.
            listenBiosAttributes();
        }
    });

    // Based on PLDM service status reset owner match registered above and
    // trigger BIOS attribute sync.
    if (dbusUtility::isServiceRunning(constants::pldmServiceName))
    {
        l_nameOwnerMatch.reset();
        m_specificBiosHandler->backUpOrRestoreBiosAttributes();

        // Start listener now that we have done the restore.
        listenBiosAttributes();
    }
}

template <typename T>
void BiosHandler<T>::listenBiosAttributes()
{
    static std::shared_ptr<sdbusplus::bus::match_t> l_biosMatch =
        std::make_shared<sdbusplus::bus::match_t>(
            *m_asioConn,
            sdbusplus::bus::match::rules::propertiesChanged(
                constants::biosConfigMgrObjPath,
                constants::biosConfigMgrService),
            [this](sdbusplus::message_t& l_msg) {
        m_specificBiosHandler->biosAttributesCallback(l_msg);
    });
}

void IbmBiosHandler::biosAttributesCallback(sdbusplus::message_t& i_msg)
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
            // Looking for change in Base BIOS table only.
            continue;
        }

        auto l_attributeList = std::get<0>(l_property.second);

        for (const auto& l_attribute : l_attributeList)
        {
            if (auto l_val = std::get_if<std::string>(
                    &(std::get<5>(std::get<1>(l_attribute)))))
            {
                (void)l_val; // use when APIs are implemented.
                std::string l_attributeName = std::get<0>(l_attribute);
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
                    &(std::get<5>(std::get<1>(l_attribute)))))
            {
                (void)l_val; // use when APIs are implemented.
                std::string l_attributeName = std::get<0>(l_attribute);
                if (l_attributeName == "hb_field_core_override")
                {
                    // TODO: Save FCO to VPD.
                }
            }
        }
    }
}

void IbmBiosHandler::backUpOrRestoreBiosAttributes()
{
    // TODO: Implement IBM specific logic to back up and restore.
}
} // namespace vpd
