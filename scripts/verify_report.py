#!/usr/bin/etc pyhton3

import json
import os

from constants import *
from generate import *


def verify_invalid_eeproms():
    os.system("peltool -afh > /tmp/pel_output.json")

    with open("/tmp/pel_output.json") as fs:
        pel_data = json.load(fs)

    os.system("rm -rf /tmp/pel_output.json")

    with open(VERIFY_FILE_PATH) as file:
        verify_data = json.load(file)

    for path in verify_data:
        if path["type"] == "invalid_eeprom":
            for pel in pel_data:
                if (
                    "User Data 1" in pel
                    and "DESCRIPTION" in pel["User Data 1"]
                    and path["file_path"] in pel["User Data 1"]["DESCRIPTION"]
                ):
                    path["pel_id"] = pel["Private Header"]["Platform Log Id"]
                    path["status"] = "verify_completed"
                    break

            if "pel_id" not in path:
                path["status"] = "verify_failed"

    update_verify_info(verify_data)


verify_invalid_eeproms()
