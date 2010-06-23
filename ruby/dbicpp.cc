#include <dbic++.h>
#include <ruby/ruby.h>
#include <ruby/io.h>

#define ID_CONST_GET rb_intern("const_get")
#define CONST_GET(scope, constant) (rb_funcall(scope, ID_CONST_GET, 1, rb_str_new2(constant)))

static VALUE mDBI;
static VALUE cHandle;
static VALUE cStatement;
static VALUE eRuntimeError;
static VALUE eArgumentError;
static VALUE eStandardError;
static VALUE fStringify;

#define CSTRING(v) RSTRING_PTR(TYPE(v) == T_STRING ? v : rb_funcall(v, fStringify, 0))
#define EXCEPTION(type) (std::exception &e) { rb_raise(eRuntimeError, "%s Error: %s", type, e.what()); }

static dbi::Handle* DBI_HANDLE(VALUE self) {
    dbi::Handle *h;
	Data_Get_Struct(self, dbi::Handle, h);
    return h;
}

static dbi::Statement* DBI_STATEMENT(VALUE self) {
    dbi::Statement *st;
	Data_Get_Struct(self, dbi::Statement, st);
    return st;
}

static void free_connection(dbi::Handle *self) {
    delete self;
}

static void free_statement(dbi::Statement *self) {
    delete self;
}

VALUE rb_dbi_init(VALUE self, VALUE path) {
    try { dbi::dbiInitialize(RSTRING_PTR(path)); } catch EXCEPTION("Invalid Driver");
}

void static inline rb_extract_bind_params(int argc, VALUE* argv, std::vector<dbi::Param> &bind) {
    for (int i = 0; i < argc; i++) bind.push_back(dbi::PARAM(CSTRING(argv[i])));
}

static VALUE rb_handle_new(VALUE klass, VALUE opts) {
    dbi::Handle *h;
    VALUE obj;

    VALUE db       = rb_hash_aref(opts, ID2SYM(rb_intern("db")));
    VALUE host     = rb_hash_aref(opts, ID2SYM(rb_intern("host")));
    VALUE port     = rb_hash_aref(opts, ID2SYM(rb_intern("port")));
    VALUE user     = rb_hash_aref(opts, ID2SYM(rb_intern("user")));
    VALUE driver   = rb_hash_aref(opts, ID2SYM(rb_intern("driver")));
    VALUE password = rb_hash_aref(opts, ID2SYM(rb_intern("password")));

    if (db     == Qnil) rb_raise(eArgumentError, "Handle#new called without :db");
    if (user   == Qnil) rb_raise(eArgumentError, "Handle#new called without :user");
    if (driver == Qnil) rb_raise(eArgumentError, "Handle#new called without :driver");

    host     = host == Qnil ? rb_str_new2("") : host;
    port     = port == Qnil ? rb_str_new2("") : port;
    password = password == Qnil ? rb_str_new2("") : password;

    try {
        h = new dbi::Handle(
            RSTRING_PTR(driver), RSTRING_PTR(user), RSTRING_PTR(password),
            RSTRING_PTR(db), RSTRING_PTR(host), RSTRING_PTR(port)
        );
    } catch EXCEPTION("Connection");

    obj = Data_Wrap_Struct(cHandle, NULL, free_connection, h);
    return obj;
}

static VALUE rb_handle_prepare(VALUE self, VALUE sql) {
    dbi::Handle *h = DBI_HANDLE(self);
    dbi::Statement *st = new dbi::Statement(h, RSTRING_PTR(sql));
    VALUE rv = Data_Wrap_Struct(cStatement, NULL, free_statement, st);
    return rv;
}

