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

#include "gmlc_mysql_query.h"

#include <stdlib.h>
#include <string.h>

static void gmlc_mysql_query_finalize (GmlcMysqlQuery * pGmlcMysqlQry);
static void gmlc_mysql_query_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gmlc_mysql_query_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gmlc_mysql_query_connect(GmlcMysqlQuery * pGmlcMysqlQry);

void gmlc_mysql_query_read_error(GmlcMysqlQuery * pGmlcMysqlQry, gboolean bClearOnly);


enum {
	PROP_0,
	PROP_RAW_HEADERS,
	PROP_SERVER,
	PROP_QUERY,
	PROP_SRV_CHARSET,
	PROP_DB_NAME,
	PROP_ERR_MSG,
	PROP_ABS_TABLE_NAME,
	PROP_TABLE_NAME,
	PROP_NB_FIELD,
	PROP_EDIT_RESULT,
	PROP_ERR_CODE,
	PROP_CAN_EDIT
};

G_DEFINE_TYPE (GmlcMysqlQuery, gmlc_mysql_query, G_TYPE_OBJECT);

gboolean gmlc_tools_query_is_read_query(const gchar * pcQuery, gsize szQuery) {
	GString * pstrWord = NULL;
	gchar * pcQueryUp = NULL, * pcTmp = NULL, * pcPrevTmp = NULL;
	gunichar ucChar = 0;
	gsize szPos = 0;
	gboolean bReturn = TRUE;
	gboolean bEndLoop, bFirstWord, bInString;
	
	pcQueryUp = g_utf8_strup(pcQuery, szQuery);
	
	if (pcQueryUp == NULL) {
		return FALSE;
	}
	
	if (!g_utf8_validate(pcQueryUp, szQuery, NULL)) {
		return FALSE;
	}
	
	/*g_print("Test query (%d) : '%s'\n", szQuery, pcQueryUp);*/
	
	pcTmp = pcQueryUp;
	pstrWord = g_string_sized_new(16);
	bFirstWord = TRUE;
	bEndLoop = FALSE;
	bInString = FALSE;
	szPos = 0;
	
	while (!bEndLoop && szPos < szQuery) {
		ucChar = g_utf8_get_char(pcTmp);
		
		/*g_print("Current (%d/%d) '%c' : '%s'\n", szPos, szQuery, *pcTmp, pcTmp);*/
		
		if (bFirstWord) {
			if (!g_unichar_isspace(ucChar)) {
				g_string_append_unichar(pstrWord, ucChar);
			} else if (g_unichar_isspace(ucChar) && pstrWord->len == 0) {
				/* Do nothing ! */
			} else { /* Check word */
				gboolean bReadCmd = FALSE;
				
				/*g_print("**** Checked first word : '%s' : ", pstrWord->str);*/
				
				if (g_ascii_strncasecmp(pstrWord->str, "SELECT", 6) == 0) {
					bReadCmd = TRUE;
				} else if (g_ascii_strncasecmp(pstrWord->str, "SHOW", 4) == 0) {
					bReadCmd = TRUE;
				} else if (g_ascii_strncasecmp(pstrWord->str, "EXPLAIN", 7) == 0) {
					bReadCmd = TRUE;
				} else if (g_ascii_strncasecmp(pstrWord->str, "DESCRIBE", 8) == 0) {
					bReadCmd = TRUE;
				} else if (g_ascii_strncasecmp(pstrWord->str, "#", 1) == 0) { /* Comments */
					bReadCmd = TRUE;
					while (*pcTmp != '\n') {
						pcTmp = g_utf8_next_char(pcTmp);
						szPos ++;
					}
				}
				
				if (!bReadCmd) {
					bReturn = FALSE;
					bEndLoop = TRUE;
				}
				
				/*g_print("- Word status : %s - Global status : %s ****\n", ((bReadCmd) ? "True" : "False"), ((bReturn) ? "True" : "False"));*/
				
				bFirstWord = FALSE;
			}
		} else if ((*pcTmp) == ';' && !bInString) {
			g_string_set_size(pstrWord, 0);
			bFirstWord = TRUE;
		} else if (((*pcTmp) == '\'' || (*pcTmp) == '"') && (*pcPrevTmp) != '\\') {
			bInString = !bInString;
			/*g_print("In a string : %s\n", ((bInString) ? "True" : "False"));*/
		}
		
		pcPrevTmp = pcTmp;
		pcTmp = g_utf8_next_char(pcTmp);
		szPos ++;
	}
	
	/*g_print("Returned value : %s\n", ((bReturn) ? "True" : "False"));*/
	
	g_free (pcQueryUp);
	return bReturn;
}

