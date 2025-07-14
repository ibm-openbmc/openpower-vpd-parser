#!/usr/bin/env python3

import copy
import json
import os
import shutil

from constants import *

# from generate_valid_eeproms import *


def createWrongGpioFru(src_file_path):
    dest_file_path = os.path.join(
        EEPROM_BASE_PATH, "gpioPinNotFound", "eeprom"
    )
    fru = get_fru_config("gpioPinNotFound")
    fru[dest_file_path][0].update(copy.deepcopy(PREACTION_DATA))
    verify_info = get_verify_info(
        dest_file_path, fru[dest_file_path][0]["inventoryPath"]
    )

    return (fru, verify_info)


def createCorruptedFile(src_file_path, dest_name, offset, new_bytes):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, dest_name, "eeprom")
    os.makedirs(os.path.dirname(dest_file_path), exist_ok=True)
    shutil.copy(src_file_path, dest_file_path)

    fs = open(dest_file_path, "rb+")
    fs.seek(offset)
    fs.write(new_bytes)

    fs.close()
    return dest_file_path


def truncateFile(src_file_path, dest_name, offset):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, dest_name, "eeprom")
    os.makedirs(os.path.dirname(dest_file_path), exist_ok=True)
    shutil.copy(src_file_path, dest_file_path)

    with open(dest_file_path, "rb+") as fs:
        fs.truncate(offset)

    return dest_file_path


def emptyFile(dest_name):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, dest_name, "eeprom")
    os.makedirs(os.path.dirname(dest_file_path), exist_ok=True)

    with open(dest_file_path, "wb+") as fs:
        pass

    return dest_file_path


def get_fru_config(name):
    sub_fru = copy.deepcopy(FRU_INVENTORY_DATA)
    eeprom_path = f"{EEPROM_BASE_PATH}/{name}/eeprom"
    sub_fru["inventoryPath"] = f"{BASE_INVENTORY_PATH}/invalid_fru_{name}"
    sub_fru["extraInterfaces"]["xyz.openbmc_project.Inventory.Item"][
        "PrettyName"
    ] = f"Test FRU {name}"

    return {eeprom_path: [sub_fru]}


def get_verify_info(eeprom, inv_path):
    return {
        "eeprom_path": eeprom,
        "inventory_path": inv_path,
        "type": "invalid_eeprom",
        "status": "verify_not_started",
    }


def createInvalidEeproms():
    frus_cfg = {}
    verify_info = []

    eeprom_path = truncateFile(ipzSrcFilePath, "truncated", 40)
    fru = get_fru_config("truncated")
    frus_cfg.update(fru)
    verify_info.append(
        get_verify_info(eeprom_path, fru[eeprom_path][0]["inventoryPath"])
    )

    eeprom_path = emptyFile("emptyFile")
    fru = get_fru_config("emptyFile")
    frus_cfg.update(fru)
    verify_info.append(
        get_verify_info(eeprom_path, fru[eeprom_path][0]["inventoryPath"])
    )

    gpio_fru, gpio_verify_info = createWrongGpioFru(ipzSrcFilePath)
    frus_cfg.update(gpio_fru)
    verify_info.append(gpio_verify_info)

    for file_name, eeprom_path, offset, bytes, error_type in corrupt_format:
        dst_eeprom_path = createCorruptedFile(
            eeprom_path, file_name, offset, bytes
        )
        fru = get_fru_config(file_name)
        frus_cfg.update(fru)
        verify_info.append(
            get_verify_info(
                dst_eeprom_path, fru[dst_eeprom_path][0]["inventoryPath"]
            )
        )

    print(json.dumps(verify_info))

    return (frus_cfg, verify_info)


if __name__ == "__main__":
    createInvalidEeproms()
