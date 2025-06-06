#!/usr/bin/env python3

import random
import shutil
import sys
import json

from generate_corrupted_eeproms import *
from generate_valid_eeproms import *

def update_verify_info(verify_data):
    with open(VERIFY_FILE_PATH, "w") as f:
        json.dump(verify_data, f, indent=4)


def add_frus_to_json(fru_details, file_path):
    with open(file_path, "r") as file:
        file_data = json.load(file)

        file_data["frus"].update(fru_details)

        file.seek(0)
        with open(file_path, "w") as f:
            json.dump(file_data, f, indent=4)


def main():
    fru_data = {}

    # default IM value
    im_value = "50001000_v2"

    if len(sys.argv) > 1:
        print("IM value is set to: ", sys.argv[1])
        im_value = sys.argv[1]

    system_config_json_path = (
        f"{SYSTEM_CONFIG_JSON_PATH_BASE}{"/"}{im_value}{".json"}"
    )

    print("System Config JSON path: ", system_config_json_path)

    # Take a backup of config file
    if os.path.exists(system_config_json_path + "_bkp"):
        shutil.copy(system_config_json_path+ "_bkp", system_config_json_path)
    else:
        shutil.copy(system_config_json_path, system_config_json_path + "_bkp")

    valid_frus_map, valid_verify_data = create_valid_eeproms(
        random.randrange(50)
    )
    invalid_frus_map, invalid_verify_data = createInvalidEeproms()

    fru_data.update(valid_frus_map)
    fru_data.update(invalid_frus_map)

    add_frus_to_json(fru_data, system_config_json_path)
    update_verify_info(valid_verify_data + invalid_verify_data)


if __name__ == "__main__":
    main()