gchar * gmlc_tools_query_get_one_result(MYSQL * pMysqlLink, const gchar * pcCharset, const gchar * pcQuery, const gint iIdxField) {
	MYSQL_RES *	pMysqlResult = NULL;
	MYSQL_ROW pCurrRow = NULL;
	GError * oErr = NULL;
	gulong * arlLengths = NULL;
	gchar * pcRet = NULL, * pcConvQuery = NULL;
	gint iErrCode = 0, iNbrField = 0;
	
	if (pMysqlLink == NULL) {
		return NULL;
	}
	
	pcConvQuery = g_convert(pcQuery, -1, pcCharset, "UTF-8", NULL, NULL, &oErr);
	if (pcConvQuery == NULL && oErr != NULL) {
		g_printerr("gmlc_tools_query_get_one_result - g_convert - Failed to convert query\nQuery : '%s'\nError: (%d) %s\n", pcQuery, oErr->code, oErr->message);
	}
	
	iErrCode = mysql_real_query(pMysqlLink, pcConvQuery, strlen(pcConvQuery));
	
	if (iErrCode != 0) {
		g_printerr("gmlc_tools_query_get_one_result - mysql_real_query - Failed to query\nQuery : '%s'\nError: (%d - %d) %s\n", pcQuery, iErrCode, mysql_errno(pMysqlLink), mysql_error(pMysqlLink));
		return NULL;
	}
	
	iNbrField = mysql_field_count(pMysqlLink);
	
	if (iErrCode != 0 && iNbrField < 0) {
		g_printerr("gmlc_tools_query_get_one_result - mysql_field_count - Failed to query\nQuery : '%s'\nError: (%d - %d) %s\n", pcQuery, iErrCode, mysql_errno(pMysqlLink), mysql_error(pMysqlLink));
		return NULL;
	} else if (iErrCode == 0 && iNbrField == 0) {
		/*mysql_qry->editResult = mysql_affected_rows(mysql_qry->mysql_link);*/
		return NULL;
	}
	
	pMysqlResult = mysql_use_result(pMysqlLink);
	
	if (pMysqlResult == (MYSQL_RES *)NULL) {
		g_printerr("gmlc_tools_query_get_one_result - mysql_use_result - Failed to use results\nQuery : '%s'\nError: (%d) %s\n", pcQuery, mysql_errno(pMysqlLink), mysql_error(pMysqlLink));
		return NULL;
	}
	
	pCurrRow = mysql_fetch_row(pMysqlResult);
	if (pCurrRow == NULL) {
		g_printerr("gmlc_tools_query_get_one_result - mysql_fetch_row - Failed to use results\nQuery : '%s'\nError: (%d) %s\n", pcQuery, mysql_errno(pMysqlLink), mysql_error(pMysqlLink));
		mysql_free_result(pMysqlResult);
		return NULL;
	}
	arlLengths = mysql_fetch_lengths(pMysqlResult);
	
	pcRet = g_convert(pCurrRow[iIdxField], arlLengths[iIdxField], "UTF-8", pcCharset, NULL, NULL, &oErr);
	
	if (pcRet == NULL && oErr != NULL) {
		g_printerr("gmlc_tools_query_get_one_result - g_convert - Failed to convert result \nQuery : '%s'\nError: (%d) %s\n", pcQuery, oErr->code, oErr->message);
	}
	
	mysql_free_result(pMysqlResult);
	
	return pcRet;
}

