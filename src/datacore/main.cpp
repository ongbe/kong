#include "conf.h"
#include "packet.h"
#include "datacore.h"
#include <liby/net.h>
#include <liby/packet_parser.h>
#include <liby/plugin.h>
#include <glog/logging.h>
#include <signal.h>
#include <dirent.h>

/*
 * socket
 */

static int on_packet(struct ysock *conn)
{
	char *buffer;
	size_t len;

	while ((len = ysock_rbuf_get(conn, &buffer)) >= CMD_LEN) {
		struct packet_parser *p = find_packet_parser(CONV_W(buffer));
		if (p) {
			p->do_parse(conn, buffer, len);
			if (ysock_rbuf_get(conn, &buffer) == len)
				break;
		} else {
			LOG(WARNING) << "unknown packet: 0x"
				     << std::hex << CONV_W(buffer);
			ysock_rbuf_head(conn, len);
		}
	}

	return 0;
}

static int on_connection(struct ysock *server, struct ysock *conn)
{
	ysock_on_packet(conn, on_packet);
	return 0;
}

static int do_parse_alive(void *sess, char *buffer, size_t len)
{
	if (len >= 26) {
		ysock_rbuf_head((struct ysock *)sess, 26);
		ysock_write((struct ysock *)sess, buffer, 2);
	}
	return 0;
}

static struct packet_parser pptab[] = {
	PACKET_PARSER_INIT(PACK_ALIVE, do_parse_alive),
};

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
	ysock_stop();
}

int main(int argc, char *argv[])
{
	// init
	conf_init("./kong.xml");

	FLAGS_log_dir = conf.get<std::string>("conf.kong.logdir").c_str();
	FLAGS_minloglevel = google::INFO;
	FLAGS_stderrthreshold = google::INFO;
	FLAGS_colorlogtostderr = true;
	google::InitGoogleLogging(argv[0]);

	signal(SIGINT, signal_handler);
	plugins_init(conf.get<std::string>("conf.kong.plugindir").c_str());
	datacore_init();

	// run server
	for (size_t i = 0; i < sizeof(pptab)/sizeof(struct packet_parser); i++)
		register_packet_parser(&pptab[i]);
	struct ysock server;
	ysock_init(&server);
	if (ysock_listen(&server, htons(conf.get<unsigned short>("conf.kong.port")),
			 0, NULL) == -1)
		exit(EXIT_FAILURE);
	ysock_on_connection(&server, on_connection);
	ysock_exec();
	ysock_close(&server);
	for (size_t i = 0; i < sizeof(pptab)/sizeof(struct packet_parser); i++)
		unregister_packet_parser(&pptab[i]);

	// fini
	datacore_fini();
	plugins_fini();
	google::ShutdownGoogleLogging();
	return 0;
}