/***************************************************************************
 *            gmlc_mysql_procedure.c
 *
 *  
 *  Copyright  2007  Didier "pozer" Prolhac
 *  pozer@thepozer.net
 ***************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "gmlc_mysql_procedure.h"

static void gmlc_mysql_procedure_finalize (GmlcMysqlProcedure * pGmlcMysqlPrc);
static void gmlc_mysql_procedure_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void gmlc_mysql_procedure_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);

enum {
	PROP_0,
	PROP_DATABASE,
	PROP_NAME,
	PROP_FLAGGED
};


G_DEFINE_TYPE (GmlcMysqlProcedure, gmlc_mysql_procedure, G_TYPE_OBJECT)

static void gmlc_mysql_procedure_class_init(GmlcMysqlProcedureClass *pClass) {
	GObjectClass * pObjClass = G_OBJECT_CLASS(pClass);

	pObjClass->finalize = (GObjectFinalizeFunc) gmlc_mysql_procedure_finalize;
	pObjClass->get_property = gmlc_mysql_procedure_get_property;
	pObjClass->set_property = gmlc_mysql_procedure_set_property;
	
	g_object_class_install_property(pObjClass, PROP_DATABASE, 
		g_param_spec_object("database", "Database object", "Database object", GMLC_TYPE_MYSQL_DATABASE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property(pObjClass, PROP_NAME, 
		g_param_spec_string("name", "Procedure name", "Name of the procedure", "", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property(pObjClass, PROP_FLAGGED, 
		g_param_spec_boolean("flagged", "Object Flagged", "Object Flagged", FALSE, G_PARAM_READWRITE));
}

static void gmlc_mysql_procedure_init(GmlcMysqlProcedure * pGmlcMysqlPrc) {
	pGmlcMysqlPrc->pGmlcMysqlDb = NULL;
	pGmlcMysqlPrc->pcName = NULL;
	pGmlcMysqlPrc->bFlagged = FALSE;
}

static void gmlc_mysql_procedure_finalize(GmlcMysqlProcedure * pGmlcMysqlPrc) {
	
	g_free(pGmlcMysqlPrc->pcName);
	/*G_OBJECT_CLASS(parent_class)->finalize(object);*/
}

static void gmlc_mysql_procedure_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec) {
	GmlcMysqlProcedure * pGmlcMysqlPrc = GMLC_MYSQL_PROCEDURE(object);
	
	switch (prop_id) {
		case PROP_DATABASE :
			g_value_set_object(value, pGmlcMysqlPrc->pGmlcMysqlDb);
			break;
		case PROP_NAME :
			g_value_set_string(value, pGmlcMysqlPrc->pcName);
			break;
		case PROP_FLAGGED :
			g_value_set_boolean(value, pGmlcMysqlPrc->bFlagged);
			break;
		default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
		}
	}
}

static void gmlc_mysql_procedure_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec) {
	GmlcMysqlProcedure * pGmlcMysqlPrc = GMLC_MYSQL_PROCEDURE(object);
	
	switch (prop_id) {
		case PROP_DATABASE :
			pGmlcMysqlPrc->pGmlcMysqlDb = g_value_get_object(value);
			break;
		case PROP_NAME :
			g_free(pGmlcMysqlPrc->pcName);
			
			pGmlcMysqlPrc->pcName = g_value_dup_string(value);
			break;
		case PROP_FLAGGED :
			pGmlcMysqlPrc->bFlagged = g_value_get_boolean(value);
			break;
		default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
		}
	}
}

GmlcMysqlProcedure * gmlc_mysql_procedure_new(GmlcMysqlDatabase * pGmlcMysqlDb, gchar * pcName) {
	GmlcMysqlProcedure * pGmlcMysqlPrc;
	
	pGmlcMysqlPrc = GMLC_MYSQL_PROCEDURE(g_object_new(GMLC_TYPE_MYSQL_PROCEDURE, "database", pGmlcMysqlDb, "name", pcName, NULL));
	
	return pGmlcMysqlPrc;
}
