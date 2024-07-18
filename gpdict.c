//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Cyril Hrubis <metan@ucw.cz>
 */

#include <libstardict.h>
#include <gfxprim.h>

static struct sd_dict_paths dict_paths;
static size_t dict_paths_idx = 0;
static size_t last_dict_idx = -1;
static struct sd_dict *dict;
static struct sd_lookup_res res;

static gp_widget *result;
static gp_widget *lookup_res;
static gp_widget *lookup;
static gp_widget *dict_name;
static gp_widget *layout_switch;

static int lookup_res_seek_row(gp_widget *self, int op, unsigned int pos)
{
	switch (op) {
	case GP_TABLE_ROW_RESET:
                self->tbl->row_idx = 0;
        break;
        case GP_TABLE_ROW_ADVANCE:
                self->tbl->row_idx += pos;
        break;
        case GP_TABLE_ROW_MAX:
                return sd_lookup_res_cnt(&res);
        }

        if (self->tbl->row_idx < sd_lookup_res_cnt(&res))
                return 1;

	return 0;
}

static int res_get_elem(gp_widget *self, gp_widget_table_cell *cell, unsigned int col)
{
	if (col || !dict)
		return 0;

	cell->text = sd_idx_to_word(dict, res.min + self->tbl->row_idx);

	return 1;
}

static void entry_markup_set(gp_widget *result, struct sd_entry *entry)
{
	int flags = 0;
	enum gp_markup_fmt fmt;

	switch (entry->fmt) {
	case SD_ENTRY_PANGO_MARKUP:
	case SD_ENTRY_XDXF:
		flags = GP_MARKUP_HTML_KEEP_WS;
	/* fallthrough */
	case SD_ENTRY_HTML:
		fmt = GP_MARKUP_HTML;
	break;
	default:
		fmt = GP_MARKUP_PLAINTEXT;
	break;
	}

	gp_widget_markup_set(result, fmt, flags, entry->data);
}

static void show_entry(unsigned int idx)
{
	struct sd_entry *entry;

	entry = sd_get_entry(dict, idx);
	entry_markup_set(result, entry);
	gp_widget_redraw(lookup_res);
	gp_widget_label_set(lookup, sd_idx_to_word(dict, idx));
	sd_free_entry(entry);
}

static int lookup_res_set(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	if (ev->sub_type != GP_WIDGET_TABLE_SELECT)
		return 0;

	show_entry(res.min + ev->self->tbl->selected_row);

	return 0;
}

const gp_widget_table_col_ops lookup_res_col_ops = {
	.seek_row = lookup_res_seek_row,
	.get_cell = res_get_elem,
	.on_event = lookup_res_set,
	.col_map = {
		{.id = "res", .idx = 0}
	}
};

int edit_event(gp_widget_event *ev)
{
	struct sd_lookup_res tmp;

	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	switch (ev->sub_type) {
	case GP_WIDGET_TBOX_POST_FILTER:
		if (!sd_lookup(dict, ev->self->tbox->buf, &tmp))
			return 1;

		return 0;
	case GP_WIDGET_TBOX_PRE_FILTER:
		if (!dict)
			return 1;
		return 0;
	case GP_WIDGET_TBOX_EDIT:
		sd_lookup(dict, ev->self->tbox->buf, &res);
		show_entry(res.min);
	break;
	case GP_WIDGET_TBOX_PASTE:
		gp_widget_tbox_clear(ev->self);
	break;
	}

	return 0;
}

gp_app_info app_info = {
	.name = "gpdict",
	.desc = "A stardict compatible dictionary",
	.version = "1.0",
	.license = "GPL-2.0-or-later",
	.url = "http://github.com/gfxprim/gpdict",
	.authors = (gp_app_info_author []) {
		{.name = "Cyril Hrubis", .email = "metan@ucw.cz", .years = "2022-2023"},
		{}
	}
};

static const char *get_dict_name(gp_widget *self, size_t idx)
{
	(void) self;

	if (idx >= dict_paths.dict_cnt)
		return NULL;

	return dict_paths.paths[idx]->book_name;
}

static void set_dict(gp_widget *self, size_t idx)
{
	(void) self;

	if (idx >= dict_paths.dict_cnt)
		return;

//	if (idx == dict_paths_idx)
//		return;

	sd_close_dict(dict);
	dict = sd_open_dict(dict_paths.paths[idx]->dir, dict_paths.paths[idx]->fname);

	dict_paths_idx = idx;

	if (dict_name)
		gp_widget_label_set(dict_name, dict->book_name);
}

static size_t get_dict(gp_widget *self, enum gp_widget_choice_op op)
{
	(void) self;

	switch (op) {
	case GP_WIDGET_CHOICE_OP_SEL:
		return dict_paths_idx;
	case GP_WIDGET_CHOICE_OP_CNT:
		return dict_paths.dict_cnt;
	}

	return 0;
}

const gp_widget_choice_desc dict_selection = {
	.ops = &(gp_widget_choice_ops) {
		.get_choice = get_dict_name,
		.get = get_dict,
		.set = set_dict,
	}
};

int select_layout_0(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	gp_widget_layout_switch_layout(layout_switch, 0);

	return 0;
}

int select_layout_1(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	gp_widget_layout_switch_layout(layout_switch, 1);

	return 0;
}

static void restore_last_used_dict(void)
{
	char dict_name[128];
	unsigned int i;

	if (gp_app_cfg_scanf("gpdict", "selected_dict.txt", "%127[^\n]s", dict_name) != 1) {
		set_dict(NULL, 0);
		return;
	}

	for (i = 0; i < dict_paths.dict_cnt; i++) {
		if (!strcmp(dict_paths.paths[i]->book_name, dict_name)) {
			set_dict(NULL, i);
			last_dict_idx = i;
			return;
		}
	}

	set_dict(NULL, 0);
}

static int save_last_used_dict(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_FREE)
		return 0;

	if (dict_paths_idx == last_dict_idx)
		return 0;

	gp_app_cfg_printf("gpdict", "selected_dict.txt", "%s",
	                  dict_paths.paths[dict_paths_idx]->book_name);
	return 1;
}

void reload_dicts(void)
{
	sd_free_dict_paths(&dict_paths);
	sd_lookup_dict_paths(&dict_paths);
	restore_last_used_dict();
}

int main(int argc, char *argv[])
{
	gp_htable *uids;

	sd_lookup_dict_paths(&dict_paths);

	gp_widget *layout = gp_app_layout_load("gpdict", &uids);
	result = gp_widget_by_uid(uids, "result", GP_WIDGET_MARKUP);
	lookup = gp_widget_by_uid(uids, "lookup", GP_WIDGET_LABEL);
	lookup_res = gp_widget_by_uid(uids, "lookup_res", GP_WIDGET_TABLE);
	layout_switch = gp_widget_by_uid(uids, "layout_switch", GP_WIDGET_SWITCH);
	dict_name = gp_widget_by_uid(uids, "dict_name", GP_WIDGET_LABEL);

	restore_last_used_dict();

	gp_htable_free(uids);

	gp_app_on_event_set(save_last_used_dict);

	gp_widgets_main_loop(layout, NULL, argc, argv);

	return 0;
}
