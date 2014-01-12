/*
 * Copyright (C) 2013 Mark Hills <mark@xwax.org>,
 *                    Yves Adler <yves.adler@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <assert.h>
#include <stdlib.h>

#include "selector.h"

/*
 * Scroll to our target entry if it can be found, otherwise leave our
 * position unchanged
 */

static void retain_target(struct selector *sel)
{
    size_t n;
    struct index *l;

    if (sel->target == NULL)
        return;

    l = sel->view_index;

    switch (sel->sort) {
    case SORT_ARTIST:
    case SORT_BPM:
        n = index_find(l, sel->target, sel->sort);
        break;
    case SORT_PLAYLIST:
        /* Linear search */
        for (n = 0; n < l->entries; n++) {
            if (l->record[n] == sel->target)
                break;
        }
        break;
    default:
        abort();
    }

    if (n < l->entries)
        listbox_to(&sel->records, n);
}

/*
 * Optimised version of retain_target where our position may
 * only have moved due to insertion of a single record
 */

static void hunt_target(struct selector *s)
{
    struct index *l;
    size_t n;

    if (s->target == NULL)
        return;

    l = s->view_index;
    n = listbox_current(&s->records);

    if (n < l->entries && l->record[n + 1] == s->target) {
        struct listbox *x;

        /* Retain selection in the same position on screen
         * FIXME: listbox should provide this functionality */

        x = &s->records;

        x->selected++;
        x->offset++;
    }
}

static struct crate* current_crate(struct selector *sel)
{
    int n;

    n = listbox_current(&sel->crates);
    if (n == -1)
        return NULL;

    return sel->library->crate[n];
}

/*
 * Return the index which acts as the starting point before
 * string matching, based on the current crate
 */

static struct index* initial(struct selector *sel)
{
    struct crate *c;

    c = current_crate(sel);
    assert(c != NULL);

    switch (sel->sort) {
    case SORT_ARTIST:
        return &c->listing.by_artist;
    case SORT_BPM:
        return &c->listing.by_bpm;
    case SORT_PLAYLIST:
        return &c->listing.by_order;
    default:
        abort();
    }
}

static void notify(struct selector *s)
{
    fire(&s->changed, NULL);
}

/*
 * When the crate has changed, update the current index to reflect
 * the crate and the search criteria
 */

static void do_content_change(struct selector *sel)
{
    /* FIXME: this is called when the UI is too small and no
     * crate is selected, so initial() hits an assertion */

    (void)index_match(initial(sel), sel->view_index, &sel->match);
    listbox_set_entries(&sel->records, sel->view_index->entries);
    retain_target(sel);
    notify(sel);
}

/*
 * A new record has been added to the currently selected crate. Merge
 * this new addition into the current view, if applicable.
 */

static void merge_addition(struct observer *o, void *x)
{
    struct selector *s = container_of(o, struct selector, on_addition);
    struct record *r = x;

    if (!record_match(r, &s->match))
        return;

    if (s->sort == SORT_PLAYLIST)
        index_add(s->view_index, r);
    else
        index_insert(s->view_index, r, s->sort);

    listbox_set_entries(&s->records, s->view_index->entries);
    hunt_target(s);
    notify(s);
}

void selector_init(struct selector *sel, struct library *lib)
{
    struct crate *c;

    sel->library = lib;

    listbox_init(&sel->records);
    listbox_init(&sel->crates);

    listbox_set_entries(&sel->crates, lib->crates);

    sel->toggled = false;
    sel->sort = SORT_ARTIST;
    sel->search[0] = '\0';
    sel->search_len = 0;
    sel->target = NULL;

    index_init(&sel->index_a);
    index_init(&sel->index_b);
    sel->view_index = &sel->index_a;
    sel->swap_index = &sel->index_b;

    c = current_crate(sel);
    watch(&sel->on_addition, &c->addition, merge_addition);

    (void)index_copy(initial(sel), sel->view_index);
    listbox_set_entries(&sel->records, sel->view_index->entries);

    event_init(&sel->changed);
}

