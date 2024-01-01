#pragma once

#include <string>
#include <filesystem>
#include <map>
#include <map>
#include <exception>
#include <vector>
#include <iterator>
#include <functional>
#include <thread>
#include <format.hpp>

#include "sqlite3.h"

#ifdef __UVA_PQXX_FOUND__
#       undef null
#       include <pqxx/pqxx>
#endif

#include <uva/core.hpp>
#include <console.hpp>
#include <diagnostics.hpp>
#include <uva/string.hpp>
// record(record&& other) : uva::db::basic_active_record(std::forward<std::map<std::string, var>>(other.values))\
// {\
//     other.id = 0;\
// }\

#define UVA_DATABASE_AVAILABLE

#define uva_database_params(...) __VA_ARGS__ 

#define uva_database_declare(record) \
public:\
    record() : uva::db::basic_active_record() { } \
    record(const record& other) : uva::db::basic_active_record(other) { } \
    record(var&& values) : uva::db::basic_active_record(std::move(values)) { } \
    record(const var& values) : uva::db::basic_active_record(values) { } \
    const uva::db::table* get_table() const override { return table(); } \
    uva::db::table* get_table() override { return table(); } \
    static uva::db::table* table(); \
    static record create(std::map<var, var>&& relations) {\
        record r(std::move(relations)); \
        r.save();\
        return r;\
    } \
    static record create(const std::map<var, var>& relations) {\
        record r(relations); \
        r.save();\
        return r;\
    } \
    static size_t column_count() { return table()->m_columns.size(); } \
    static std::map<std::string, std::string>& columns() { return table()->m_columns; } \
    static uva::db::active_record_relation all() { return uva::db::active_record_relation(table()).select("*").from(table()->m_name);  } \
    template<class... Args> static uva::db::active_record_relation where(const std::string where, Args const&... args) { return record::all().where(where, args...); }\
    static uva::db::active_record_relation from(const std::string& from) { return record::all().from(from); } \
    static uva::db::active_record_relation select(const std::string& select) { return record::all().select(select); } \
    static size_t count(const std::string& count = "*") { return record::all().count(); } \
    template<class... Args> static uva::db::active_record_relation order_by(const std::string order, Args const&... args) { return record::all().order_by(order, args...); }\
    static uva::db::active_record_relation limit(const std::string& limit) { return record::all().limit(limit); } \
    static uva::db::active_record_relation limit(const size_t& limit) { return record::all().limit(limit); } \
    static void each_with_index(std::function<void(std::map<std::string, var>&, const size_t&)> func) { return record::all().each_with_index(func); }\
    static void each(std::function<void(std::map<std::string, var>&)> func) { return record::all().each(func); }\
    static void each_with_index(std::function<void(record&, const size_t&)> func) { return record::all().each_with_index<record>(func); }\
    static void each(std::function<void(record&)> func) { return record::all().each<record>(func); }\
    template<class... Args> static record find_by(std::string where, Args const&... args) { return record(record::all().find_by(where, args...)); }\
    static record find_by(std::map<var, var>&& v) { return record(record::all().where(std::move(v))); }\
    static record find_or_create_by(std::map<var, var>&& v) {\
        auto result = record::find_by(std::move(std::map<var, var>(v)));\
\
        if(!result.present()) {\
            record r(std::move(v));\
            r.save();\
\
            r = record::find_by("id={}", r["id"]);\
            return r;\
        }\
        return result;\
    }\
    static record first() { return all().first(); }\
    record& operator=(const record& other)\
    {\
        values = other.values;\
        return *this;\
    }\
    virtual const std::string& class_name() const override\
    {\
        static const std::string class_name = #record;\
        return class_name;\
    };\

