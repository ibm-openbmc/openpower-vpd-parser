#pragma once

#include "logger.hpp"
#include "parser_interface.hpp"
#include "types.hpp"

namespace vpd
{
/**
 * @brief Concrete class to implement DDIMM VPD parsing.
 *
 * The class inherits ParserInterface interface class and overrides the parser
 * functionality to implement parsing logic for DDIMM VPD format.
 */
class DdimmVpdParser : public ParserInterface
{
  public:
    // Deleted API's
    DdimmVpdParser() = delete;
    DdimmVpdParser(const DdimmVpdParser&) = delete;
    DdimmVpdParser& operator=(const DdimmVpdParser&) = delete;
    DdimmVpdParser(DdimmVpdParser&&) = delete;
    DdimmVpdParser& operator=(DdimmVpdParser&&) = delete;

    /**
     * @brief Defaul destructor.
     */
    ~DdimmVpdParser() = default;

    /**
     * @brief Constructor
     *
     * @param[in] vpdVector - VPD data.
     */
    DdimmVpdParser(const types::BinaryVector& vpdVector) : m_vpdVector(vpdVector) {}

    /**
     * @brief Api to parse DDIMM VPD file.
     *
     * @return parsed VPD data
     */
    virtual types::VPDMapVariant parse() override;

  private:
    /**
     * @brief Api to read keyword data based on the type DDR4/DDR5.
     *
     * @return keyword data, empty otherwise.
     */
    types::DdimmVpdMap readKeywords(types::BinaryVector::const_iterator iterator);

    /**
     * @brief Api to calculate dimm size from DDIMM VPD
     *
     * @param[in] iterator - iterator to buffer containing VPD
     * @return calculated data or 0 in case of any error.
     */
    size_t getDDimmSize(types::BinaryVector::const_iterator iterator);

    /**
     * @brief This function calculates DDR5 based DDIMM's capacity
     *
     * @param[in] iterator - iterator to buffer containing VPD
     * @return calculated data or 0 in case of any error.
     */
    auto getDdr5BasedDDimmSize(types::BinaryVector::const_iterator iterator);

    /**
     * @brief This function calculates DDR5 based die per package
     *
     * @param[in] l_ByteValue - the bit value for calculation
     * @return die per package value.
     */
    uint8_t getDDR5DiePerPackage(uint8_t l_ByteValue);

    /**
     * @brief This function calculates DDR5 based density per die
     *
     * @param[in] l_ByteValue - the bit value for calculation
     * @return density per die.
     */
    uint8_t getDDR5DensityPerDie(uint8_t l_ByteValue);

    /**
     * @brief This function checks the validity of the bits
     *
     * @param[in] l_ByteValue - the byte value with relevant bits
     * @param[in] shift - shifter value to selects needed bits
     * @param[in] minValue - minimum value it can contain
     * @param[in] maxValue - maximum value it can contain
     * @return true if valid else false.
     */
    bool checkValidValue(uint8_t l_ByteValue, uint8_t shift, uint8_t minValue,
                         uint8_t maxValue);

    // vdp file to be parsed
    const types::BinaryVector& m_vpdVector;
};
} // namespace vpd
