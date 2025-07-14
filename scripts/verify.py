import json
import os
import re
import subprocess
import sys

from constants import *

# Property name you want to filter
COLLECTION_PROPERTY_NAME = "CollectionStatus"

# Start dbus-monitor command
COLLECTION_MONITOR_CMD = [
    "dbus-monitor",
    "--system",
    "path='/com/ibm/VPD/Manager',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',sender='com.ibm.VPD.Manager'",
]


def watchOnCollectionStatus():
    property_value = ""
    print(
        f"🔍 Watching for '{COLLECTION_PROPERTY_NAME}' property changes...\n"
    )

    with subprocess.Popen(
        COLLECTION_MONITOR_CMD,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    ) as proc:
        try:
            capture = False
            buffer = []

            for line in proc.stdout:
                line = line.strip()

                if "signal" in line and "PropertiesChanged" in line:
                    # Start of a new signal
                    capture = True
                    buffer = [line]
                    continue

                elif capture:
                    buffer.append(line)

                    if line == "]":  # End of the array
                        full_signal = "\n".join(buffer)
                        capture = False

                        if COLLECTION_PROPERTY_NAME in full_signal:
                            match = re.search(
                                r'variant\s+string\s+"([^"]+)"', full_signal
                            )

                            if match and match.group(1) in "Completed":
                                property_value = match.group(1)
                                print(
                                    f"\n✅ Detected change in '{COLLECTION_PROPERTY_NAME}, value:{property_value}'\n"
                                )

                                proc.terminate()
                                proc.wait(timeout=2)
                                break
                            else:
                                print(
                                    f"\n✅ Detected change in '{COLLECTION_PROPERTY_NAME}, {match}'\n"
                                )

                        capture = False

        except KeyboardInterrupt:
            print("\n🛑 Exiting.")
            proc.terminate()

        return property_value


def get_dbus_property(service, path, interface, property_name):
    try:
        cmd = [
            "busctl",
            "get-property",
            service,
            path,
            interface,
            property_name,
        ]

        result = subprocess.run(
            cmd, capture_output=True, text=True, check=True
        )

        output = result.stdout.strip()

        if output.startswith("s "):
            value = output.split(" ", 1)[1].strip().strip('"')
            return value
        else:
            return output

    except subprocess.CalledProcessError as e:
        print("get-property command failed: ", e)
    return None


def update_eeprom_status():
    pel_data = get_pel_dump()

    with open(VERIFY_FILE_PATH, "r") as file:
        verify_report = json.load(file)

    for item in verify_report:
        if item["type"] == "valid_eeprom":
            value = get_dbus_property(
                "xyz.openbmc_project.Inventory.Manager",
                item["inventory_path"],
                "com.ibm.VPD.Collection",
                "CollectionStatus",
            )
            if value == "com.ibm.VPD.Collection.Status.Success":
                item["status"] = "verify_success"
            else:
                item["status"] = "verify_failed"
        elif item["type"] == "invalid_eeprom":
            for pel in pel_data:
                if (
                    "User Data 1" in pel
                    and "DESCRIPTION" in pel["User Data 1"]
                    and item["eeprom_path"]
                    in pel["User Data 1"]["DESCRIPTION"]
                ):
                    item["pel_id"] = pel["Private Header"]["Platform Log Id"]
                    item["status"] = "verify_success"
                    break
            if "pel_id" not in item:
                item["status"] = "verify_failed"

    with open(VERIFY_FILE_PATH, "w") as file:
        json.dump(verify_report, file, indent=4)

    print("Updated JSON:", verify_report)


def get_pel_dump():
    os.system("peltool -afh > " + PEL_DUMP_PATH)

    with open(PEL_DUMP_PATH) as fs:
        pel_data = json.load(fs)

    return pel_data


if __name__ == "__main__":
    # watchOnCollectionStatus()
    update_eeprom_status()

    print("done..")