#define uva_db_expose_column(column_name)\
    uva::db::basic_active_record_column column_name = { #column_name, (basic_active_record*)this };

#define uva_db_define_full(record, __table_name) \
uva::db::table* record::table() { \
\
    static std::string table_name = __table_name; \
\
    static uva::db::table* table = uva::db::table::get_table(table_name);\
    \
    return table; \
}\

#define uva_db_define(record) uva_db_define_full(record, uva::string::to_snake_case(uva::string::pluralize(#record)))

#define uva_db_define_sqlite3(db) uva::db::sqlite3_connection* connection = new uva::db::sqlite3_connection(db);

#define uva_db_declare_migration(migration) public: migration()

#define uva_db_define_migration(m) m::m() : uva::db::basic_migration(#m, __FILE__) {  } m* _##m = new m();

#define uva_db_run_migrations() uva::db::basic_migration::do_pending_migrations();

namespace uva
{
    namespace db
    {        
        // Default value of query_buffer_lenght. Don't change this. You can change query_buffer_lenght.
        static constexpr size_t query_buffer_lenght_default = 1024;
        // Increase this if you are going to do big inserts/updates. Helps a lot with performance.
        // You can change it to a big value and then go back to default. The buffer is reset in the next run.
        extern size_t query_buffer_lenght;
        // Default value of query_buffer_lenght. Don't change this. You can change query_buffer_lenght.
        static constexpr bool enable_query_cout_printing_default = true;
        extern bool enable_query_cout_printing;
        class table;
        class basic_active_record;
        class basic_active_record_column;

        void within_transaction(std::function<void()> __f);

        class basic_connection
        {
        private:
            static basic_connection* s_connection;
        public:
            basic_connection();
        public:
            virtual bool open() { return false; };
            virtual bool is_open() const { return false; };

            virtual void run_sql(const std::string& sql) { };
            virtual var select_all(const std::string& sql) { return null; };

            static basic_connection* get_connection();
        };

        class sqlite3_connection : public basic_connection
        {
            public:
                sqlite3_connection();
                sqlite3_connection(const std::filesystem::path& database_path);
                ~sqlite3_connection();
            protected:
                sqlite3 *m_database = nullptr;
                std::filesystem::path m_database_path;
            public:
                sqlite3* get_database() const { return m_database; }
                virtual bool open() override;
                virtual bool is_open() const override;
                bool open(const std::filesystem::path& path);

                virtual void run_sql(const std::string& sql) override;
                virtual var select_all(const std::string& sql) override;
        };

        class postgres_connection : public basic_connection
        {
            public:
                postgres_connection() = default;
                postgres_connection(var params = var::map());
                ~postgres_connection();
                #ifdef __UVA_PQXX_FOUND__
                    protected:
                        std::shared_ptr<pqxx::connection> m_pqxx_connection = nullptr;
                    public:
                        std::shared_ptr<pqxx::connection> get_pqxx_connection() const { return m_pqxx_connection; }
                #endif
            public:
                virtual bool open() override { return false; }
                virtual bool is_open() const override { return false; }
                bool open(const std::filesystem::path& path) { return false; }

                virtual void run_sql(const std::string& sql) override;
                virtual var select_all(const std::string& sql) override;
        };
 
        using result = std::vector<std::pair<std::string, std::string>>;
        using results = std::vector<std::vector<std::pair<std::string, std::string>>>;

        extern std::map<std::string, var::var_type> sql_values_types_map;
        var::var_type sql_delctype_to_value_type(const std::string& type);

        class active_record_relation
        {
        public:
            active_record_relation() = default;
            active_record_relation(table* table);
        private:
            std::map<std::string, var> m_update;
            std::string m_select;
            std::string m_from;
            std::string m_where;
            std::string m_group;
            std::string m_order;
            std::string m_limit;
            var m_insert = var::array();
            std::vector<std::string> m_columns;
            std::string m_into;
            std::string m_returning;
            table* m_table;

            std::map<std::string, size_t> m_columnsIndexes;
            std::vector<std::string> m_columnsNames;
            std::vector<var::var_type> m_columnsTypes;


            bool m_unscoped = false;
        public:
            std::vector<std::vector<var>> m_results;
            std::string to_sql() const;
        public:
            active_record_relation& select(const std::string& select);
            active_record_relation& from(const std::string& from);
            template<class... Args>
            active_record_relation& where(const std::string format, Args... args)
            {
                std::vector<var> parameters =  {{ args ... }};
                std::string __where;
                __where.reserve(format.size() + (parameters.size() * 100));

                size_t current_parameter = 0;

                for(size_t i = 0; i < format.size(); ++i)
                {
                    if(format[i] == '{' && i + 1 < format.size() && format[i+1] == '}') {
                        ++i;
                        __where.append(parameters[current_parameter].to_typed_s('[', ']', false));
                        ++current_parameter;
                    } else {
                        __where.push_back(format[i]);
                    }
                }
    
                append_where(__where);
                return *this;
            }
            std::map<std::string, var> where(std::map<var, var>&& v);
            active_record_relation& group_by(const std::string& group);
            template<class... Args>
            active_record_relation& order_by(const std::string order, Args... args)
            {
                std::string __order = vformat(order, std::make_format_args(args...));
                order_by(__order);
                return *this;
            }
            active_record_relation& order_by(const std::string& order);
            active_record_relation& limit(const std::string& limit);
            active_record_relation& limit(const size_t& limit);
            active_record_relation& insert(var& insert);
            active_record_relation& insert(std::vector<var>& insert);
            active_record_relation& insert(std::vector<std::vector<var>>& insert);
            active_record_relation& columns(const std::vector<std::string>& cols);
            active_record_relation& into(const std::string& into);
            active_record_relation& returning(const std::string& returning);
            std::vector<var> run_sql(const std::string& col);
            std::vector<var> pluck(const std::string& col);
            std::vector<std::vector<var>> pluckm(const std::string& cols);
            size_t count(const std::string& count = "*") const;
            var first();
            void each_with_index(std::function<void(std::map<std::string, var>&, const size_t&)> func);
            void each(std::function<void(std::map<std::string, var>&)> func);
            template<class record>
            void each_with_index(std::function<void(record& value, const size_t&)>& func)
            {
                each_with_index([&](std::map<std::string, var>& value, const size_t& index){
                    record r(std::move(value));
                    func(r, index);
                });
            }
            template<class record>            
            void each(std::function<void(record&)>& func)
            {
                each([&](std::map<std::string, var>& value){
                    record r(std::move(value));
                    func(r);
                });
            }
            template<class... Args>
            var find_by(std::string where, Args... args)
            {
                return active_record_relation(*this).where(where, std::forward<Args>(args)...).first();
            }
            std::map<std::string, var> find_or_create_by(std::map<var, var>&& v);

            var create(var values);
            var update(var values);
        public:
            bool empty();
            std::map<std::string, var> operator[](const size_t& index);
            active_record_relation unscoped();
        public:
            void append_where(const std::string& where);
            std::string commit_sql() const;
            void commit();
            void commit_without_prepare();
            void commit(const std::string& sql);
            void commit_without_prepare(const std::string& sql);
            operator var();
        };

        class table
        {
        public:
            table(const std::string& name, const std::map<std::string, std::string>& cols);
            table(const std::string& name);

            std::string m_name;
            std::map<std::string, std::string> m_columns;
            std::map<size_t, std::map<std::string, std::string>> m_relations;
            std::string primary_key;
            static std::map<std::string, table*>& get_tables();
            static table* get_table(const std::string& name);
            static void add_table(uva::db::table* table);

            /* SQL */
            static void create_table(uva::db::table* table, bool if_not_exists = false);
        };
        class basic_active_record_column
        {
        public:
            std::string key;
            basic_active_record* active_record;
        public:
            basic_active_record_column(const std::string& __key, basic_active_record* __record);
            ~basic_active_record_column();
        public:
            const var* operator->() const;
            var* operator->();

            var& operator*() const;
            var& operator*();
        
            operator var() const;
            
        };
        class basic_active_record
        {              
        public:
            basic_active_record();
            basic_active_record(const basic_active_record& record);
            basic_active_record(basic_active_record&& record);
            basic_active_record(var&& values);
            basic_active_record(const var& values);
        public:
            bool present() const;
            void destroy();            
        private:
            virtual void call_before_create();
            virtual void call_before_save();
            virtual void call_before_update();
        protected:
            virtual const table* get_table() const = 0;
            virtual table* get_table() = 0;
            virtual void before_create() { };
            virtual void before_save() { };
            virtual void before_update() { };
            virtual const std::string& class_name() const = 0;
            std::string to_s() const;
            std::map<std::string, var> values;
            std::map<std::string, basic_active_record_column*> columns;
            //Need to come AFTER values and columns declaration
        public:
            uva_db_expose_column(id);
        public:
            basic_active_record& operator=(const basic_active_record& other);
        public:
            var& at(const std::string& str);
            const var& at(const std::string& str) const;

            void save();
            void update(const std::string& col, const var& value);
            void update(const std::map<std::string, var>& values);
        public:
            var& operator[](const char* str);
            const var& operator[](const char* str) const;
            var& operator[](const std::string& str);
            const var& operator[](const std::string& str) const;

            operator var() const;
        };
        class basic_migration : public basic_active_record
        {
        uva_database_declare(basic_migration);
        public:
            basic_migration(const std::string& __title, std::string_view __filename);
        private:
            std::string make_label();
            std::string title;
            std::string date_str;
        public:
            virtual bool is_pending();
            virtual void change() {};
            void apply();
        public:
            static std::vector<basic_migration*>& get_migrations();
            static void do_pending_migrations();
        protected:
            void call_change();
        public:
            void add_table(const std::string& table_name, const std::map<std::string, std::string>& cols);
            void add_table_if_not_exists(const std::string& table_name, const std::map<std::string, std::string>& cols);

            void drop_table(const std::string& table_name);

            void drop_column(const std::string& table_name, const std::string& name);
            void add_column(const std::string& table_name, const std::string& name, const std::string& type, const std::string& default_value) const;
            void add_index(const std::string& table_name, const std::string& column);
            void change_column(const std::string& table_name, const std::string& name, const std::string& type) const;
        };
    };
};

template <>
struct std::formatter<uva::db::basic_active_record_column> : std::formatter<std::string> {
    auto format(const uva::db::basic_active_record_column& v, format_context& ctx) {
        return std::format_to(ctx.out(), "{}", v->to_s());
    }
};