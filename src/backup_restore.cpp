#include "backup_restore.hpp"

#include "constants.hpp"
#include "exceptions.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "utils.hpp"

namespace vpd
{
BackupRestore::BackupRestore(const nlohmann::json& i_sysCfgJsonObj) :
    m_sysCfgJsonObj(i_sysCfgJsonObj)
{
    m_backupRestoreCfgJsonObj = utils::getParsedJson(
        i_sysCfgJsonObj.value("backupRestoreConfigPath", ""));
}

std::tuple<types::VPDMapVariant, types::VPDMapVariant>
    BackupRestore::backupAndRestore()
{
    auto l_emptyVariantPair = std::make_tuple(std::monostate{},
                                              std::monostate{});
    if (m_backupAndRestoreDone)
    {
        logging::logMessage("Backup and restore triggered already.");
        return l_emptyVariantPair;
    }

    try
    {
        if (m_backupRestoreCfgJsonObj.empty() ||
            !m_backupRestoreCfgJsonObj.contains("source") ||
            !m_backupRestoreCfgJsonObj.contains("destination") ||
            !m_backupRestoreCfgJsonObj.contains("type") ||
            !m_backupRestoreCfgJsonObj.contains("backupMap"))
        {
            logging::logMessage(
                "Backup restore config JSON is missing necessary tag(s), can't initiate backup and restore.");
            return l_emptyVariantPair;
        }

        std::string l_srcVpdPath;
        types::VPDMapVariant l_srcVpdMap;
        if (l_srcVpdPath =
                m_backupRestoreCfgJsonObj["source"].value("hardwarePath", "");
            !l_srcVpdPath.empty() && std::filesystem::exists(l_srcVpdPath))
        {
            std::shared_ptr<Parser> l_vpdParser =
                std::make_shared<Parser>(l_srcVpdPath, m_sysCfgJsonObj);
            l_srcVpdMap = l_vpdParser->parse();
        }
        else if (l_srcVpdPath = m_backupRestoreCfgJsonObj["source"].value(
                     "inventoryPath", "");
                 l_srcVpdPath.empty())
        {
            logging::logMessage(
                "Couldn't extract source path, can't initiate backup and restore.");
            return l_emptyVariantPair;
        }

        std::string l_dstVpdPath;
        types::VPDMapVariant l_dstVpdMap;
        if (l_dstVpdPath = m_backupRestoreCfgJsonObj["destination"].value(
                "hardwarePath", "");
            !l_dstVpdPath.empty() && std::filesystem::exists(l_dstVpdPath))
        {
            std::shared_ptr<Parser> l_vpdParser =
                std::make_shared<Parser>(l_dstVpdPath, m_sysCfgJsonObj);
            l_dstVpdMap = l_vpdParser->parse();
        }
        else if (l_dstVpdPath = m_backupRestoreCfgJsonObj["destination"].value(
                     "inventoryPath", "");
                 l_dstVpdPath.empty())
        {
            logging::logMessage(
                "Couldn't extract destination path, can't initiate backup and restore.");
            return l_emptyVariantPair;
        }

        // Implement backup and restore for IPZ type VPD
        auto l_backupRestoreType = m_backupRestoreCfgJsonObj.value("type", "");
        if (l_backupRestoreType.compare("IPZ") == constants::STR_CMP_SUCCESS)
        {
            types::IPZVpdMap* l_srcVpdPtr = nullptr;
            if (!(l_srcVpdPtr = std::get_if<types::IPZVpdMap>(&l_srcVpdMap)))
            {
                if (std::holds_alternative<std::monostate>(l_srcVpdMap))
                {
                    l_srcVpdPtr = new types::IPZVpdMap();
                }
                else
                {
                    logging::logMessage("Source VPD is not of IPZ type.");
                    return l_emptyVariantPair;
                }
            }

            types::IPZVpdMap* l_dstVpdPtr = nullptr;
            if (!(l_dstVpdPtr = std::get_if<types::IPZVpdMap>(&l_dstVpdMap)))
            {
                if (std::holds_alternative<std::monostate>(l_dstVpdMap))
                {
                    l_dstVpdPtr = new types::IPZVpdMap();
                }
                else
                {
                    logging::logMessage("Destination VPD is not of IPZ type.");
                    return l_emptyVariantPair;
                }
            }

            backupAndRestoreIpzVpd(*l_srcVpdPtr, *l_dstVpdPtr, l_srcVpdPath,
                                   l_dstVpdPath);
            return std::make_tuple(*l_srcVpdPtr, *l_dstVpdPtr);
        }
        // Note: add implementation here to support any other VPD type.
    }
    catch (const std::exception& ex)
    {
        logging::logMessage("Back up and restore failed with exception: " +
                            std::string(ex.what()));
    }
    return l_emptyVariantPair;
}

void BackupRestore::backupAndRestoreIpzVpd(types::IPZVpdMap& io_srcVpdMap,
                                           types::IPZVpdMap& io_dstVpdMap,
                                           const std::string& i_srcPath,
                                           const std::string& i_dstPath)
{
    (void)io_srcVpdMap;
    (void)io_dstVpdMap;
    (void)i_srcPath;
    (void)i_dstPath;
}

void BackupRestore::setBackupAndRestoreStatus(bool i_status)
{
    m_backupAndRestoreDone = i_status;
}
} // namespace vpd
