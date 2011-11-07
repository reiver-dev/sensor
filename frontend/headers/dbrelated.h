#ifndef DBRELATED_H
#define	DBRELATED_H
#include "sensord.h"
#include <mysql/mysql.h>

void db_init(struct arguments *arguments);

void db_connect();
void db_disconnect();

void db_prepare_statement();
int db_execute_statement();

void db_close_statement();

#endif	/* DBRELATED_H */