VALUE rb_handle_execute(int argc, VALUE *argv, VALUE self) {
    unsigned int rows = 0;
    dbi::Handle *h = DBI_HANDLE(self);
    if (argc == 0) rb_raise(eArgumentError, "Handle#execute called without a SQL command");
    try {
        if (argc == 1) {
            rows = h->execute(RSTRING_PTR(argv[0]));
        }
        else {
            dbi::ResultRow bind;
            rb_extract_bind_params(argc, argv+1, bind);
            dbi::Statement st = h->prepare(RSTRING_PTR(argv[0]));
            rows = st.execute(bind);
        }
    } catch EXCEPTION("Runtime");
    return INT2NUM(rows);
}

VALUE rb_handle_begin(int argc, VALUE *argv, VALUE self) {
    dbi::Handle *h = DBI_HANDLE(self);
    if (argc > 1) rb_raise(eArgumentError, "Got %d parameters. Handle#begin expects 0 or 1", argc);
    try { argc == 0 ? h->begin() : h->begin(CSTRING(argv[0])); } catch EXCEPTION("Runtime");
}


VALUE rb_handle_commit(int argc, VALUE *argv, VALUE self) {
    dbi::Handle *h = DBI_HANDLE(self);
    if (argc > 1) rb_raise(eArgumentError, "Got %d parameters. Handle#commit expect 0 or 1", argc);
    try { argc == 0 ? h->commit() : h->commit(CSTRING(argv[0])); } catch EXCEPTION("Runtime");
}

VALUE rb_handle_rollback(int argc, VALUE *argv, VALUE self) {
    dbi::Handle *h = DBI_HANDLE(self);
    if (argc > 1) rb_raise(eArgumentError, "Got %d parameters. Handle#rollback expects 0 or 1", argc);
    try { argc == 0 ? h->rollback() : h->rollback(CSTRING(argv[0])); } catch EXCEPTION("Runtime");
}

VALUE rb_handle_transaction(VALUE self) {
    int status;
    std::string spname = "SP" + dbi::generateCompactUUID();
    dbi::Handle *h = DBI_HANDLE(self);
    if (rb_block_given_p()) {
        h->begin(spname);
        rb_protect(rb_yield, self, &status);
        if (status == 0 && h->transactions().back() == spname) {
            h->commit(spname);
        }
        else if (status != 0) {
            if (h->transactions().back() == spname) h->rollback(spname);
            rb_jump_tag(status);
        }
    }
    else {
        rb_raise(eRuntimeError, "No block given to Handle#transaction");
    }
}

VALUE rb_statement_new(VALUE klass, VALUE hl, VALUE sql) {
    dbi::Handle *h = DBI_HANDLE(hl);

    if (hl  == Qnil || !h)
        rb_raise(eArgumentError, "Statement#new called without a Handle instance");
    if (sql == Qnil || TYPE(sql) != T_STRING)
        rb_raise(eArgumentError, "Statement#new called without a SQL command");

    dbi::Statement *st = new dbi::Statement(h, RSTRING_PTR(sql));
    VALUE rv = Data_Wrap_Struct(cStatement, NULL, free_statement, st);
    return rv;
}

static VALUE rb_statement_each(VALUE self) {
    unsigned int r, c;
    const char *vptr;
    dbi::Statement *st = DBI_STATEMENT(self);
    try {
        std::vector<string> fields = st->fields();
        for (r = 0; r < st->rows(); r++) {
            VALUE row = rb_hash_new();
            for (c = 0; c < fields.size(); c++) {
                vptr = (const char*)st->fetchValue(r,c);
                rb_hash_aset(row, rb_str_new2(fields[c].c_str()), vptr ? rb_str_new2(vptr) : Qnil);
            }
            rb_yield(row);
        }
    } catch EXCEPTION("Runtime");
}

VALUE rb_statement_execute(int argc, VALUE *argv, VALUE self) {
    dbi::Statement *st = DBI_STATEMENT(self);
    try {
        if (argc == 0) {
            st->execute();
        }
        else {
            dbi::ResultRow bind;
            rb_extract_bind_params(argc, argv, bind);
            st->execute(bind);
        }
    } catch EXCEPTION("Runtime");

    if (rb_block_given_p()) return rb_statement_each(self);
    return self;
}

