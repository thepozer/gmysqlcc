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

#ifndef __GMLC_DUMP_FORMAT_SQL_H__
#define __GMLC_DUMP_FORMAT_SQL_H__ 

#include "gmlc_main.h"
#include "gmlc_mysql_server.h"

G_BEGIN_DECLS

#define GMLC_DUMP_TYPE_FORMAT_SQL				(gmlc_dump_format_sql_get_type ())
#define GMLC_DUMP_FORMAT_SQL(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), GMLC_DUMP_TYPE_FORMAT_SQL, GmlcDumpFormatSql))
#define GMLC_DUMP_FORMAT_SQL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GMLC_DUMP_TYPE_FORMAT_SQL, GmlcDumpFormatSqlClass))
#define GMLC_DUMP_IS_FORMAT_SQL(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMLC_DUMP_TYPE_FORMAT_SQL))
#define GMLC_DUMP_IS_FORMAT_SQL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GMLC_DUMP_TYPE_FORMAT_SQL))
#define GMLC_DUMP_GET_FORMAT_SQL_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GMLC_DUMP_TYPE_FORMAT_SQL, GmlcDumpFormatSqlClass))

typedef struct _GmlcDumpFormatSql GmlcDumpFormatSql;
typedef struct _GmlcDumpFormatSqlClass GmlcDumpFormatSqlClass;

struct _GmlcDumpFormatSql {
	GObject parent;
	
	/* private members */
};

struct _GmlcDumpFormatSqlClass {
	GObjectClass parent_class;
	
	/* define vtable methods and signals here */
};

GType gmlc_dump_format_sql_get_type (void);

GmlcDumpFormatSql * gmlc_dump_format_sql_new ();

G_END_DECLS

#endif /* __GMLC_DUMP_FORMAT_SQL_H__ */
