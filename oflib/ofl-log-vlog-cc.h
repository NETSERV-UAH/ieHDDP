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

#ifndef OFL_LOG_VLOG_CC_H
#define OFL_LOG_VLOG_CC_H

#include "vlog.hh"

#define VLOG_NAME(NAME) lg_##NAME

#define OFL_LOG_INIT(MODULE) \
        vigil::Vlog_module VLOG_NAME(MODULE)("MODULE");

#define OFL_LOG_DBG(MODULE, ...) \
    VLOG_NAME(MODULE).dbg(__VA_ARGS__)

#define OFL_LOG_WARN(MODULE, ...) \
    VLOG_NAME(MODULE).warn(__VA_ARGS__)

#define OFL_LOG_IS_DBG_ENABLED(MODULE) \
    VLOG_NAME(MODULE).is_dbg_enabled()

#define OFL_LOG_IS_WARN_ENABLED(MODULE) \
    VLOG_NAME(MODULE).is_warn_enabled()

#endif /* OFL_LOG_VLOG_CC_H */
