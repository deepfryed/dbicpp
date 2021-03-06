#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <mysql/mysql.h>
#include <mysql/mysql_com.h>
#include <getopt.h>
#include <string>
#include <cstring>
#include <vector>
#include <unistd.h>

using namespace std;

long max_iter = 1;
FILE* outfile = stdout;
vector<string> bind;
char sql[4096], driver[4096];

#define MYSQL_ERROR(c) {\
    printf("ERROR: %s\n", mysql_error(c)); \
    mysql_close(c); \
    exit(1); \
}

#define MYSQL_STMT_ERROR(s) {\
    printf("ERROR: %s\n", mysql_stmt_error(s)); \
    exit(1); \
}

#define MYSQL_DATA(r, c) (*(result[c].is_null) ? "(nil)" : result[c].buffer)

void parseOptions(int argc, char **argv) {
    int option_index, c;
    static struct option long_options[] = {
        { "sql",    required_argument, 0, 's' },
        { "bind",   required_argument, 0, 'b' },
        { "num",    required_argument, 0, 'n' },
        { "driver", required_argument, 0, 'd' },
        { "output", optional_argument, 0, 'o' }
    };

    max_iter = 1;
    strcpy(driver, "mysql");

    while ((c = getopt_long(argc, argv, "s:b:n:d:o:", long_options, &option_index)) >= 0) {
        if (c == 0) {
            c = long_options[option_index].val;
        }
        switch(c) {
            case 's': strcpy(sql, optarg); break;
            case 'b': bind.push_back(optarg); break;
            case 'n': max_iter = atol(optarg); break;
            case 'd': strcpy(driver, optarg); break;
            case 'o': outfile = fopen(optarg, "w");
                      if (!outfile) { perror("Unabled to open file"); exit(1); }
                      break;
            default: exit(1);
        }
    }
}

void error (const char *msg) {
    printf("ERROR: %s\n", msg);
    exit(1);
}

int main(int argc, char*argv[]) {
    int i, c, n, rc, cols, rows;
    unsigned long length;
    parseOptions(argc, argv);
    MYSQL *conn;
    MYSQL_STMT *stmt;
    MYSQL_BIND *params, *result;

    my_bool MYSQL_TRUE  = 1;
    my_bool MYSQL_FALSE = 0;


    conn = mysql_init(0);

    if (!mysql_real_connect(conn, "127.0.0.1", getlogin(), "", "dbicpp", 3306, 0, 0))
        MYSQL_ERROR(conn);

    stmt = mysql_stmt_init(conn);
    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0)
        MYSQL_ERROR(conn);

    cols = (int) mysql_stmt_field_count(stmt);

    result = new MYSQL_BIND[cols];

    for (i = 0; i < cols; i++) {
        memset(&result[i], 0, sizeof(MYSQL_BIND));
        result[i].buffer        = (void*) new unsigned char[8192];
        result[i].buffer_length = 8192;
        result[i].is_null       = new my_bool;
        result[i].length        = new unsigned long;
    }

    if (mysql_stmt_bind_result(stmt, result) != 0)
        MYSQL_STMT_ERROR(stmt);

    vector<unsigned long> bs;
    bs.reserve(cols);

    for (n = 0; n < max_iter; n++) {
        bs.clear();
        for (c = 0; c < cols; c++)
            bs.push_back(result[c].buffer_length);

        params = new MYSQL_BIND[bind.size()];
        for (i = 0; i < (int)bind.size(); i++) {
            memset(&params[i], 0, sizeof(MYSQL_BIND));
            params[i].buffer        = (void *)bind[i].data();
            params[i].buffer_length = bind[i].length();
            params[i].is_null       = &MYSQL_FALSE;
            params[i].buffer_type   = MYSQL_TYPE_STRING;
        }

        if (bind.size() > 0 && mysql_stmt_bind_param(stmt, params) != 0)
            MYSQL_STMT_ERROR(stmt);

        if (mysql_stmt_execute(stmt) != 0)
            MYSQL_STMT_ERROR(stmt);

        if (mysql_stmt_store_result(stmt) != 0)
            MYSQL_STMT_ERROR(stmt);

        delete [] params;
        rows = (int)mysql_stmt_num_rows(stmt);

        for (i = 0; i < rows; i++) {
            rc = mysql_stmt_fetch(stmt);
            if (rc != 0 && rc != MYSQL_DATA_TRUNCATED)
                MYSQL_STMT_ERROR(stmt);

            for (c = 0; c < cols; c++) {
                length = *result[c].length;
                if (length >= bs[c]) {
                    if (result[c].buffer_length <= length) {
                        delete [] (unsigned char*)result[c].buffer;
                        result[c].buffer = new unsigned char[length + 1];
                        result[c].buffer_length = length + 1;
                    }
                    mysql_stmt_fetch_column(stmt, &result[c], c, 0);
                }
                ((unsigned char*)result[c].buffer)[*result[c].length] = 0;
                fprintf(outfile, "%s\t", (unsigned char*)MYSQL_DATA(result, c));
            }
            fprintf(outfile, "\n");
        }

        mysql_stmt_free_result(stmt);
    }


    mysql_close(conn);
}
