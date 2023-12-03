#!/bin/python3
import subprocess

for build_type in ["Release", "Debug", "RelWithDebInfo"]:
    subprocess.call(
        f"pipenv run conan install . -s build_type={build_type} --build=missing",
        shell=True,
    )
