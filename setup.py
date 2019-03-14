#! /usr/bin/python3
# This script is used for the installation and removal of the rootkit

import os
import sys
# import argparse

#######
# Prompting/Printing Helpers
######

colors = {
	"RED": "\033[31m",
	"GREEN": "\033[32m",
	"BLUE": "\033[34m",
}

style = {
	"BOLD": "\033[1m",
	"RESET": "\033[0m",
}


def print_success(text):
	print(colors["GREEN"], style["BOLD"], text, style["RESET"])


def print_error(text):
	print(colors["RED"], style["BOLD"], "ERROR:", text, style["RESET"])


def print_status(text):
	print(colors["BLUE"], style["BOLD"], text, style["RESET"])


def prompt_yes_no(question):
	"""
	Prints question and waits for Y/N. Loops on invalid response.
	Returns True if Yes, False if No..
	"""
	print_status(question, "[Y / N]")
	valid_response = False
	while not valid_response:
		response = input().strip().lower()
		if response == "y":
			return True
		elif response == "n":
			return False
		else:
			print_error("Invalid response. Enter either Y or N. No other letters are valid.")


def prompt(text, default=""):
	"""
	Prompt with the option to leave the default.
	"""
	print_status(text, "(Default: {})".format(default))
	response = input().strip()
	if response == "":
		return default
	else:
		return response


def create_config_header_file(user_defines: dict):
	"""
	Creates config.h file and populates it with user defined constants.
	:param: user_defines: dict
	"""
	contents = "#ifndef _CONFIG_H\n#define _CONFIG_H\n"
	for key, val in user_defines.items():
		contents += "#define {} {}".format(key, val)
	contents += "#endif"

	with open("config.h", "w") as f:
		f.write(contents)


def validate_os_and_kernel():
	"""
	Checks if the rootkit is compatible with the OS and kernel.
	Exits if it's incompatible. Returns kernel version as string if
	everything is okay.
	"""
	os_info = os.uname()
	if os_info[0].lower() != "linux":
		print_error("Not on linux. Exiting.")
		sys.exit(1)

	kernel_version = os_info[2]
	valid_kernels = ["4.18.0-16-generic", "4.18.0-15-generic"]
	if kernel_version not in valid_kernels:
		print_error("Invalid kernel. Exiting.")
		sys.exit(1)

	return kernel_version


def install(kernel_version):
	print_status("Starting rootkit installation...")

	config = {}
	config["MODULE_NAME"] = "garden"
	config["DRIVER_NAME"] = prompt("Enter the name of a kernel driver to disguise your rootkit.", "garden")
	# config["HIDDEN_FILE_PREFIX"] = prompt("Enter prefix for files to hide.", "Garden")
	# config["REVERSE_SHELL_IP_ADDR"] = prompt("Enter IP address for reverse shell.")

	module_dir = "/lib/modules/{0}/kernel/drivers/{1}".format(kernel_version, config["DRIVER_NAME"])
	if not os.path.exists(module_dir):
		os.mkdir(module_dir)

	print_status("Creating config.h file...")
	create_config_header_file(config)
	print_success("config.h created.")

	print_status("Compiling rootkit...")

	print_success("Successful compilation.")

	print_success("Successful installation.")
	pass


def remove():
    pass


def main():
	kernel_version = validate_os_and_kernel()

	# TODO: Set up argument parser.
	# parser = argparse.ArgumentParser(description='Rootkit setup.')
	# parser.add_argument("--install", help="install rootkit")
	# args = parser.parse_args()

	install(kernel_version)
	pass


if __name__ == "__main__":
    main()
