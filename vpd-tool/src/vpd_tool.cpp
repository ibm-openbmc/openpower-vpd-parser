#include "vpd_tool.hpp"

#include "tool_constants.hpp"
#include "tool_types.hpp"
#include "tool_utils.hpp"

#include <iostream>

namespace vpd
{
int VpdTool::readKeyword(const std::string& i_vpdPath,
                         const std::string& i_recordName,
                         const std::string& i_keywordName,
                         const bool i_onHardware,
                         const std::string& i_fileToSave)
{
    int l_rc = constants::FAILURE;
    try
    {
        types::DbusVariantType l_keywordValue;
        if (i_onHardware)
        {
            l_keywordValue = utils::readKeywordFromHardware(
                i_vpdPath, std::make_tuple(i_recordName, i_keywordName));
        }
        else
        {
            std::string l_inventoryObjectPath(constants::baseInventoryPath +
                                              i_vpdPath);

            l_keywordValue = utils::readDbusProperty(
                constants::inventoryManagerService, l_inventoryObjectPath,
                constants::ipzVpdInfPrefix + i_recordName, i_keywordName);
        }

        if (const auto l_value =
                std::get_if<types::BinaryVector>(&l_keywordValue);
            l_value && !l_value->empty())
        {
            // ToDo: Print value in both ASCII and hex formats
            const std::string& l_keywordStrValue =
                utils::getPrintableValue(*l_value);

            if (i_fileToSave.empty())
            {
                utils::displayOnConsole(i_vpdPath, i_keywordName,
                                        l_keywordStrValue);
                l_rc = constants::SUCCESS;
            }
            else
            {
                if (utils::saveToFile(i_fileToSave, l_keywordStrValue))
                {
                    std::cout
                        << "Value read is saved on the file: " << i_fileToSave
                        << std::endl;
                    l_rc = constants::SUCCESS;
                }
                else
                {
                    std::cerr
                        << "Error while saving the read value on the file: "
                        << i_fileToSave
                        << "\nDisplaying the read value on console"
                        << std::endl;
                    utils::displayOnConsole(i_vpdPath, i_keywordName,
                                            l_keywordStrValue);
                }
            }
        }
        else
        {
            // TODO: Enable logging when verbose is enabled.
            // std::cout << "Invalid data type or empty data received." <<
            // std::endl;
        }
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        /*std::cerr << "Read keyword's value for path: " << i_vpdPath
                  << ", Record: " << i_recordName
                  << ", Keyword: " << i_keywordName
                  << " is failed, exception: " << l_ex.what() << std::endl;*/
    }
    return l_rc;
}
} // namespace vpd
