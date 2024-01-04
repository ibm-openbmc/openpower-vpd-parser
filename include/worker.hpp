#pragma once

#include "types.hpp"

#include <nlohmann/json.hpp>

namespace vpd
{
/**
 * @brief A class to process and publish VPD data.
 *
 * The class works on VPD and is mainly responsible for following tasks:
 * 1) Select appropriate device tree and JSON. Reboot if required.
 * 2) Get desired parser using parser factory.
 * 3) Calling respective parser class to get parsed VPD.
 * 4) Arranging VPD data under required interfaces.
 * 5) Calling PIM to publish VPD.
 *
 * The class may also implement helper functions required for VPD handling.
 */
class Worker
{
  public:
    /**
     * List of deleted functions.
     */
    Worker(const Worker&);
    Worker& operator=(const Worker&);
    Worker(Worker&&) = delete;

    /**
     * @brief Constructor.
     *
     * Constructor will also, based on symlink pick the correct JSON and
     * initialize the parsed JSON variable.
     *
     * Note: Throws std::exception in case of construction failure. Caller needs
     * to handle to detect successful object creation.
     */
    Worker();

    /**
     * @brief Destructor
     */
    ~Worker() = default;

    /**
     * @brief An API to perform initial setup before BUS name is claimed.
     *
     * Before BUS name for VPD-Manager is claimed, fitconfig whould be set for
     * corret device tree, inventory JSON w.r.t system should be linked and
     * system VPD should be on DBus.
     */
    void performInitialSetup();

    /**
     * @brief An API to process all FRUs.
     *
     * This API based on inventory JSON selected for the system will parse and
     * publish their VPD over Dbus.
     */
    void processAllFRU();

  private:
    /**
     * @brief An API to set appropriate device tree and JSON.
     *
     * This API based on system chooses corresponding device tree and JSON.
     * If device tree change is required, it updates the "fitconfig" and reboots
     * the system. Else it is NOOP.
     *
     * Note: In case of any error, exception is thrown. Caller need to handle.
     */
    void setDeviceTreeAndJson();

    /**
     * @brief API to select system specific JSON.
     *
     * The api based on the IM value of VPD, will select appropriate JSON for
     * the system. In case no system is found corresponding to the extracted IM
     * value, error will be logged.
     *
     * @param[out] systemJson - System JSON name.
     * @param[in] parsedVpdMap - Parsed VPD map.
     */
    void getSystemJson(std::string& systemJson,
                       const types::VPDMapVariant& parsedVpdMap);

    /**
     * @brief An API to read IM value from VPD.
     *
     * Note: Throws exception in case of error. Caller need to handle.
     *
     * @param[in] parsedVpd - Parsed VPD.
     */
    std::string getIMValue(const types::IPZVpdMap& parsedVpd) const;

    /**
     * @brief An API to read HW version from VPD.
     *
     * Note: Throws exception in case of error. Caller need to handle.
     *
     * @param[in] parsedVpd - Parsed VPD.
     */
    std::string getHWVersion(const types::IPZVpdMap& parsedVpd) const;

    /**
     * @brief An API to parse given VPD file path.
     *
     * @param[in] vpdFilePath - EEPROM file path.
     * @param[out] parsedVpd - Parsed VPD as a map.
     */
    void fillVPDMap(const std::string& vpdFilePath,
                    types::VPDMapVariant& parsedVpd);

    /**
     * @brief AN API to populate DBus interfaces for a FRU.
     *
     * @param[in] parsedVpdMap - Parsed VPD as a map.
     * @param[out] objectInterfaceMap - Object and its interfaces map.
     * @param[in] vpdFilePath - EEPROM path of FRU.
     */
    void populateDbus(const types::VPDMapVariant& parsedVpdMap,
                      types::ObjectMap& objectInterfaceMap,
                      const std::string& vpdFilePath);

    /**
     * @brief An API to parse and publish system VPD on D-Bus.
     *
     * Note: Throws exception in case of invalid VPD format.
     *
     * @param[in] parsedVpdMap - Parsed VPD as a map.
     */
    void publishSystemVPD(const types::VPDMapVariant& parsedVpdMap);

    /**
     * @brief An API to process extrainterfaces w.r.t a FRU.
     *
     * @param[in] singleFru - JSON block for a single FRU.
     * @param[out] interfaces - Map to hold interface along with its properties.
     * @param[in] parsedVpdMap - Parsed VPD as a map.
     */
    void processExtraInterfaces(const nlohmann::json& singleFru,
                                types::InterfaceMap& interfaces,
                                const types::VPDMapVariant& parsedVpdMap);

    /**
     * @brief An API to process embedded and synthesized FRUs.
     *
     * @param[in] singleFru - FRU to be processed.
     * @param[out] interfaces - Map to hold interface along with its properties.
     */
    void processEmbeddedAndSynthesizedFrus(const nlohmann::json& singleFru,
                                           types::InterfaceMap& interfaces);

