#!/usr/bin/env python3
import os
import sys


def create_usr_overlay():
    os.system("mkdir -p /tmp/persist/usr")
    os.system("mkdir -p /tmp/persist/work/usr")
    os.system(
        "mount -t overlay -o lowerdir=/usr,upperdir=/tmp/persist/usr,workdir=/tmp/persist/work/usr overlay /usr"
    )


def main():
    print("SR:", sys.argv[0])

    if len(sys.argv) > 1:
        print("Args:", sys.argv[1:])

    create_usr_overlay()
    os.system("/usr/bin/generate.py " + " ".join(sys.argv[1:]))


if __name__ == "__main__":
    main()
