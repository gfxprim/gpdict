//SPDX-License-Identifier: GPL-2.0-or-later

/*

    Copyright (C) 2022-2023 Cyril Hrubis <metan@ucw.cz>

 */

#include <libstardict.h>
#include <gfxprim.h>

static struct sd_dict_paths dict_paths;
static size_t dict_paths_idx = 0;
static struct sd_dict *dict;
static struct sd_entry *entry;
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

const gp_widget_table_col_ops lookup_res_col_ops = {
	.seek_row = lookup_res_seek_row,
	.get_cell = res_get_elem,
	.col_map = {
		{.id = "res", .idx = 0}
	}
};

static enum gp_markup_fmt entry_markup_fmt(struct sd_entry *entry)
{
	switch (entry->fmt) {
	case SD_ENTRY_PANGO_MARKUP:
	case SD_ENTRY_HTML:
		return GP_MARKUP_HTML;
	default:
		return GP_MARKUP_PLAINTEXT;
	}
}

int edit_event(gp_widget_event *ev)
{
	struct sd_lookup_res tmp;

	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	switch (ev->sub_type) {
	case GP_WIDGET_TBOX_POST_FILTER:
		if (!sd_lookup_dict(dict, ev->self->tbox->buf, &tmp))
			return 1;

		return 0;
	case GP_WIDGET_TBOX_PRE_FILTER:
		if (!dict)
			return 1;
		return 0;
	case GP_WIDGET_TBOX_EDIT:
		sd_lookup_dict(dict, ev->self->tbox->buf, &res);
		sd_free_entry(entry);
		entry = sd_get_entry(dict, res.min);


		gp_widget_markup_set(result, entry_markup_fmt(entry), entry->data);
		gp_widget_redraw(lookup_res);
		gp_widget_label_set(lookup, sd_idx_to_word(dict, res.min));
	break;
	}

	return 0;
}

static gp_app_info app_info = {
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

	return dict_paths.paths[idx]->name;
}

static void set_dict(gp_widget *self, size_t idx)
{
	(void) self;

	if (idx >= dict_paths.dict_cnt)
		return;

//	if (idx == dict_paths_idx)
//		return;

	sd_close_dict(dict);
	dict = sd_open_dict(dict_paths.paths[idx]->dir, dict_paths.paths[idx]->name);

	dict_paths_idx = idx;

	if (dict_name)
		gp_widget_label_set(dict_name, dict_paths.paths[idx]->name);
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

struct gp_widget_choice_ops dict_selection = {
	.get_choice = get_dict_name,
	.get = get_dict,
	.set = set_dict,
};

int select_layout_0(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	gp_widget_switch_layout(layout_switch, 0);

	return 0;
}

int select_layout_1(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	gp_widget_switch_layout(layout_switch, 1);

	return 0;
}

int main(int argc, char *argv[])
{
	gp_htable *uids;

	gp_app_info_set(&app_info);

	sd_lookup_dict_paths(&dict_paths);

	gp_widget *layout = gp_app_layout_load("gpdict", &uids);
	result = gp_widget_by_uid(uids, "result", GP_WIDGET_MARKUP);
	lookup = gp_widget_by_uid(uids, "lookup", GP_WIDGET_LABEL);
	lookup_res = gp_widget_by_uid(uids, "lookup_res", GP_WIDGET_TABLE);
	layout_switch = gp_widget_by_uid(uids, "layout_switch", GP_WIDGET_SWITCH);
	dict_name = gp_widget_by_uid(uids, "dict_name", GP_WIDGET_LABEL);

	gp_htable_free(uids);

	set_dict(NULL, 0);

	gp_widgets_main_loop(layout, "gpdict", NULL, argc, argv);

	return 0;
}
