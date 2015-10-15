/*
** Authors: Simone Mutti <simone.mutti@unibg.it>
**          Enrico Bacis <enrico.bacis@unibg.it>
**
** Copyright 2015, Università degli Studi di Bergamo
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#include "sesqlite.h"

/*
** Parameter zTab is the name of a table that is about to be used in a SQL
** If the table is a system table (e.g., sqlite_, selinux_), this function 
** return 0, otherwise 1 is returned.
*/
int isRewritableTable(char *zTab){

	if(0 != sqlite3StrNICmp(zTab, "sqlite_", 7) && 
		0 != sqlite3StrNICmp(zTab, "selinux_", 8) ){
		return 1;
	}
	return 0;
}

/**
 * Create the function used to rewrite the SQL statement.
 *
 */
Expr* initRewriteExpr(Parse *pParse, int action, char *zName){

	sqlite3 *db = pParse->db;

	Expr *pFName = sqlite3Expr(db, TK_FUNCTION, SECURITY_CONTEXT_FUNCTION);
	Expr *pFTable = sqlite3Expr(db, TK_ID, zName);
	Expr *pFColumn = sqlite3Expr(db, TK_ID, SECURITY_CONTEXT_COLUMN_NAME);
	Expr *pFClass = sqlite3Expr(db, TK_STRING, SECURITY_CONTEXT_CLASS);
	Expr *pFDebug = sqlite3Expr(db, TK_STRING, zName);

	Expr *pFAction = NULL;
	switch(action){
		case 0:
			pFAction = sqlite3Expr(db, TK_STRING, 
					SECURITY_CONTEXT_ACTION_SELECT);
			break;
		case 1:
			pFAction = sqlite3Expr(db, TK_STRING, 
					SECURITY_CONTEXT_ACTION_UPDATE);
			break;
		case 2:
			pFAction = sqlite3Expr(db, TK_STRING, 
					SECURITY_CONTEXT_ACTION_DELETE);
			break;
	}

	Expr *pFunction = sqlite3ExprAlloc(db, TK_DOT, 0, 0);

	sqlite3ExprAttachSubtrees(db, pFunction, pFTable, pFColumn);

	ExprList *pExprFunction;
	pExprFunction = sqlite3ExprListAppend(pParse, 0, pFunction);
	pExprFunction = sqlite3ExprListAppend(pParse, pExprFunction, pFClass);
	pExprFunction = sqlite3ExprListAppend(pParse, pExprFunction, pFAction);
	pExprFunction = sqlite3ExprListAppend(pParse, pExprFunction, pFDebug);

	pFName->x.pList = pExprFunction;
	sqlite3ExprSetHeight(pParse, pFName);

	return pFName;
}

int sesqlite_rewrite_select(Parse *pParse,
		SrcList *pSrc,
		Expr **pWhere){

	int i = 0;
	int rc = SQLITE_OK;
	sqlite3 *db = pParse->db;

	if( (db->openFlags & SQLITE_OPEN_READONLY) != 0){
		return rc;
	}

	for(i = 0; i < pSrc->nAlloc; i++){
		char *zName = NULL;
		if( pSrc->a[i].zAlias )
			zName = pSrc->a[i].zAlias;
		else
			zName = pSrc->a[i].zName;

		if( !isRewritableTable(zName) )
			continue;

		Expr *pRewrite = initRewriteExpr(pParse, 0, zName);

		if(*pWhere)
			*pWhere = sqlite3ExprAnd(db, pRewrite, *pWhere);
		else
			*pWhere = pRewrite;
	}

	return rc;
}

int sesqlite_rewrite_insert(Parse *pParse, 
		Table *pTab,
		Select *pSelect,
		IdList *pColumn,
		ExprList *pList){


	int rc = SQLITE_OK;
	sqlite3 *db = pParse->db;
	int iDb = 0;
	char *zDb = NULL;
	char *zTab = pTab->zName;
	Db *pDb = NULL;

	if( !isRewritableTable(zTab) &&
		(db->openFlags & SQLITE_OPEN_READONLY) == 0){
		return rc;
	}

	iDb = sqlite3SchemaToIndex(db, pTab->pSchema);
	assert( iDb<db->nDb );
	pDb = &db->aDb[iDb];
	zDb = pDb->zName;

	/* if the insert statement is in the following form:
	 * 'insert into TABLE (IDLIST) ...'
	*/
	if(pColumn){
		int idx;
		pColumn->a = sqlite3ArrayAllocate(
			db,
			pColumn->a,
			sizeof(pColumn->a[0]),
			&pColumn->nId,
			&idx
		);
		sqlite3SetString(&pColumn->a[idx].zName, 
			 db,
			"%s",
			SECURITY_CONTEXT_COLUMN_NAME);
	}

	/* create expression to inject */
	Expr *pValue = sqlite3Expr(db, TK_INTEGER, 0);
	pValue->flags |= EP_IntValue;
	pValue->u.iValue = lookup_security_context(hash_id, 
			(char *) zDb, 
			zTab);

	if(pSelect){
		/* if the insert statement is in the following form:
		 * 'insert into TABLE SELECT ...'
		*/
		sqlite3ExprListAppend(pParse, pSelect->pEList, pValue);

		/* if the insert statement is in the following form:
		 * 'insert into TABLE VALUES (EXPRLIST)'
		*/
		if(pSelect->pPrior){
			Select *pPrior;
			pPrior = pSelect->pPrior;
			while(pPrior != NULL){
				Expr *pPriorValue = sqlite3Expr(db, TK_INTEGER, 0);
				pPriorValue->flags |= EP_IntValue;
				pPriorValue->u.iValue = lookup_security_context(hash_id, 
						(char *) zDb, 
						zTab);
				sqlite3ExprListAppend(pParse, pPrior->pEList, pPriorValue);
				pPrior = pPrior->pPrior; 
			}
		}
	}else{
		/* if the insert statement is in the following form:
		 * 'insert into TABLE VALUES (EXPR)'
		*/
		pList = sqlite3ExprListAppend(pParse, pList, pValue);
	}

	return rc;
}


int sesqlite_rewrite_update(Parse *pParse,
		SrcList *pTabList,
		Expr **pWhere){

	int rc = SQLITE_OK;
	sqlite3 *db = pParse->db;
	Table *pTab = sqlite3SrcListLookup(pParse, pTabList);

	if( !isRewritableTable(pTab->zName) &&
		(db->openFlags & SQLITE_OPEN_READONLY) == 0){
		return rc;
	}

	int i = 0;
	for(i = 0; i < pTabList->nAlloc; i++){
		Expr *pRewrite = initRewriteExpr(pParse, 1, pTabList->a[i].zName);

		if(*pWhere)
			*pWhere = sqlite3ExprAnd(db, pRewrite, *pWhere);
		else
			*pWhere = pRewrite;
	}

	return rc;
}

int sesqlite_rewrite_delete(Parse *pParse,
		SrcList *pTabList,
		Expr **pWhere){

	int rc = SQLITE_OK;
	sqlite3 *db = pParse->db;
	Table *pTab = sqlite3SrcListLookup(pParse, pTabList);

	if( !isRewritableTable(pTab->zName) &&
		(db->openFlags & SQLITE_OPEN_READONLY) == 0){
		return rc;
	}

	int i = 0;
	for(i = 0; i < pTabList->nAlloc; i++){
		Expr *pRewrite = initRewriteExpr(pParse, 2, pTabList->a[i].zName);

		if(*pWhere)
			*pWhere = sqlite3ExprAnd(db, pRewrite, *pWhere);
		else
			*pWhere = pRewrite;
	}

	return rc;
}

#endif
