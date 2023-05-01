//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Cyril Hrubis <metan@ucw.cz>
 */

#include <errno.h>
#include <string.h>

#include <widgets/gp_widgets.h>
#include <widgets/gp_dialog_download.h>
#include <utils/gp_json.h>
#include <utils/gp_vec.h>
#include <utils/gp_user_path.h>

struct dict_url {
	char lang[16];
	char type[16];
	char license[16];
	char *url;
};

struct gp_json_struct dict_url_desc[] = {
	GP_JSON_SERDES_STR_DUP(struct dict_url, url, 0, 4096, "URL"),
	GP_JSON_SERDES_STR_CPY(struct dict_url, lang, 0, 16),
	GP_JSON_SERDES_STR_CPY(struct dict_url, license, 0, 16),
	GP_JSON_SERDES_STR_CPY(struct dict_url, type, GP_JSON_SERDES_OPTIONAL, 16),
	{}
};

//TODO: !this is hardcoded
#define URLS_ETC_PATH "/etc/gp_apps/gpdict/dict_urls.json"

struct dict_url *load_urls(void)
{
	char buf[4096];
	gp_json_val val = {.buf = buf, .buf_size = sizeof(buf)};

	struct dict_url *ret = gp_vec_new(0, sizeof(struct dict_url));
	if (!ret)
		return NULL;

	gp_json_reader *json = gp_json_reader_load("dict_urls.json");
	if (!json) {
		json = gp_json_reader_load(URLS_ETC_PATH);
		if (!json)
			goto err0;
	}

	if (gp_json_start(json) != GP_JSON_ARR) {
		gp_json_warn(json, "Expected array");
		goto err0;
	}

	GP_JSON_ARR_FOREACH(json, &val) {
		struct dict_url url = {};

		if (gp_json_read_struct(json, &val, dict_url_desc, &url))
			continue;

		GP_VEC_APPEND(ret, url);
	}

	gp_json_reader_finish(json);
	gp_json_reader_free(json);

	if (!gp_vec_len(ret))
		goto err0;

	return ret;
err0:
	gp_vec_free(ret);
	return NULL;
}

static void free_urls(struct dict_url *urls)
{
	size_t i;

	for (i = 0; i < gp_vec_len(urls); i++)
		free(urls[i].url);

	gp_vec_free(urls);
}

struct url_dialog {
	struct dict_url *urls;
	gp_widget *url_table;
	gp_dialog dialog;
};

enum urls_col_map {
	LANG,
	TYPE,
	LICENSE,
	URL
};

static int urls_get_cell(gp_widget *self, gp_widget_table_cell *cell, unsigned int col_id)
{
	unsigned int row = self->tbl->row_idx;
	struct url_dialog *url_dialog = self->priv;

	switch (col_id) {
	case LANG:
		cell->text = url_dialog->urls[row].lang;
	break;
	case TYPE:
		cell->text = url_dialog->urls[row].type;
	break;
	case LICENSE:
		cell->text = url_dialog->urls[row].license;
	break;
	case URL:
		cell->text = url_dialog->urls[row].url;
	break;
	}

	return 1;
}

static int urls_seek_row(gp_widget *self, int op, unsigned int pos)
{
	struct url_dialog *url_dialog = self->priv;

	if (!url_dialog)
		return 0;

	switch (op) {
	case GP_TABLE_ROW_RESET:
		self->tbl->row_idx = 0;
	break;
	case GP_TABLE_ROW_ADVANCE:
		self->tbl->row_idx += pos;
	break;
	case GP_TABLE_ROW_MAX:
		return gp_vec_len(url_dialog->urls);
	}

	if (self->tbl->row_idx >= gp_vec_len(url_dialog->urls))
		return 0;

	return 1;
}

static gp_widget_table_col_ops urls_col_ops = {
	.seek_row = urls_seek_row,
	.get_cell = urls_get_cell,
	.col_map = {
		{.id = "lang", .idx = LANG},
		{.id = "license", .idx = LICENSE},
		{.id = "type", .idx = TYPE},
		{.id = "url", .idx = URL},
	}
};

static int exit_dialog(gp_widget_event *ev)
{
	struct url_dialog *url_dialog = ev->self->priv;

	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	url_dialog->dialog.retval = 1;

	return 0;
}

#define DESTDIR ".stardict/dic/"

//TODO: Move to library
static const char *fname(const char *path)
{
	size_t len = strlen(path);

	while (len > 0 && path[len] != '/')
		len--;

	return path + len;
}

static int download_url(gp_widget_event *ev)
{
	struct url_dialog *url_dialog = ev->self->priv;

	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	if (ev->sub_type != GP_WIDGET_TABLE_TRIGGER)
		return 0;

	if (!url_dialog->url_table)
		return 0;

	unsigned int sel = url_dialog->url_table->tbl->selected_row;
	char *url = url_dialog->urls[sel].url;

	printf("Downloading '%s'\n", url);

	if (gp_user_mkpath(DESTDIR, 0)) {
		gp_dialog_msg_printf_run(GP_DIALOG_MSG_ERR, "Failed to create directory",
                                         "'%s': %s", DESTDIR, strerror(errno));
		return 0;
	}

	char *fname_path = gp_user_path(DESTDIR, fname(url));

	if (gp_dialog_download_run(url_dialog->urls[sel].url, fname_path))
		return 0;

	//TODO: Cleanup
	char *cmd;

	asprintf(&cmd, "tar xf '%s' --strip-components=1 -C '%s/" DESTDIR "'", fname_path, gp_user_home());

	printf("%s\n", cmd);
	system(cmd);

	unlink(fname_path);
	free(fname_path);

	return 0;
}

static const gp_widget_json_addr addrs[] = {
	{.id = "download_url", .on_event = download_url},
	{.id = "exit_dialog", .on_event = exit_dialog},
	{.id = "url_table_ops", .table_col_ops = &urls_col_ops},
	{}
};

void reload_dicts(void);

void run_download_dialog(void)
{
	struct url_dialog url_dialog = {};
	gp_htable *uids = NULL;

	url_dialog.urls = load_urls();
	if (!url_dialog.urls) {
		gp_dialog_msg_run(GP_DIALOG_MSG_WARN, "Warning", "No dictionary URLs defined");
		return;
	}

	gp_widget_json_callbacks callbacks = {
		.default_priv = &url_dialog,
		.addrs = addrs,
	};

	url_dialog.dialog.layout = gp_app_named_layout_load("gpdict", "layout_download", &callbacks, &uids);
	url_dialog.url_table = gp_widget_by_uid(uids, "url_table", GP_WIDGET_TABLE);
	gp_htable_free(uids);

	if (url_dialog.dialog.layout) {
		gp_dialog_run(&url_dialog.dialog);
		reload_dicts();
	}

	gp_widget_free(url_dialog.dialog.layout);

	free_urls(url_dialog.urls);
}

int download_dicts(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	run_download_dialog();

	return 0;
}