    /**
     * @brief An API to read process FRU based in CCIN.
     *
     * For some FRUs VPD can be processed only if the FRU has some specific
     * value for CCIN. In case the value is not from that set, VPD for those
     * FRUs can't be processed.
     *
     * @param[in] singleFru - Fru whose CCIN value needs to be matched.
     * @param[in] parsedVpdMap - Parsed VPD map.
     */
    bool processFruWithCCIN(const nlohmann::json& singleFru,
                            const types::VPDMapVariant& parsedVpdMap);

    /**
     * @brief API to process json's inherit flag.
     *
     * Inherit flag denotes that some property in the child FRU needs to be
     * inherited from parent FRU.
     *
     * @param[in] parsedVpdMap - Parsed VPD as a map.
     * @param[out] interfaces - Map to hold interface along with its properties.
     */
    void processInheritFlag(const types::VPDMapVariant& parsedVpdMap,
                            types::InterfaceMap& interfaces);

    /**
     * @brief API to process json's "copyRecord" flag.
     *
     * copyRecord flag denotes if some record data needs to be copies in the
     * given FRU.
     *
     * @param[in] singleFru - FRU being processed.
     * @param[in] parsedVpdMap - Parsed VPD as a map.
     * @param[out] interfaces - Map to hold interface along with its properties.
     */
    void processCopyRecordFlag(const nlohmann::json& singleFru,
                               const types::VPDMapVariant& parsedVpdMap,
                               types::InterfaceMap& interfaces);

    /**
     * @brief An API to populate IPZ VPD property map.
     *
     * @param[out] interfacePropMap - Map of interface and properties under it.
     * @param[in] keyordValueMap - Keyword value map of IPZ VPD.
     * @param[in] interfaceName - Name of the interface.
     */
    void populateIPZVPDpropertyMap(types::InterfaceMap& interfacePropMap,
                                   const types::IPZKwdValueMap& keyordValueMap,
                                   const std::string& interfaceName);

    /**
     * @brief An API to populate Kwd VPD property map.
     *
     * @param[in] keyordValueMap - Keyword value map of Kwd VPD.
     * @param[out] interfaceMap - interface and property,value under it.
     */
    void populateKwdVPDpropertyMap(const types::KeywordVpdMap& keyordVPDMap,
                                   types::InterfaceMap& interfaceMap);

    /**
     * @brief API to populate all required interface for a FRU.
     *
     * @param[in] interfaceJson - JSON containing interfaces to be populated.
     * @param[out] interfaceMap - Map to hold populated interfaces.
     * @param[in] parsedVpdMap - Parsed VPD as a map.
     */
    void populateInterfaces(const nlohmann::json& interfaceJson,
                            types::InterfaceMap& interfaceMap,
                            const types::VPDMapVariant& parsedVpdMap);

    /**
     * @brief Helper function to insert or merge in map.
     *
     * This method checks in the given inventory::InterfaceMap if the given
     * interface key is existing or not. If the interface key already exists,
     * given property map is inserted into it. If the key does'nt exist then
     * given interface and property map pair is newly created. If the property
     * present in propertymap already exist in the InterfaceMap, then the new
     * property value is ignored.
     *
     * @param[in,out] interfaceMap - map object of type inventory::InterfaceMap
     * only.
     * @param[in] interface - Interface name.
     * @param[in] property - new property map that needs to be emplaced.
     */
    void insertOrMerge(types::InterfaceMap& interfaceMap,
                       const std::string& interface,
                       types::PropertyMap&& property);

    /**
     * @brief Check if the given CPU is an IO only chip.
     *
     * The CPU is termed as IO, whose all of the cores are bad and can never be
     * used. Those CPU chips can be used for IO purpose like connecting PCIe
     * devices etc., The CPU whose every cores are bad, can be identified from
     * the CP00 record's PG keyword, only if all of the 8 EQs' value equals
     * 0xE7F9FF. (1EQ has 4 cores grouped together by sharing its cache memory.)
     *
     * @param [in] pgKeyword - PG Keyword of CPU.
     * @return true if the given cpu is an IO, false otherwise.
     */
    bool isCPUIOGoodOnly(const std::string& pgKeyword);

    /**
     * @brief API to prime inventory Objects.
     *
     * @param[in] ipzVpdMap - IPZ VPD parsed map.
     * @param[out] primeObjects - OBject map to hold primed objects.
     */
    void primeInventory(const types::IPZVpdMap& ipzVpdMap,
                        types::ObjectMap primeObjects);

    /**
     * @brief An API to check if system VPD is already published.
     */
    bool isSystemVPDOnDBus() const;

    // Parsed JSON file.
    nlohmann::json m_parsedJson{};

    // Hold if symlink is present or not.
    bool m_isSymlinkPresent = false;
};
} // namespace vpd