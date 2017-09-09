#include "conf.h"
#include "datacore.h"
#include <liby/plugin.h>
#include <glog/logging.h>

#include <signal.h>
#include <dirent.h>

/*
 * plugins
 */

static void plugins_init(const char *plugin_dir)
{
	DIR *dir = opendir(plugin_dir);
	if (dir == NULL) {
		perror("opendir");
		return;
	}

	struct dirent *entry;
	char bufffer[512];
	while ((entry = readdir(dir))) {
		if (strncmp(entry->d_name, ".", 2) == 0 ||
		    strncmp(entry->d_name, "..", 3) == 0 ||
		    (entry->d_type != DT_REG && entry->d_type != DT_LNK))
			continue;

		snprintf(bufffer, sizeof(bufffer),
			 "%s/%s", plugin_dir, entry->d_name);
		load_plugin(bufffer);
	}
	closedir(dir);
}

static void plugins_fini()
{
	for (struct list_head *p = plugin_head.next; p != &plugin_head;) {
		struct plugin *tmp = container_of(p, struct plugin, node);
		p = p->next;
		unload_plugin(tmp);
	}
}

/*
 * main
 */

static void signal_handler(int sig)
{
	datacore_fini();
	plugins_fini();
	google::ShutdownGoogleLogging();
	exit(0);
}

int main(int argc, char *argv[])
{
	// init
	conf_from_file(&conf, "./kong.xml");

	FLAGS_log_dir = "./log";
	FLAGS_minloglevel = google::INFO;
	FLAGS_stderrthreshold = google::INFO;
	FLAGS_colorlogtostderr = true;
	google::InitGoogleLogging(argv[0]);

	signal(SIGINT, signal_handler);
	plugins_init("./plugins/");
	datacore_init();

	// sleep
	while (1) sleep(10);

	// fini
	datacore_fini();
	plugins_fini();
	google::ShutdownGoogleLogging();
	return 0;
}
