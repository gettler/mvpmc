/*
 *  Copyright (C) 2008, Jon Gettler
 *  http://www.mvpmc.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef AV_LOCAL_H
#define AV_LOCAL_H

typedef struct {
	int	playing;
	pid_t	child;
} av_info_t;

#define do_play_file	__av_play_file
#define do_play_dvd	__av_play_dvd
#define do_stop		__av_stop
#define info		__av_info
#define arch_init	__av_arch_init

extern int do_play_file(char *path);
extern int do_play_dvd(char *path);
extern int do_stop(void);
extern int arch_init(void);

extern av_info_t info;

#endif /* AV_LOCAL_H */
