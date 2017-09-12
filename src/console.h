#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <liby/list.h>

#ifdef __cplusplus
extern "C" {
#endif

struct command {
	const char *name;
	void (*func)(const char* line);
	const char *description;
	struct list_head node;
};

void console_init(const char *prompt);
void console_add_command(struct command *cmd);
void console_del_command(struct command *cmd);

void console_run();
void console_stop();

void on_cmd_help(const char *line);
void on_cmd_history(const char *line);
void on_cmd_history_exec(const char *line);

#ifdef __cplusplus
}
#endif

#endif
