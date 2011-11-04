#include <mysql/mysql.h>
#include <string.h>
#include <stdint.h>
#include "dbrelated.h"


static char *db_host, *db_username, *db_password, *db_schema, *db_table;

static MYSQL connection;
static MYSQL_STMT *statement;
static MYSQL_BIND params[5];
static char* sql;


//--------------------------------
void db_init(struct arguments *arguments){
	db_host = arguments->db_host;
	db_username = arguments->db_username;
	db_password = arguments->db_password;
	db_table = arguments->db_table;
	db_schema = arguments->db_schema;

}

void db_connect(){
	mysql_init(&connection);
	mysql_real_connect(&connection, db_host, db_username, db_password, db_schema, 0, NULL, 0);
}

void db_disconnect(){
	mysql_close(&connection);
}
//---------------------------------


void db_prepare_statement(){
	sql = malloc(100 + strlen(db_schema) + strlen(db_table));
	sprintf(
			sql,
			"INSERT INTO %s "//18
			"(TIMESTAMP, MAC_FROM, MAC_TO, CONTENT, PAYLOAD) "///60
			"VALUES(?,?,?,?,?)",
			db_table
			);

	statement = mysql_stmt_init(&connection);
	int res = mysql_stmt_prepare(statement, sql, strlen(sql));
	if (res) {
		printf("ERROR: %s\n",mysql_stmt_error(statement));
		exit(1);
	}

	memset(&params, 0, sizeof(params));

	params[0].buffer_type = MYSQL_TYPE_TIMESTAMP;
	params[0].is_null;
	params[0].buffer_length = 32;

	params[1].buffer_type = MYSQL_TYPE_VARCHAR;
	params[1].is_null = 0;
	params[1].buffer_length = 24L;

	params[2].buffer_type = MYSQL_TYPE_VARCHAR;
	params[2].is_null = 0;
	params[2].buffer_length = 24L;

	params[3].buffer_type = MYSQL_TYPE_VARCHAR;
	params[3].is_null = 0;

	params[4].buffer_type = MYSQL_TYPE_BLOB;

}

void db_close_statement(){
	mysql_stmt_close(statement);
	free(sql);
}


int db_execute_statement(int timestamp, char *mac_from, char *mac_to, char *headers, uint8_t *payload, uint32_t payload_length){
	params[0].buffer = &timestamp;
	params[1].buffer = mac_from;
	params[2].buffer = mac_to;

	params[3].buffer = headers;
	params[3].buffer_length = (unsigned long)strlen(headers);

	if(payload && payload_length){
		params[4].buffer = payload;
		params[4].buffer_length = payload_length;
		params[4].is_null = 0;
	} else {
		params[4].buffer = 0;
		params[4].is_null = (my_bool*)1;
		params[4].length = 0;
	}

	mysql_stmt_bind_param(statement, params);
	int res = mysql_stmt_execute(statement);
	if (res) {
		printf("ERROR: %s\n",mysql_stmt_error(statement));
	} else {
		mysql_stmt_free_result(statement);
		mysql_commit(&connection);
	}
	return res;
}