VALUE rb_statement_rows(VALUE self) {
    unsigned int rows;
    dbi::Statement *st = DBI_STATEMENT(self);
    try { rows = st->rows(); } catch EXCEPTION("Runtime");
    return INT2NUM(rows);
}

VALUE rb_statement_fetchrow(VALUE self) {
    const char *vptr;
    unsigned int r, c;
    VALUE row = Qnil;
    dbi::Statement *st = DBI_STATEMENT(self);
    try {
        r = st->currentRow();
        if (r < st->rows()) {
            row = rb_ary_new();
            for (c = 0; c < st->columns(); c++) {
                vptr = (const char*)st->fetchValue(r, c);
                rb_ary_push(row, vptr ? rb_str_new2(vptr) : Qnil);
            }
            st->advanceRow();
        }
    } catch EXCEPTION("Runtime");

    return row;
}

VALUE rb_dbi_trace(int argc, VALUE *argv, VALUE self) {
    // by default log all messages to stderr.
    int fd = 2;
    rb_io_t *fptr;

    if (argc == 0) rb_raise(eArgumentError, "DBI#trace expects a boolean value, got none.");

    bool flag = argv[0] == Qtrue ? true : false;

    if (argc > 1) {
        GetOpenFile(rb_convert_type(argv[1], T_FILE, "IO", "to_io"), fptr);
        fd = fptr->fd;
    }

    dbi::trace(flag, fd);
}

extern "C" {
    void Init_dbicpp(void) {

        fStringify     = rb_intern("to_s");
        eRuntimeError  = CONST_GET(rb_mKernel, "RuntimeError");
        eArgumentError = CONST_GET(rb_mKernel, "ArgumentError");
        eStandardError = CONST_GET(rb_mKernel, "StandardError");

        mDBI           = rb_define_module("DBI");
        cHandle        = rb_define_class_under(mDBI, "Handle", rb_cObject);
        cStatement     = rb_define_class_under(mDBI, "Statement", rb_cObject);

        rb_define_module_function(mDBI, "init", RUBY_METHOD_FUNC(rb_dbi_init), 1);
        rb_define_module_function(mDBI, "trace", RUBY_METHOD_FUNC(rb_dbi_trace), -1);

        rb_define_singleton_method(cHandle, "new", RUBY_METHOD_FUNC(rb_handle_new), 1);

        rb_define_method(cHandle, "prepare",     RUBY_METHOD_FUNC(rb_handle_prepare), 1);
        rb_define_method(cHandle, "execute",     RUBY_METHOD_FUNC(rb_handle_execute), -1);
        rb_define_method(cHandle, "begin",       RUBY_METHOD_FUNC(rb_handle_begin), -1);
        rb_define_method(cHandle, "commit",      RUBY_METHOD_FUNC(rb_handle_commit), -1);
        rb_define_method(cHandle, "rollback",    RUBY_METHOD_FUNC(rb_handle_rollback), -1);
        rb_define_method(cHandle, "transaction", RUBY_METHOD_FUNC(rb_handle_transaction), 0);

        rb_define_singleton_method(cStatement, "new", RUBY_METHOD_FUNC(rb_statement_new), 2);

        rb_define_method(cStatement, "execute",  RUBY_METHOD_FUNC(rb_statement_execute), -1);
        rb_define_method(cStatement, "each",     RUBY_METHOD_FUNC(rb_statement_each), 0);
        rb_define_method(cStatement, "rows",     RUBY_METHOD_FUNC(rb_statement_rows), 0);
        rb_define_method(cStatement, "fetchrow", RUBY_METHOD_FUNC(rb_statement_fetchrow), 0);

        rb_include_module(cStatement, CONST_GET(rb_mKernel, "Enumerable"));

    }
}
