#!/usr/bin/env python3

import copy
import json
import os
import shutil
from constants import *

def get_fru_config(device_id):
    sub_fru = copy.deepcopy(FRU_INVENTORY_DATA)
    eeprom_path = f"{EEPROM_BASE_PATH}/200-{device_id:04x}/eeprom"
    sub_fru["inventoryPath"] = f"{BASE_INVENTORY_PATH}/valid_fru_{device_id}"
    sub_fru["extraInterfaces"]["xyz.openbmc_project.Inventory.Item"][
        "PrettyName"
    ] = f"Test FRU {device_id}"

    return {eeprom_path: [sub_fru]}


def get_verify_info(eeprom, field_to_check):
    return (eeprom, "valid_eeprom", field_to_check, "verify_not_started")


def create_valid_eeproms(valid_eeprom_count):
    valid_frus = {}
    valid_eeprom_info = []

    for i in range(0, valid_eeprom_count):
        eeprom_path = f"{EEPROM_BASE_PATH}/200-{i:04x}/eeprom"
        os.makedirs(os.path.dirname(eeprom_path), exist_ok=True)
        shutil.copy(VALID_EEPROM, eeprom_path)

        fru = get_fru_config(i)
        valid_frus.update(fru)
        valid_eeprom_info.append(get_verify_info(next(iter(fru)), "CollectionStatus"))

    # print(json.dumps(valid_frus))
    print(json.dumps(valid_eeprom_info))

    return (valid_frus, valid_eeprom_info)


def main():
    valid_eeprom_count = 10

    if len(sys.argv) > 1:
        valid_eeprom_count = int(sys.argv[1])

    create_valid_eeproms(valid_eeprom_count)


if __name__ == "__main__":
    main()