void selector_clear(struct selector *sel)
{
    event_clear(&sel->changed);
    ignore(&sel->on_addition);
    index_clear(&sel->index_a);
    index_clear(&sel->index_b);
}

void selector_set_lines(struct selector *sel, unsigned int lines)
{
    listbox_set_lines(&sel->crates, lines);
    listbox_set_lines(&sel->records, lines);
}

/*
 * Return: the currently selected record, or NULL if none selected
 */

struct record* selector_current(struct selector *sel)
{
    int i;

    i = listbox_current(&sel->records);
    if (i == -1) {
        return NULL;
    } else {
        return sel->view_index->record[i];
    }
}

/*
 * Make a note of the current selected record, and make it the
 * position we will try and retain if the crate is changed etc.
 */

static void set_target(struct selector *sel)
{
    struct record *x;

    x = selector_current(sel);
    if (x != NULL)
        sel->target = x;
}

void selector_up(struct selector *sel)
{
    listbox_up(&sel->records, 1);
    set_target(sel);
    notify(sel);
}

void selector_down(struct selector *sel)
{
    listbox_down(&sel->records, 1);
    set_target(sel);
    notify(sel);
}

void selector_page_up(struct selector *sel)
{
    listbox_up(&sel->records, sel->records.lines);
    set_target(sel);
    notify(sel);
}

void selector_page_down(struct selector *sel)
{
    listbox_down(&sel->records, sel->records.lines);
    set_target(sel);
    notify(sel);
}

void selector_top(struct selector *sel)
{
    listbox_first(&sel->records);
    set_target(sel);
    notify(sel);
}

void selector_bottom(struct selector *sel)
{
    listbox_last(&sel->records);
    set_target(sel);
    notify(sel);
}

/*
 * Helper function when we have switched crate
 */

static void do_crate_change(struct selector *sel)
{
    struct crate *c;

    c = current_crate(sel);

    ignore(&sel->on_addition);
    watch(&sel->on_addition, &c->addition, merge_addition);
    do_content_change(sel);
}

void selector_prev(struct selector *sel)
{
    listbox_up(&sel->crates, 1);
    sel->toggled = false;
    do_crate_change(sel);
}

void selector_next(struct selector *sel)
{
    listbox_down(&sel->crates, 1);
    sel->toggled = false;
    do_crate_change(sel);
}

/*
 * Toggle between the current crate and the 'all' crate
 */

void selector_toggle(struct selector *sel)
{
    if (!sel->toggled) {
        sel->toggle_back = listbox_current(&sel->crates);
        listbox_first(&sel->crates);
        sel->toggled = true;
    } else {
        listbox_to(&sel->crates, sel->toggle_back);
        sel->toggled = false;
    }

    do_crate_change(sel);
}

/*
 * Toggle between sort order
 */

void selector_toggle_order(struct selector *sel)
{
    set_target(sel);
    sel->sort = (sel->sort + 1) % SORT_END;
    do_content_change(sel);
}

/*
 * Expand the search. Do not disrupt the running process on memory
 * allocation failure, leave the view index incomplete
 */

void selector_search_expand(struct selector *sel)
{
    if (sel->search_len == 0)
        return;

    sel->search[--sel->search_len] = '\0';
    match_compile(&sel->match, sel->search);

    do_content_change(sel);
}

/*
 * Refine the search. Do not distrupt the running process on memory
 * allocation failure, leave the view index incomplete
 */

void selector_search_refine(struct selector *sel, char key)
{
    struct index *tmp;

    if (sel->search_len >= sizeof(sel->search) - 1) /* would overflow */
        return;

    sel->search[sel->search_len] = key;
    sel->search[++sel->search_len] = '\0';
    match_compile(&sel->match, sel->search);

    (void)index_match(sel->view_index, sel->swap_index, &sel->match);

    tmp = sel->view_index;
    sel->view_index = sel->swap_index;
    sel->swap_index = tmp;

    listbox_set_entries(&sel->records, sel->view_index->entries);
    set_target(sel);
    notify(sel);
}
