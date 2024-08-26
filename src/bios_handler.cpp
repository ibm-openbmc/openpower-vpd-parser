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

    /*
    TODO: Process specific BIOS attributes
    */
}

void IbmBiosHandler::backUpOrRestoreBiosAttributes()
{
    // process FCO
    processFieldCoreOverride();

    // process memory mirror mode
    processMemoryMirrorMode();

    // process keep and clear
    processKeepAndClear();

    // process LPAR
    processLpar();

    // process clear NVRAM
    processClearNvRam();
}

types::BiosAttributeValue
    IbmBiosHandler::readBiosAttribute(const std::string& i_attributeName)
{
    types::BiosAttributeValue l_attrValueVariant =
        dbusUtility::biosGetAttributeMethodCall(i_attributeName);

    return l_attrValueVariant;
}

void IbmBiosHandler::processFieldCoreOverride()
{
    // Read required keyword from VPD.
    auto l_kwdValueVariant = dbusUtility::readDbusProperty(
        constants::pimServiceName, constants::systemVpdInvPath,
        constants::vsysInf, constants::kwd_RG);

    if (auto pVal = std::get_if<types::BinaryVector>(&l_kwdValueVariant))
    {
        // converting to string to compare to default value.
        std::string l_fcoInVpd;
        l_fcoInVpd.assign(reinterpret_cast<const char*>(pVal->data()),
                          pVal->size());

        // Check if field core override value is default in VPD.
        if (l_fcoInVpd == constants::fourEmptySpace)
        {
            // TODO: Save the data to VPD.
        }
        else
        {
            // Restore the data in BIOS.
            types::BiosAttributeValue l_attrValueVariant =
                readBiosAttribute("hb_field_core_override");
            if (auto pVal = std::get_if<int64_t>(&l_attrValueVariant))
            {
                auto l_fcoInBios = *pVal;
                (void)l_fcoInBios;
                // TODO: save data to BIOS

                return;
            }

            logging::logMessage("Invalid type recieved for FCO from BIOS.");
        }
        return;
    }

    logging::logMessage("Invalid type recieved for FCO from VPD.");
}

void IbmBiosHandler::processMemoryMirrorMode()
{
    auto l_kwdValueVariant = dbusUtility::readDbusProperty(
        constants::pimServiceName, constants::systemVpdInvPath,
        constants::utilInf, constants::kwd_D0);

    if (auto pVal = std::get_if<std::string>(&l_kwdValueVariant))
    {
        auto l_mmmValInVpd = *pVal;

        // Check if memory mirror mode value is default in VPD.
        if (l_mmmValInVpd.at(0) == 0)
        {
            // TODO: Save mmm to VPD.
        }
        else
        {
            types::BiosAttributeValue l_attrValueVariant =
                readBiosAttribute("hb_memory_mirror_mode");

            if (auto pVal = std::get_if<std::string>(&l_attrValueVariant))
            {
                std::string l_ammInBios = *pVal;
                (void)l_ammInBios;
                // TODO: save to BIOS.

                return;
            }

            logging::logMessage(
                "Invalid type recieved for memory mode from BIOS.");
        }
        return;
    }
    logging::logMessage("Invalid type recieved for memory mode from VPD.");
}

void IbmBiosHandler::processKeepAndClear()
{
    auto l_kwdValueVariant = dbusUtility::readDbusProperty(
        constants::pimServiceName, constants::systemVpdInvPath,
        constants::utilInf, constants::kwd_D1);

    if (auto pVal = std::get_if<std::string>(&l_kwdValueVariant))
    {
        auto l_keepAndClearValInVpd = *pVal;

        // No default checking is required for keep and clear. Save what is
        // there in VPD.

        types::BiosAttributeValue l_attrValueVariant =
            readBiosAttribute("pvm_keep_and_clear");

        if (auto pVal = std::get_if<std::string>(&l_attrValueVariant))
        {
            std::string l_keepAndClearInBios = *pVal;
            (void)l_keepAndClearInBios;
            // TODO save the value to BIOS.

            return;
        }

        logging::logMessage(
            "Invalid type recieved for keep and clear from BIOS.");

        return;
    }

    logging::logMessage("Invalid type recieved for keep and clear from VPD.");
}

void IbmBiosHandler::processLpar()
{
    // Read required keyword from VPD.
    auto l_kwdValueVariant = dbusUtility::readDbusProperty(
        constants::pimServiceName, constants::systemVpdInvPath,
        constants::utilInf, constants::kwd_D1);

    if (auto pVal = std::get_if<std::string>(&l_kwdValueVariant))
    {
        auto l_createDefaultLparInVpd = *pVal;

        types::BiosAttributeValue l_attrValueVariant =
            readBiosAttribute("pvm_create_default_lpar");

        if (auto pVal = std::get_if<std::string>(&l_attrValueVariant))
        {
            std::string l_createDefaultLparInBios = *pVal;
            (void)l_createDefaultLparInBios;
            // TODO save the value to BIOS.
            return;
        }

        logging::logMessage(
            "Invalid type recieved for create default Lpar from BIOS.");

        return;
    }

    logging::logMessage(
        "Invalid type recieved for create default Lpar from VPD.");
}

void IbmBiosHandler::processClearNvRam()
{
    // Read required keyword from VPD.
    auto l_kwdValueVariant = dbusUtility::readDbusProperty(
        constants::pimServiceName, constants::systemVpdInvPath,
        constants::utilInf, constants::kwd_D1);

    if (auto pVal = std::get_if<std::string>(&l_kwdValueVariant))
    {
        auto l_clearNvRamInVpd = *pVal;
        types::BiosAttributeValue l_attrValueVariant =
            readBiosAttribute("pvm_clear_nvram");
        if (auto pVal = std::get_if<std::string>(&l_attrValueVariant))
        {
            std::string l_clearNvRamInBios = *pVal;
            (void)l_clearNvRamInBios;
            // TODO: save to BIOS
            return;
        }
        logging::logMessage("Invalid type recieved for clear NVRAM from BIOS.");

        return;
    }
    logging::logMessage("Invalid type recieved for clear NVRAM from VPD.");
}
} // namespace vpd
