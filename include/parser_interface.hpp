#pragma once

#include "types.hpp"

#include <variant>

namespace vpd
{
/**
 * @brief Interface class for parsers.
 *
 * Any concrete parser class, implementing parsing logic need to derive from
 * this interface class and override the parse method declared in this class.
 */
class ParserInterface
{
  public:
    /**
     * @brief Pure virtual function for parsing VPD file.
     *
     * The API needs to be overridden by all the classes deriving from parser
     * inerface class to implement respective VPD parsing logic.
     *
     * @return parsed format for VPD data, depending upon the
     * parsing logic.
     */
    virtual types::VPDMapVariant parse() = 0;

    /**
     * @brief Read keyword's value from hardware
     *
     * @param[in] types::ReadVpdParams - Parameters required to perform read.
     *
     * @return keyword's value on successful read. Exception on failure.
     */
    virtual types::DbusVariantType
        readKeywordFromHardware(const types::ReadVpdParams)
    {
        return types::DbusVariantType();
    }

    /**
     * @brief Virtual destructor.
     */
    virtual ~ParserInterface() {}
};
} // namespace vpd
