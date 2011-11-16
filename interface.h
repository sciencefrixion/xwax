/*
 * Copyright (C) 2011 Mark Hills <mark@pogo.org.uk>
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

#ifndef INTERFACE_H
#define INTERFACE_H

#include <pthread.h>

#include "deck.h"
#include "library.h"
#include "selector.h"

struct interface {
    pthread_t ph;

    size_t ndeck;
    struct deck *deck;

    struct selector selector;
};

int interface_start(struct interface *in, struct deck deck[], size_t ndeck,
                    struct library *lib);
void interface_stop(struct interface *in);

#endif
