/* 
 * This file is part of the HDDP Switch distribution (https://github.com/gistnetserv-uah/eHDDP).
 * Copyright (c) 2020.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MATCH_EXT_H
#define MATCH_EXT_H 1

#include <stdbool.h>
#include "oflib/ofl-structs.h"

/****************************************************************************
 * Functions for comparing two extended match structures.
 ******************************************************
 **********************/
bool
match_std_overlap(struct ofl_match *a, struct ofl_match *b);

bool 
packet_match(struct ofl_match *a, struct ofl_match *b);

/* Returns true if match a matches match b, in a strict manner. */
bool
match_std_strict(struct ofl_match *a, struct ofl_match *b);

/* Returns true if match a matches match b, in a non-strict manner. */
bool
match_std_nonstrict(struct ofl_match *a, struct ofl_match *b);



#endif /* MATCH_STD_H */
