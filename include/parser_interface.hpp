#pragma once

#include "logger.hpp"
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
     * @brief Virtual function to perform write on VPD of any type.
     *
     * The API can be overridden by classes inheriting this ParserInterface
     * class to implement VPD write operation.
     *
     * @return -1 on failure and n if n bytes are written.
     */
    virtual int write(const types::Path, const types::VpdData)
    {
        logging::logMessage(
            "Write operation not supported for the given VPD type.");
        return 0;
    }

    /**
     * @brief Virtual destructor.
     */
    virtual ~ParserInterface() {}
};
} // namespace vpd
