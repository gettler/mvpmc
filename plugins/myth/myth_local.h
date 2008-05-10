/*
 *  Copyright (C) 2008, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MYTH_LOCAL_H
#define MYTH_LOCAL_H

#include <cmyth.h>
#include <mvp_refmem.h>

#define control		__myth_control
#define server		__myth_server
#define recdir		__myth_recdir
#define conn_open	__myth_conn_open
#define conn_close	__myth_conn_close
#define rec_list	__myth_rec_list

/*
 * Swap a reference counted global for a new reference (including NULL).
 * This little dance ensures that no one can be taking a new reference
 * to the global while the old reference is being destroyed.  Either another
 * thread will get the value while it is still held and hold it again, or
 * it will get the new (held) value.  At the end of this, 'new_ref' will be
 * held both by the 'new_ref' reference and the newly created 'ref'
 * reference.  The caller is responsible for releasing 'new_ref' when it is
 * no longer in use (and 'ref' for that matter).
 */
#define CHANGE_GLOBAL_REF(global_ref, new_ref)		\
	{						\
		void *tmp_ref = ref_hold((global_ref));	\
		ref_release((global_ref));		\
		(global_ref) = ref_hold((new_ref));	\
		ref_release((tmp_ref));			\
	}

extern volatile cmyth_conn_t control;
extern char *server;
extern char *recdir;

extern int conn_open(char *host, int portnum);
extern int conn_close(void);

extern int rec_list(gw_t *widget);

#endif /* MYTH_LOCAL_H */
