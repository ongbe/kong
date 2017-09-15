#include "console.h"
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

struct console {
	int state;
	char prompt[32];
	struct list_head cmd_head;
};

static struct console console;

static char* on_rl_compentry(const char *text, int state)
{
	static struct list_head *l;

	if (!state)
		l = console.cmd_head.next;
	else
		l = l->next;

	for (; l != &console.cmd_head; l = l->next) {
		struct command *cmd = container_of(l, struct command, node);
		if (strncmp(cmd->name, text, strlen(text)) == 0)
			return strdup(cmd->name);
	}

	return NULL;
}

static char** on_rl_attempted_completion(const char *text, int start, int end)
{
	rl_attempted_completion_over = 1;
	rl_completer_quote_characters = "\"'";
	return rl_completion_matches(text, on_rl_compentry);
}

static void process_command(const char *line)
{
	if (strlen(line) == 0)
		return;

	for (struct list_head *l = console.cmd_head.next; l != &console.cmd_head; l = l->next) {
		struct command *cmd = container_of(l, struct command, node);
		if (strncmp(line, cmd->name, strlen(cmd->name)) == 0) {
			cmd->func(line);
			return;
		}
	}

	printf("%s: command not found\n", line);
}

void on_cmd_help(const char *line)
{
	for (struct list_head *l = console.cmd_head.next; l != &console.cmd_head; l = l->next) {
		struct command *cmd = container_of(l, struct command, node);
		printf(" %-10s\t%s\n", cmd->name, cmd->description);
	}
}

void on_cmd_history(const char *line)
{
	HISTORY_STATE *state = history_get_history_state();
	int index = 0;
	for (HIST_ENTRY **item = state->entries; item != state->entries + state->length; item++)
		printf(" %d %s\n", index++, (*item)->line);
}

void on_cmd_history_exec(const char *line)
{
	int index = -1;
	sscanf(line, "!%d", &index);
	if (index == -1) {
		printf("%s: event not found\n", line);
		return;
	}

	HISTORY_STATE *state = history_get_history_state();
	if (index >= state->length) {
		printf("%s: event not found\n", line);
		return;
	}

	char *real_line = (*(state->entries+index))->line;
	remove_history(state->length-1);
	add_history(real_line);

	process_command(real_line);
}

void console_run()
{
	while (console.state) {
		char *line = readline(console.prompt);

		if (line == NULL)
			break;

		if (!strlen(line))
			continue;

		add_history(line);
		process_command(line);
	}
}

void console_stop()
{
	console.state = 0;
}

void console_add_command(struct command *cmd)
{
	INIT_LIST_HEAD(&cmd->node);
	list_add(&cmd->node, &console.cmd_head);
}

void console_del_command(struct command *cmd)
{
	list_del(&cmd->node);
}

void console_init(const char *prompt)
{
	snprintf(console.prompt, sizeof(console.prompt), "%s", prompt);
	console.state = 1;
	INIT_LIST_HEAD(&console.cmd_head);

	rl_attempted_completion_function = on_rl_attempted_completion;
}
