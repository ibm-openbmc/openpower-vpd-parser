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

    return sub_fru
    # return {eeprom_path: [sub_fru]}


def get_verify_info(eeprom, inv_path):
    return {
        "eeprom_path": eeprom,
        "inventory_path": inv_path,
        "type": "valid_eeprom",
        "status": "verify_not_started",
    }


def create_valid_eeproms(valid_eeprom_count):
    frus_cfg = {}
    verify_info = []

    for i in range(0, valid_eeprom_count):
        eeprom_path = f"{EEPROM_BASE_PATH}/200-{i:04x}/eeprom"
        os.makedirs(os.path.dirname(eeprom_path), exist_ok=True)
        shutil.copy(VALID_EEPROM, eeprom_path)

        fru = get_fru_config(i)
        frus_cfg.update({eeprom_path: [fru]})
        # verify_info.append(get_verify_info(next(iter(fru)), fru[0]["inventoryPath"]))
        verify_info.append(get_verify_info(eeprom_path, fru["inventoryPath"]))

    print(json.dumps(verify_info))

    return (frus_cfg, verify_info)


def main():
    valid_eeprom_count = 10

    if len(sys.argv) > 1:
        valid_eeprom_count = int(sys.argv[1])

    create_valid_eeproms(valid_eeprom_count)


if __name__ == "__main__":
    main()
