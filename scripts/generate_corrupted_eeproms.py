#!/usr/bin/env python3

import copy
import os
import shutil
from constants import *
import json

#from generate_valid_eeproms import *

def createWrongGpioFru(src_file_path):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, "gpioPinNotFound", "eeprom")
    #os.makedirs(os.path.dirname(dest_file_path), exist_ok=True)
    #shutil.copy(src_file_path, dest_file_path)
    fru = get_fru_config("gpioPinNotFound")
    fru[dest_file_path][0].update(copy.deepcopy(PREACTION_DATA))
    verify_info = get_verify_info(dest_file_path, "GPIO_ERROR")

    return (fru, verify_info)

def createCorruptedFiles(src_file_path, dest_name, offset, new_bytes):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, dest_name, "eeprom")
    os.makedirs(os.path.dirname(dest_file_path), exist_ok=True)
    shutil.copy(src_file_path, dest_file_path)

    fs = open(dest_file_path, "rb+")
    fs.seek(offset)
    fs.write(new_bytes)

    fs.close()


def truncateFile(src_file_path, dest_name, offset):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, dest_name, "eeprom")
    os.makedirs(os.path.dirname(dest_file_path), exist_ok=True)
    shutil.copy(src_file_path, dest_file_path)

    with open(dest_file_path, "rb+") as fs:
        fs.truncate(offset)


def emptyFile(dest_name):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, dest_name, "eeprom")
    os.makedirs(os.path.dirname(dest_file_path), exist_ok=True)

    with open(dest_file_path, "wb+") as fs:
        pass


def get_fru_config(name):
    sub_fru = copy.deepcopy(FRU_INVENTORY_DATA)
    eeprom_path = f"{EEPROM_BASE_PATH}/{name}/eeprom"
    sub_fru["inventoryPath"] = f"{BASE_INVENTORY_PATH}/invalid_fru_{name}"
    sub_fru["extraInterfaces"]["xyz.openbmc_project.Inventory.Item"][
        "PrettyName"
    ] = f"Test FRU {name}"

    return {eeprom_path: [sub_fru]}


def get_verify_info(eeprom, field_to_check):
    return (eeprom, "invalid_eeprom", field_to_check, "verify_not_started")


def createInvalidEeproms():
    invalid_frus = {}
    invalid_eeprom_info = []

    truncateFile(ipzSrcFilePath, "truncated", 40)
    fru = get_fru_config("truncated")
    invalid_frus.update(fru)
    invalid_eeprom_info.append(get_verify_info(next(iter(fru)), "ERROR"))

    emptyFile("emptyFile")
    fru = get_fru_config("emptyFile")
    invalid_frus.update(fru)
    invalid_eeprom_info.append(get_verify_info(next(iter(fru)), "ERROR"))

    gpio_fru, gpio_verify_info = createWrongGpioFru(ipzSrcFilePath)
    invalid_frus.update(gpio_fru)
    invalid_eeprom_info.append(gpio_verify_info)

    for file_name, eeprom_path, offset, bytes, error_type in corrupt_format:
        createCorruptedFiles(eeprom_path, file_name, offset, bytes)
        fru = get_fru_config(file_name)
        invalid_frus.update(fru)
        invalid_eeprom_info.append(
            get_verify_info(next(iter(fru)), error_type)
        )

    # print(json.dumps(invalid_frus))
    print(json.dumps(invalid_eeprom_info))

    return (invalid_frus, invalid_eeprom_info)


if __name__ == "__main__":
    createInvalidEeproms()
