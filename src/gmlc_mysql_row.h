/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GMLC_MYSQL_ROW_H__
#define __GMLC_MYSQL_ROW_H__

#include <glib.h>
#include <glib-object.h>

#include "gmlc_mysql_server.h"
#include "gmlc_mysql_query.h"

#define UNUSED_VAR(x) (x = x)

G_BEGIN_DECLS

#define GMLC_MYSQL_TYPE_ROW             (gmlc_mysql_row_get_type ())
#define GMLC_MYSQL_ROW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMLC_MYSQL_TYPE_ROW, GmlcMysqlRow))
#define GMLC_MYSQL_ROW_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), GMLC_MYSQL_TYPE_ROW,  GmlcMysqlRowClass))
#define GMLC_MYSQL_IS_ROW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMLC_MYSQL_TYPE_ROW))
#define GMLC_MYSQL_IS_ROW_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), GMLC_MYSQL_TYPE_ROW))
#define GMLC_MYSQL_ROW_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), GMLC_MYSQL_TYPE_ROW, GmlcMysqlRowClass))

typedef struct _GmlcMysqlRow            GmlcMysqlRow;
typedef struct _GmlcMysqlRowClass       GmlcMysqlRowClass;

struct _GmlcMysqlRow {
	GObject				parent;
	
	/* private */
	GArray *			arResults;
	
/* Connection infos */
	GmlcMysqlQuery *	pGmlcMysqlQry;
	GArray * 			arMysqlHeadersName;
	
/* Request for update infos */
	gchar *				pcAbsTableName;
	gchar *				pcPrimaryWherePart;
};

struct _GmlcMysqlRowClass {
	GObjectClass parent_class;
	
	/* class members */
	
};

GType gmlc_mysql_row_get_type (void);

GmlcMysqlRow * gmlc_mysql_row_new_next_record (GmlcMysqlQuery * pGmlcMysqlQry);

gchar * gmlc_mysql_row_get_field_value(GmlcMysqlRow * pGmlcMysqlRow, gint idx);
gchar * gmlc_mysql_row_set_field_value(GmlcMysqlRow * pGmlcMysqlRow, gint idx, const gchar * new_value);

gchar * gmlc_mysql_row_get_row_text(GmlcMysqlRow * pGmlcMysqlRow);

gboolean gmlc_mysql_row_delete(GmlcMysqlRow * pGmlcMysqlRow);

G_END_DECLS

#endif /* __GMLC_MYSQL_ROW_H__ */