static void gmlc_mysql_query_class_init (GmlcMysqlQueryClass * pClass) {
	GObjectClass *pObjClass = G_OBJECT_CLASS(pClass);
	
	pObjClass->finalize = (GObjectFinalizeFunc) gmlc_mysql_query_finalize;
	
	pObjClass->get_property = gmlc_mysql_query_get_property;
	pObjClass->set_property = gmlc_mysql_query_set_property;
	
	g_object_class_install_property(pObjClass, PROP_SERVER, 
		g_param_spec_object("server", "Server object", "Server object", GMLC_MYSQL_TYPE_SERVER, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property(pObjClass, PROP_QUERY, 
		g_param_spec_string("query", "Query string", "Sql Query string", "", G_PARAM_READWRITE));
	g_object_class_install_property(pObjClass, PROP_SRV_CHARSET, 
		g_param_spec_string("srv_charset", "Server charset", "Charset used on server", "", G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(pObjClass, PROP_DB_NAME, 
		g_param_spec_string("db_name", "Database name", "Name of the database", "", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property(pObjClass, PROP_ERR_MSG, 
		g_param_spec_string("err_msg", "Error message", "Error message", "", G_PARAM_READABLE));
	g_object_class_install_property(pObjClass, PROP_ERR_CODE, 
		g_param_spec_int("err_code", "Error code", "Munerical code of error", 0, 65535, 0, G_PARAM_READABLE));
	
}

static void gmlc_mysql_query_init (GmlcMysqlQuery * pGmlcMysqlQry) {
	pGmlcMysqlQry->pGmlcMysqlSrv = NULL;/* r-c */
	pGmlcMysqlQry->pMysqlLink = NULL;	/* --- */
	pGmlcMysqlQry->pMysqlResult = NULL;	/* --- */
	pGmlcMysqlQry->pcQuery = NULL;		/* r-- */
	pGmlcMysqlQry->pcDbName = NULL;		/* r-c */
	pGmlcMysqlQry->pcErrMsg = NULL;		/* r-- */
	pGmlcMysqlQry->iErrCode = 0;		/* r-- */
	pGmlcMysqlQry->iEditResult = 0;		/* r-- */
	pGmlcMysqlQry->bNoRecord = FALSE;	/* r-- */
	pGmlcMysqlQry->lVersion = 0;		/* r-- */
	pGmlcMysqlQry->pcSrvCharset = g_strdup("ISO-8859-1");	/* r-c */
	pGmlcMysqlQry->pMysqlHeaders = NULL; /* --- */
	pGmlcMysqlQry->arMysqlHeaders = NULL;
	pGmlcMysqlQry->pcAbsTableName = NULL;
}

static void gmlc_mysql_query_finalize (GmlcMysqlQuery * pGmlcMysqlQry) {
	
	/* Close DB connection */
	if (pGmlcMysqlQry->pMysqlLink != NULL) {
		mysql_close(pGmlcMysqlQry->pMysqlLink);
	}
	
	/* Release pointers */
	gmlc_mysql_query_read_error(pGmlcMysqlQry, TRUE);
	
	g_free(pGmlcMysqlQry->pcDbName);
	g_free(pGmlcMysqlQry->pcSrvCharset);
	g_free(pGmlcMysqlQry->pcQuery);
	g_free(pGmlcMysqlQry->pcAbsTableName);
	pGmlcMysqlQry->pcAbsTableName = NULL;
}

static void gmlc_mysql_query_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec) {
	GmlcMysqlQuery * pGmlcMysqlQry = GMLC_MYSQL_QUERY(object);
	
	switch (prop_id) {
		case PROP_SERVER :
			pGmlcMysqlQry->pGmlcMysqlSrv = g_value_get_object(value);
			break;
		case PROP_QUERY:
			g_free(pGmlcMysqlQry->pcQuery);
			
			pGmlcMysqlQry->pcQuery = g_value_dup_string(value);
			break;
		case PROP_SRV_CHARSET:
			g_free(pGmlcMysqlQry->pcSrvCharset);
			
			pGmlcMysqlQry->pcSrvCharset = g_value_dup_string(value);
			break;
		case PROP_DB_NAME :
			g_free(pGmlcMysqlQry->pcDbName);
			
			pGmlcMysqlQry->pcDbName = g_value_dup_string(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void gmlc_mysql_query_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec) {
	GmlcMysqlQuery * pGmlcMysqlQry = GMLC_MYSQL_QUERY(object);
	UNUSED_VAR(pGmlcMysqlQry);
	
	switch (prop_id) {
		case PROP_SERVER:
			g_value_set_object(value, pGmlcMysqlQry->pGmlcMysqlSrv);
			break;
		case PROP_QUERY:
			g_value_set_string(value, pGmlcMysqlQry->pcQuery);
			break;
		case PROP_SRV_CHARSET:
			g_value_set_string(value, pGmlcMysqlQry->pcSrvCharset);
			break;
		case PROP_DB_NAME:
			g_value_set_string(value, pGmlcMysqlQry->pcDbName);
			break;
		case PROP_ERR_MSG:
			g_value_set_string(value, pGmlcMysqlQry->pcErrMsg);
			break;
		case PROP_ERR_CODE:
			g_value_set_int(value, pGmlcMysqlQry->iErrCode);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

GmlcMysqlQuery * gmlc_mysql_query_new (GmlcMysqlServer * pGmlcMysqlSrv, const gchar * pcDbName) {
	GmlcMysqlQuery * pGmlcMysqlQry = NULL;
	
	pGmlcMysqlQry = GMLC_MYSQL_QUERY(g_object_new (GMLC_MYSQL_TYPE_QUERY, "server", GMLC_MYSQL_SERVER(pGmlcMysqlSrv), "db_name", pcDbName, NULL));
	
	gmlc_mysql_query_get_version(pGmlcMysqlQry);
	gmlc_mysql_query_get_current_charset(pGmlcMysqlQry);
	
	return pGmlcMysqlQry;
}

GmlcMysqlQuery * gmlc_mysql_query_new_duplicate (GmlcMysqlQuery * pGmlcMysqlBaseQry) {
	GmlcMysqlQuery * pGmlcMysqlQry = NULL;
	GmlcMysqlServer * pGmlcMysqlSrv = NULL;
	gchar * pcDbName = NULL, *pcSqlQuery = NULL;
	
	g_object_get(pGmlcMysqlBaseQry, "server", &pGmlcMysqlSrv, "db_name", &pcDbName, "query", &pcSqlQuery, NULL);
	
	pGmlcMysqlQry = gmlc_mysql_query_new(pGmlcMysqlSrv, pcDbName);
	
	//g_object_set(pGmlcMysqlQry, "query", pcSqlQuery, NULL);
	
	g_free(pcSqlQuery);
	g_free(pcDbName);
	
	return pGmlcMysqlQry;
}

gchar * gmlc_mysql_query_static_get_one_result(GmlcMysqlServer * pGmlcMysqlSrv, const gchar * pcDbName, const gchar * pcQuery, const gint iIdxField) {
	GmlcMysqlQuery * pGmlcMysqlQry = NULL;
	gchar * pcRetValue = NULL;
	
	pGmlcMysqlQry = gmlc_mysql_query_new(pGmlcMysqlSrv, pcDbName);
	
	g_return_val_if_fail(pGmlcMysqlQry != NULL, NULL);
	
	if (pGmlcMysqlQry->pMysqlLink == NULL) {
		gmlc_mysql_query_connect(pGmlcMysqlQry);
	}
	
	g_return_val_if_fail(pGmlcMysqlQry->pMysqlLink != NULL, NULL);
	
	pcRetValue = gmlc_tools_query_get_one_result(pGmlcMysqlQry->pMysqlLink, pGmlcMysqlQry->pcSrvCharset, pcQuery, iIdxField);
	
	g_object_unref(pGmlcMysqlQry);
	
	return pcRetValue;
}

static gboolean gmlc_mysql_query_connect(GmlcMysqlQuery * pGmlcMysqlQry) {
	
	g_return_val_if_fail(pGmlcMysqlQry != NULL, FALSE);
	
	if (pGmlcMysqlQry->pMysqlLink != NULL) {
		return TRUE;
	}
	
	pGmlcMysqlQry->pMysqlLink = mysql_init(NULL);
	if (pGmlcMysqlQry->pMysqlLink == NULL) {
		gmlc_mysql_query_read_error(pGmlcMysqlQry, TRUE);
		pGmlcMysqlQry->iErrCode = -1;
		pGmlcMysqlQry->pcErrMsg = g_strdup("**** Can't prepare mysql connection ****");
		g_printerr("Failed to connect to database: Error (%d) : %s\n", pGmlcMysqlQry->iErrCode, pGmlcMysqlQry->pcErrMsg);
		return FALSE;
	}

	if (!mysql_real_connect(pGmlcMysqlQry->pMysqlLink, pGmlcMysqlQry->pGmlcMysqlSrv->pcHost, 
			pGmlcMysqlQry->pGmlcMysqlSrv->pcLogin, pGmlcMysqlQry->pGmlcMysqlSrv->pcPassword, 
			pGmlcMysqlQry->pcDbName, pGmlcMysqlQry->pGmlcMysqlSrv->iPort, 
			pGmlcMysqlQry->pGmlcMysqlSrv->pcLocalSock, CLIENT_MULTI_STATEMENTS)
			) {
		gmlc_mysql_query_read_error(pGmlcMysqlQry, FALSE);
		pGmlcMysqlQry->pMysqlLink = NULL;
		g_printerr("Failed to connect to database: Error (%d) : %s\n", pGmlcMysqlQry->iErrCode, pGmlcMysqlQry->pcErrMsg);
		return FALSE;
	}
	
	return TRUE;
}

gboolean gmlc_mysql_query_execute(GmlcMysqlQuery * pGmlcMysqlQry, const gchar * pcQuery, gsize szQuery, gboolean bForceWrite) {
	GError * oErr = NULL;
	gchar * pcTmp = NULL;
	gsize szTmp = 0;
	gint iErrCode = 0;
	gboolean bIsReadQuery = FALSE;
	
	g_return_val_if_fail(pGmlcMysqlQry != NULL, FALSE);
	
	pcTmp = g_convert(pcQuery, szQuery, pGmlcMysqlQry->pcSrvCharset, "UTF-8", NULL, &szTmp, &oErr);
	if (pcTmp == NULL && oErr != NULL) {
		g_printerr("gmlc_mysql_query_execute - g_convert - Failed to convert query\nQuery : '%s'\nError: (%d) %s\n", pcQuery, oErr->code, oErr->message);
		return FALSE;
	}
	
	g_free(pGmlcMysqlQry->pcQuery);
	pGmlcMysqlQry->pcQuery = pcTmp;
	pGmlcMysqlQry->szQuery = szTmp;
	
	
	gmlc_mysql_query_connect(pGmlcMysqlQry);

	if (pGmlcMysqlQry->pMysqlLink == NULL) {
		return FALSE;
	}
	
	bIsReadQuery = gmlc_tools_query_is_read_query(pcQuery, szQuery);
	
	if (pGmlcMysqlQry->pGmlcMysqlSrv->bReadOnly && !bIsReadQuery) {
		gmlc_mysql_query_read_error(pGmlcMysqlQry, TRUE);
		pGmlcMysqlQry->iErrCode = -1000;
		pGmlcMysqlQry->pcErrMsg = g_strdup("gmysqlcc : Server in Read-only mode");
		return FALSE;
	}
	
	if (pGmlcMysqlQry->pGmlcMysqlSrv->bWriteWarning && !bIsReadQuery) {
		if (!bForceWrite) {
			gmlc_mysql_query_read_error(pGmlcMysqlQry, TRUE);
			pGmlcMysqlQry->iErrCode = -1001;
			pGmlcMysqlQry->pcErrMsg = g_strdup("gmysqlcc : Server in write warning mode");
			return FALSE;
		}
	}
	
	iErrCode = mysql_real_query(pGmlcMysqlQry->pMysqlLink, pGmlcMysqlQry->pcQuery, pGmlcMysqlQry->szQuery);
	
	if (iErrCode != 0) {
		gmlc_mysql_query_read_error(pGmlcMysqlQry, FALSE);
		g_printerr("gmlc_mysql_query_execute - mysql_real_query - Failed to query\nQuery : '%s'\nError: (%d - %d) %s\n", pGmlcMysqlQry->pcQuery, iErrCode, pGmlcMysqlQry->iErrCode, pGmlcMysqlQry->pcErrMsg);
		return FALSE;
	}
	
	pGmlcMysqlQry->iNbField = mysql_field_count(pGmlcMysqlQry->pMysqlLink);

	if (iErrCode != 0 && pGmlcMysqlQry->iNbField < 0) {
		gmlc_mysql_query_read_error(pGmlcMysqlQry, FALSE);
		g_printerr("gmlc_mysql_query_execute - mysql_field_count - Failed to query : '%s'\nError: (%d - %d) %s\n", pGmlcMysqlQry->pcQuery, iErrCode, pGmlcMysqlQry->iErrCode, pGmlcMysqlQry->pcErrMsg);
		return FALSE;
	} else if (iErrCode == 0 && pGmlcMysqlQry->iNbField == 0) {
		pGmlcMysqlQry->iEditResult = mysql_affected_rows(pGmlcMysqlQry->pMysqlLink);
		pGmlcMysqlQry->bNoRecord = TRUE;
		gmlc_mysql_query_read_error(pGmlcMysqlQry, TRUE);
		return TRUE;
	}
	
	return TRUE;
}

gboolean gmlc_mysql_query_have_record(GmlcMysqlQuery * pGmlcMysqlQry) {
	
	g_return_val_if_fail(pGmlcMysqlQry != NULL, FALSE);
	
	return (!pGmlcMysqlQry->bNoRecord);
}

GArray * gmlc_mysql_query_next_record(GmlcMysqlQuery * pGmlcMysqlQry) {
	GArray * arRecordValue = NULL;
	GError * oErr = NULL;
	MYSQL_ROW pCurrRow;
	gchar * pcConverted = NULL;
	gulong * arlSizes = NULL;
	gsize szConverted = 0;
	int i = 0;
		
	g_return_val_if_fail(pGmlcMysqlQry != NULL, FALSE);
	
	if (pGmlcMysqlQry->pMysqlResult == NULL) {
		if (pGmlcMysqlQry->bNoRecord) {
			gmlc_mysql_query_read_error(pGmlcMysqlQry, TRUE);
			return NULL;
		}
		
		pGmlcMysqlQry->pMysqlResult = mysql_use_result(pGmlcMysqlQry->pMysqlLink);
		
		if (pGmlcMysqlQry->pMysqlResult == NULL) {
			gmlc_mysql_query_read_error(pGmlcMysqlQry, FALSE);
			g_printerr("gmlc_mysql_query_next_record - mysql_use_result - Failed\nQuery : '%s'\nError: (%d) %s\n", pGmlcMysqlQry->pcQuery, pGmlcMysqlQry->iErrCode, pGmlcMysqlQry->pcErrMsg);
			return NULL;
		}
		
		pGmlcMysqlQry->pMysqlHeaders = mysql_fetch_fields(pGmlcMysqlQry->pMysqlResult);
		if (pGmlcMysqlQry->pMysqlHeaders == NULL) {
			gmlc_mysql_query_read_error(pGmlcMysqlQry, FALSE);
			g_printerr("gmlc_mysql_query_execute - mysql_fetch_fields - Failed to query : '%s'\nError: (%d) %s\n", pGmlcMysqlQry->pcQuery, pGmlcMysqlQry->iErrCode, pGmlcMysqlQry->pcErrMsg);
		}
		
		pGmlcMysqlQry->bNoRecord = FALSE;
	}
	
	pCurrRow = mysql_fetch_row(pGmlcMysqlQry->pMysqlResult);
	if (pCurrRow == NULL) {
		gmlc_mysql_query_read_error(pGmlcMysqlQry, FALSE);
		if (pGmlcMysqlQry->iErrCode != 0) {
			g_printerr("gmlc_mysql_query_next_record - mysql_fetch_row - Failed\nQuery : '%s'\nError: (%d) %s\n", pGmlcMysqlQry->pcQuery, pGmlcMysqlQry->iErrCode, pGmlcMysqlQry->pcErrMsg);
		}
		return NULL;
	}
	
	arlSizes = mysql_fetch_lengths(pGmlcMysqlQry->pMysqlResult);
	if (pCurrRow == NULL) {
		gmlc_mysql_query_read_error(pGmlcMysqlQry, FALSE);
		g_printerr("gmlc_mysql_query_next_record - mysql_fetch_lengths - Failed\nQuery : '%s'\nError: (%d) %s\n", pGmlcMysqlQry->pcQuery, pGmlcMysqlQry->iErrCode, pGmlcMysqlQry->pcErrMsg);
		return NULL;
	}
	
	arRecordValue = g_array_sized_new (FALSE, TRUE, sizeof (gchar *), pGmlcMysqlQry->iNbField);
	
	for(i = 0; i < pGmlcMysqlQry->iNbField; i++) {
		if (pCurrRow[i] != NULL) {
			pcConverted = g_convert(pCurrRow[i], arlSizes[i], "UTF-8", pGmlcMysqlQry->pcSrvCharset, NULL, &szConverted, &oErr);
			if (pcConverted == NULL) {
				pcConverted = g_strdup("Binary BLOB");
			}
		} else {
			pcConverted = g_strdup("NULL");
		}
		
		g_array_append_val (arRecordValue, pcConverted);
	}
	
	return arRecordValue;
}

void gmlc_mysql_query_free_record_content(GArray * arRow) {
	gint i;
	
	for (i = 0; i < arRow->len; i++) {
		g_free(g_array_index(arRow, gchar *, i));
	}
	
	g_array_free(arRow, TRUE);
}

GArray * gmlc_mysql_query_get_headers(GmlcMysqlQuery * pGmlcMysqlQry, gboolean bDuplicate) {
	MYSQL_FIELD * pField = NULL;
	GArray * arHeaders = NULL;
	GError * oErr = NULL;
	gchar * pcTmp = NULL;
	gsize szTmp = 0;
	int i = 0;
	
	g_return_val_if_fail(pGmlcMysqlQry != NULL, NULL);
	
	if (pGmlcMysqlQry->iNbField == 0) {
		return NULL;
	}
	
	if (pGmlcMysqlQry->pMysqlHeaders == NULL) {
		return NULL;
	}
	
	if (pGmlcMysqlQry->arMysqlHeaders == NULL) {
		pGmlcMysqlQry->arMysqlHeaders = g_array_sized_new (FALSE, TRUE, sizeof (char *), pGmlcMysqlQry->iNbField);
		
		for(i = 0; i < pGmlcMysqlQry->iNbField; i++) {
			pField = &pGmlcMysqlQry->pMysqlHeaders[i];
			pcTmp = g_convert(pField->name, strlen(pField->name), "UTF-8", pGmlcMysqlQry->pcSrvCharset, NULL, &szTmp, &oErr);
			if (pcTmp == NULL && oErr != NULL) {
				g_printerr("gmlc_mysql_query_get_headers - g_convert - Failed to convert header\nHeader : '%s'\nError: (%d) %s\n", pField->name, oErr->code, oErr->message);
				pcTmp = g_strdup(pField->name);
			}
			
			g_array_append_val (pGmlcMysqlQry->arMysqlHeaders, pcTmp);
		}
	}
	
	if (bDuplicate) {
		arHeaders = g_array_sized_new (FALSE, TRUE, sizeof (char *), pGmlcMysqlQry->iNbField);
		
		for(i = 0; i < pGmlcMysqlQry->arMysqlHeaders->len; i++) {
			pcTmp = g_strdup(g_array_index(pGmlcMysqlQry->arMysqlHeaders, gchar *, i));
			g_array_append_val (arHeaders, pcTmp);
		}
		
		return arHeaders;
	} else {
		return pGmlcMysqlQry->arMysqlHeaders;
	}
}

gboolean gmlc_mysql_query_have_more_result(GmlcMysqlQuery * pGmlcMysqlQry) {
	
	g_return_val_if_fail(pGmlcMysqlQry != NULL, FALSE);
	
	return mysql_more_results(pGmlcMysqlQry->pMysqlLink);
}

gboolean gmlc_mysql_query_goto_next_result(GmlcMysqlQuery * pGmlcMysqlQry) {
	gint iNextRes = 0;
	
	g_return_val_if_fail(pGmlcMysqlQry != NULL, FALSE);
	
	gmlc_mysql_query_free_result(pGmlcMysqlQry);
	
	iNextRes = mysql_next_result(pGmlcMysqlQry->pMysqlLink);
	
	if (iNextRes < 0) {
		pGmlcMysqlQry->iEditResult = 0;
		pGmlcMysqlQry->bNoRecord = TRUE;
		gmlc_mysql_query_read_error(pGmlcMysqlQry, TRUE);
		return FALSE;
	}
	
	if (iNextRes > 0) {
		gmlc_mysql_query_read_error(pGmlcMysqlQry, FALSE);
		g_printerr("gmlc_mysql_query_goto_next_result - mysql_next_result - Failed\nQuery : '%s'\nError: (%d) %s\n", pGmlcMysqlQry->pcQuery, pGmlcMysqlQry->iErrCode, pGmlcMysqlQry->pcErrMsg);
		return FALSE;
	}

	pGmlcMysqlQry->iNbField = mysql_field_count(pGmlcMysqlQry->pMysqlLink);

	if (iNextRes > 0 && pGmlcMysqlQry->iNbField < 0) {
		gmlc_mysql_query_read_error(pGmlcMysqlQry, FALSE);
		g_printerr("gmlc_mysql_query_goto_next_result - mysql_field_count - Failed to query : '%s'\nError: (%d - %d) %s\n", pGmlcMysqlQry->pcQuery, iNextRes, pGmlcMysqlQry->iErrCode, pGmlcMysqlQry->pcErrMsg);
		return FALSE;
	} else if (iNextRes == 0 && pGmlcMysqlQry->iNbField == 0) {
		pGmlcMysqlQry->iEditResult = mysql_affected_rows(pGmlcMysqlQry->pMysqlLink);
		pGmlcMysqlQry->bNoRecord = TRUE;
		gmlc_mysql_query_read_error(pGmlcMysqlQry, TRUE);
		return TRUE;
	}
	
	return (iNextRes == 0) ? TRUE : FALSE ;
}

gboolean gmlc_mysql_query_free_result (GmlcMysqlQuery * pGmlcMysqlQry) {
	gchar * pcTmp = NULL;
	gint i = 0;
	
	g_return_val_if_fail(pGmlcMysqlQry != NULL, FALSE);
	
	if (pGmlcMysqlQry->pMysqlResult != NULL || (pGmlcMysqlQry->pMysqlResult == NULL && pGmlcMysqlQry->bNoRecord == TRUE)) {
		mysql_free_result(pGmlcMysqlQry->pMysqlResult);
		
		if (pGmlcMysqlQry->arMysqlHeaders != NULL) {
			for (i = 0; i < pGmlcMysqlQry->arMysqlHeaders->len; i ++) {
				pcTmp = g_array_index(pGmlcMysqlQry->arMysqlHeaders, gchar *, i);
				g_free(pcTmp);
			}
			g_array_free(pGmlcMysqlQry->arMysqlHeaders, TRUE);
		}
		
		pGmlcMysqlQry->pMysqlResult = NULL;
		pGmlcMysqlQry->pMysqlHeaders = NULL;
		pGmlcMysqlQry->arMysqlHeaders = NULL;
		pGmlcMysqlQry->iNbField = 0;
		pGmlcMysqlQry->bNoRecord = FALSE;
		pGmlcMysqlQry->iEditResult = 0;
		g_free(pGmlcMysqlQry->pcAbsTableName);
		pGmlcMysqlQry->pcAbsTableName = NULL;
		gmlc_mysql_query_read_error(pGmlcMysqlQry, TRUE);
	}
	
	return TRUE;
}

gulong gmlc_mysql_query_get_version(GmlcMysqlQuery * pGmlcMysqlQry) {
	
	g_return_val_if_fail(pGmlcMysqlQry != NULL, 0);
	
	if (pGmlcMysqlQry->lVersion != 0) {
		return pGmlcMysqlQry->lVersion;
	}
	
	if (pGmlcMysqlQry->pMysqlLink == NULL) {
		gmlc_mysql_query_connect(pGmlcMysqlQry);
	}
	
	if (pGmlcMysqlQry->pMysqlLink == NULL) {
		return 0;
	}
	
	pGmlcMysqlQry->lVersion = mysql_get_server_version(pGmlcMysqlQry->pMysqlLink);
	
	return pGmlcMysqlQry->lVersion;
}

gchar * gmlc_mysql_query_get_current_charset(GmlcMysqlQuery * pGmlcMysqlQry) {
	gchar * pcCharset = NULL;
	
	g_return_val_if_fail(pGmlcMysqlQry != NULL, NULL);
	
	if (pGmlcMysqlQry->pMysqlLink == NULL) {
		gmlc_mysql_query_connect(pGmlcMysqlQry);
	}
	
	if (pGmlcMysqlQry->pMysqlLink == NULL) {
		return NULL;
	}
	
	pcCharset = gmlc_tools_query_get_one_result(pGmlcMysqlQry->pMysqlLink, "ISO-8859-1", "SHOW LOCAL VARIABLES LIKE 'character_set_results'", 1);
	
	if (pcCharset != NULL) {
		g_free(pGmlcMysqlQry->pcSrvCharset);
		pGmlcMysqlQry->pcSrvCharset = pcCharset;
	}
	
	return pGmlcMysqlQry->pcSrvCharset;
}

void gmlc_mysql_query_read_error(GmlcMysqlQuery * pGmlcMysqlQry, gboolean bClearOnly) {
	
	g_return_if_fail(pGmlcMysqlQry != NULL);
	
	if (pGmlcMysqlQry->iErrCode < 0) {
		g_free(pGmlcMysqlQry->pcErrMsg);
	}
	
	if (bClearOnly) {
		pGmlcMysqlQry->iErrCode = 0;
		pGmlcMysqlQry->pcErrMsg = NULL;
	} else {
		pGmlcMysqlQry->iErrCode = mysql_errno(pGmlcMysqlQry->pMysqlLink);
		pGmlcMysqlQry->pcErrMsg = (gchar *)mysql_error(pGmlcMysqlQry->pMysqlLink);
	}

}

gchar * gmlc_mysql_query_get_absolute_table_name (GmlcMysqlQuery * pGmlcMysqlQry, gboolean bOnlyTableName) {
	MYSQL_FIELD * field = NULL;
	gchar * pcTblName = NULL, * pcAbsTblName = NULL, * pcAbsTblNameOld = NULL;
	int i = 0;
	
	if (pGmlcMysqlQry->pcAbsTableName != NULL && !bOnlyTableName) {
		return g_strdup(pGmlcMysqlQry->pcAbsTableName);
	}

	if (pGmlcMysqlQry->pMysqlHeaders == NULL) {
		return NULL;
	}

	pcAbsTblName = NULL;
	pcAbsTblNameOld = NULL;
	
	for(i = 0; i < pGmlcMysqlQry->iNbField; i++) {
		g_free(pcAbsTblName);
		
		field = &pGmlcMysqlQry->pMysqlHeaders[i];
		if (bOnlyTableName) {
			pcAbsTblName = g_strdup_printf("%s", field->table);
		} else {
			pcAbsTblName = g_strdup_printf("`%s`.`%s`", pGmlcMysqlQry->pcDbName, field->table);
		}
		
		if (pcAbsTblNameOld != NULL) {
			
			if (g_ascii_strcasecmp(pcAbsTblName, pcAbsTblNameOld) != 0) {
				g_free(pGmlcMysqlQry->pcAbsTableName);
				pGmlcMysqlQry->pcAbsTableName = NULL;
				g_free(pcAbsTblName);
				g_free(pcAbsTblNameOld);
				return NULL;
			}
			
		} else {
			pcAbsTblNameOld = g_strdup(pcAbsTblName);
		}
	}
	
	pcTblName = pcAbsTblName;
	g_free(pcAbsTblNameOld);
	
	if (!bOnlyTableName) {
		pGmlcMysqlQry->pcAbsTableName = g_strdup(pcTblName);
	}
	
	return pcTblName;
}

gchar * gmlc_mysql_query_get_primary_where (GmlcMysqlQuery * pGmlcMysqlQry, GArray * arDatas) {
	MYSQL_FIELD * field = NULL;
	GString * strWhere = NULL;
	GArray * arHeaders = NULL;
	gchar * pcRetStr = NULL;
	int i = 0;
	
	if ((arHeaders = gmlc_mysql_query_get_headers(pGmlcMysqlQry, FALSE)) == NULL) {
		return NULL;
	}
	
	strWhere = g_string_new("");
	
	for(i = 0; i < pGmlcMysqlQry->iNbField; i++) {
		
		field = &pGmlcMysqlQry->pMysqlHeaders[i];
		if (field->flags & PRI_KEY_FLAG) {
			if (strWhere->len == 0) {
				g_string_append_printf(strWhere, "`%s` = '%s'", field->name, g_array_index(arDatas, gchar *, i));
			} else {
				g_string_append_printf(strWhere, " AND `%s` = '%s'", field->name, g_array_index(arDatas, gchar *, i));
			}
		}
	}
	
	pcRetStr = strWhere->str;
	g_string_free(strWhere, FALSE);
	return pcRetStr;
}

gboolean gmlc_mysql_query_is_editable (GmlcMysqlQuery * pGmlcMysqlQry) {
	g_free(gmlc_mysql_query_get_absolute_table_name(pGmlcMysqlQry, FALSE));
	
	return pGmlcMysqlQry->pcAbsTableName != NULL && !pGmlcMysqlQry->pGmlcMysqlSrv->bReadOnly && !pGmlcMysqlQry->pGmlcMysqlSrv->bWriteWarning ;
}
