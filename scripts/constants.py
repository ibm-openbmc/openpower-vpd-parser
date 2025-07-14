EEPROM_BASE_PATH = "/tmp/eeproms"
BASE_INVENTORY_PATH = (
    "/xyz/openbmc_project/inventory/system/chassis/motherboard"
)
SYSTEM_CONFIG_JSON_PATH_BASE = "/usr/share/vpd"
VERIFY_FILE_PATH = "/tmp/verify_report.json"

VALID_EEPROM = "/sys/bus/i2c/drivers/at24/0-0051/eeprom"
ipzSrcFilePath = "/sys/bus/i2c/drivers/at24/7-0051/eeprom"
dimmSrcFilePath = "/sys/bus/i2c/drivers/at24/110-0050/eeprom"
PEL_DUMP_PATH = "/tmp/pel_output.json"

FRU_INVENTORY_DATA = {
    "inventoryPath": "/xyz/openbmc_project/inventory/system/chassis/motherboard/test_fru1",
    "serviceName": "xyz.openbmc_project.Inventory.Manager",
    "extraInterfaces": {
        "com.ibm.ipzvpd.Location": {"LocationCode": "Ufcs-P0-C5"},
        "xyz.openbmc_project.Inventory.Item": {"PrettyName": "Test FRU 1"},
    },
}

PREACTION_DATA = {
    "preAction": {
        "collection": {
            "gpioPresence": {"pin": "PRESENT_PIN_NOT_EXISTS_N", "value": 0}
        }
    }
}

# (error_name, eeprom_file, offset, bytes_to_update, pel_type)
corrupt_format = [
    ("missingVtoc", ipzSrcFilePath, 61, b"\x49\x56", "ERROR"),
    ("missingVhdr", ipzSrcFilePath, 17, b"\x49\x56", "ERROR"),
    ("invalidVtocEcc", ipzSrcFilePath, 74, b"\x49\x56", "ERROR"),
    ("invalidVhdrEcc", ipzSrcFilePath, 10, b"\x55", "ERROR"),
    ("invalidRecordOffset", ipzSrcFilePath, 88, b"\x00\x00", "EEPROM"),
    ("invalidRecordEccLength", ipzSrcFilePath, 94, b"\x00\x00", "EEPROM"),
    ("invalidRecordLength", ipzSrcFilePath, 90, b"\x00\x00", "EEPROM"),
    ("recordEccCheckFail", ipzSrcFilePath, 407, b"\x10\x11\x12", "EEPROM"),
    ("ecc1BitCorruption", ipzSrcFilePath, 127, b"\x4d", "EEPROM"),
    ("invalidDdrType", dimmSrcFilePath, 2, b"\x49", "ERROR"),
    ("invalidDensityPerDie", dimmSrcFilePath, 4, b"\x49\x56", "ERROR"),
    ("invalidVpdType", dimmSrcFilePath, 2, b"\x49\x56", "ERROR"),
    ("ZeroDdimmSize", dimmSrcFilePath, 235, b"\x00", "ERROR"),
]
