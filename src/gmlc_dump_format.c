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

#include "gmlc_dump_format.h"

static void gmlc_dump_format_base_init (gpointer g_class) {
	static gboolean initialized = FALSE;
	UNUSED_VAR(g_class);
	
	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

GType gmlc_dump_format_get_type (void) {
  static GType type = 0;
  
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (GmlcDumpFormatInterface),
      gmlc_dump_format_base_init,   /* base_init */
      NULL,   /* base_finalize */
      NULL,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      0,
      0,      /* n_preallocs */
      NULL    /* instance_init */
    };
    
    type = g_type_register_static (G_TYPE_INTERFACE, "GmlcDumpFormat", &info, 0);
  }
  
  return type;
}

gboolean gmlc_dump_format_set_struct (GmlcDumpFormat * self, const gchar * pcStruct) {
	return GMLC_DUMP_FORMAT_GET_INTERFACE(self)->set_struct(self, pcStruct);
}

gboolean gmlc_dump_format_set_data (GmlcDumpFormat * self, const GArray * arDatas) {
	return GMLC_DUMP_FORMAT_GET_INTERFACE(self)->set_data(self, arDatas);
}

gchar * gmlc_dump_format_run (GmlcDumpFormat * self) {
	return GMLC_DUMP_FORMAT_GET_INTERFACE(self)->run(self);
}


