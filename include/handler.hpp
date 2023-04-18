#pragma once

#include "types.hpp"

#include <nlohmann/json.hpp>
#include <variant>
#include <string_view>

namespace vpd
{
/**
 * @brief A class to handle VPD data.
 *
 * The class exposes API to get parsed VPD data map, process FRUs VPD by calling
 * respective parser, arranging them under specific interfaces and publish those
 * VPD data over D-Bus.
 *
 * The class may also implement helper functions required for VPD handling.
 */
class Handler
{
  public:
    /**
     * List of deleted functions.
     */
    Handler(const Handler&);
    Handler& operator=(const Handler&);
    Handler(Handler&&) = delete;

    /**
     * @brief Constructor.
     *
     * @param[in] json - Parsed inventory JSON.
     */
    Handler(const nlohmann::json& json) : m_inventoryJson(json)
    {
    }

    /**
     * @brief Destructor
     */
    ~Handler() = default;

    /**
     * @brief An API to process VPD.
     *
     * This api is exposed to process VPD map recieved as a parameter and call
     * respective API to populate VPD under interfaces and publish them on
     * D-Bus.
     *
     * @param[in] vpdFilePath - EEPROM path of the FRU whose data needs to be
     * handled.
     * @param[in] vpdMap - Map of parsed VPD data.
     */
    void processAndPublishVPD(
        std::string_view vpdFilePath,
        std::variant<types::ParsedVPD, types::KeywordVpdMap>& vpdMap);

    /**
     * @brief An API to get parsed VPD map.
     *
     * The api can be called to get map of parsed VPD data in the format as
     * defined by the type returned.
     *
     * Note: Empty map is returned in case of any error. Caller responsibility
     * to handle accordingly.
     *
     * @param[in] vpdFilePath - EEPROM path of FRU whose data needs to be
     * parsed.
     * @param[out] vpdMap - Parsed VPD map.
     */
    void
        getVPDMap(std::string_view vpdFilePath,
                  std::variant<types::ParsedVPD, types::KeywordVpdMap>& vpdMap);

  private:
    /**
     * @brief API to get VPD data in a uint8_t vector.
     *
     * Some FRU has entry for offset from where VPD data starts. This api reads
     * the VPD data into a uint8_t vector, corresponding to offset w.r.t that
     * FRU.
     *
     * Note: Caller responsibility to check for validity of the VPD vector.
     *
     * @param[in] vpdFilePath - EEPROM path of FRU.
     * @param[out] vpdVector - VPD data, read into an uint8_t vector.
     * @param[out] vpdStartOffset - Start offset of VPD as read from JSON,
     * defaulted to zero otherwise.
     */
    void getVpdDataInVector(const std::string& vpdFilePath,
                            types::BinaryVector& vpdVector,
                            uint32_t& vpdStartOffset);

    // Object to hold parsed inventory JSON.
    const nlohmann::json& m_inventoryJson;
}; // class Handler
} // namespace vpd