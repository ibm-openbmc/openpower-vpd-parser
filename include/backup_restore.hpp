#pragma once

#include "types.hpp"

#include <nlohmann/json.hpp>

#include <tuple>

namespace vpd
{
/**
 * @brief class to implement backup and restore VPD.
 *
 */
class BackupRestore
{
  public:
    // delete functions
    BackupRestore() = delete;
    BackupRestore(const BackupRestore&) = delete;
    BackupRestore& operator=(const BackupRestore&) = delete;
    BackupRestore(BackupRestore&&) = delete;
    BackupRestore& operator=(BackupRestore&&) = delete;

    /**
     * @brief Constructor.
     *
     * @param[in] i_sysCfgJsonObj - Parsed system config JSON.
     *
     * Note: Throws exception in case of construction failure. Caller needs
     * to handle to detect successful object creation.
     */
    BackupRestore(const nlohmann::json& i_sysCfgJsonObj);

    /**
     * @brief Default destructor.
     */
    ~BackupRestore() = default;

    /**
     * @brief An API to backup and restore VPD.
     *
     * Note: This API works on the keywords declared in the backup and restore
     * config JSON. Restore or backup action could be triggered for each
     * keyword, based on the keyword's value present in the source and
     * destination keyword.
     *
     * Restore source keyword's value with destination keyword's value,
     * when source keyword has default value but
     * destination's keyword has non default value.
     *
     * Backup the source keyword value to the destination's keyword's value,
     * when source keyword has non default value but
     * destination's keyword has default value.
     *
     * @return Tuple of updated source and destination VPD map variant.
     */
    std::tuple<types::VPDMapVariant, types::VPDMapVariant> backupAndRestore();

    /**
     * @brief An API to set backup and restore is done or not.
     *
     * @param[in] i_status - Status to set.
     */
    static void setBackupAndRestoreStatus(bool i_status);

  private:
    /**
     * @brief An API to handle backup and restore of IPZ type VPD.
     *
     * @param[in,out] io_srcVpdMap - Source VPD map.
     * @param[in,out] io_dstVpdMap - Destination VPD map.
     * @param[in] i_srcPath - Source EEPROM file path or inventory path.
     * @param[in] i_dstPath - Destination EEPROM file path or inventory path.
     */
    void backupAndRestoreIpzVpd(types::IPZVpdMap& io_srcVpdMap,
                                types::IPZVpdMap& io_dstVpdMap,
                                const std::string& i_srcPath,
                                const std::string& i_dstPath);

    // Parsed system JSON config file.
    nlohmann::json m_sysCfgJsonObj{};

    // Parsed backup and restore's JSON config file.
    nlohmann::json m_backupRestoreCfgJsonObj{};

    // Indicates if backup and restore has been performed.
    static bool m_backupAndRestoreDone;
};

bool BackupRestore::m_backupAndRestoreDone = false;
} // namespace vpd
