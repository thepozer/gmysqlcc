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

#ifndef __GMLC_DUMP_SOURCE_TABLE_H__
#define __GMLC_DUMP_SOURCE_TABLE_H__ 

#include "gmlc_main.h"
#include "gmlc_mysql_table.h"

G_BEGIN_DECLS

#define GMLC_DUMP_TYPE_SOURCE_TABLE				(gmlc_dump_source_table_get_type ())
#define GMLC_DUMP_SOURCE_TABLE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), GMLC_DUMP_TYPE_SOURCE_TABLE, GmlcDumpSourceTable))
#define GMLC_DUMP_SOURCE_TABLE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GMLC_DUMP_TYPE_SOURCE_TABLE, GmlcDumpSourceTableClass))
#define GMLC_DUMP_IS_SOURCE_TABLE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMLC_DUMP_TYPE_SOURCE_TABLE))
#define GMLC_DUMP_IS_SOURCE_TABLE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GMLC_DUMP_TYPE_SOURCE_TABLE))
#define GMLC_DUMP_GET_SOURCE_TABLE_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GMLC_DUMP_TYPE_SOURCE_TABLE, GmlcDumpSourceTableClass))

typedef struct _GmlcDumpSourceTable GmlcDumpSourceTable;
typedef struct _GmlcDumpSourceTableClass GmlcDumpSourceTableClass;

struct _GmlcDumpSourceTable {
	GObject parent;
	
	/* private members */
	GmlcMysqlTable * pGmlcMysqlTbl;
};

struct _GmlcDumpSourceTableClass {
	GObjectClass parent_class;
	
	/* define vtable methods and signals here */
};

GType gmlc_dump_source_table_get_type (void);

GmlcDumpSourceTable * gmlc_dump_source_table_new (GmlcMysqlTable *pGmlcMysqlTbl);

G_END_DECLS

#endif /* __GMLC_GUI_SERVER_TAB_DATA_H__ */
