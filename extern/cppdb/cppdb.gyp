{
    'variables': {
        'version_major': '0',
        'version_minor': '3',
        'version_patch': '1',
        'use_sqlite%': 1,
        'use_odbc%': 0,
        'use_mysql%': 0,
        'use_postgres%': 0,
    },
    'targets': [
        {
            'target_name': 'cppdb',
            'type': 'static_library',
            'msvs_guid': '1CB5B49F-DE25-4A6B-9666-13CB404331F6',
            'defines': [
                'CPPDB_MAJOR=<(version_major)',
                'CPPDB_MINOR=<(version_minor)',
                'CPPDB_PATCH=<(version_patch)',
                'CPPDB_VERSION="<(version_major).<(version_minor).<(version_patch)"',
                'CPPDB_LIBRARY_PREFIX="<(STATIC_LIB_PREFIX)"',
                'CPPDB_LIBRARY_SUFFIX="<(STATIC_LIB_SUFFIX)"',
                'CPPDB_SOVERSION="<(version_major)"',
                'NOMINMAX',
            ],
            'include_dirs': ['.'],
            'direct_dependent_settings': {
                'include_dirs': ['.'],
            },
            'sources': [
                # include files
                'cppdb/atomic_counter.h',
                'cppdb/backend.h',
                'cppdb/col_type.h',
                'cppdb/connection_specific.h',
                'cppdb/conn_manager.h',
                'cppdb/defs.h',
                'cppdb/driver_manager.h',
                'cppdb/errors.h',
                'cppdb/frontend.h',
                'cppdb/mutex.h',
                'cppdb/numeric_util.h',
                'cppdb/pool.h',
                'cppdb/ref_ptr.h',
                'cppdb/shared_object.h',
                'cppdb/utils.h',

                # sources
                'src/atomic_counter.cpp',
                'src/backend.cpp',
                'src/conn_manager.cpp',
                'src/driver_manager.cpp',
                'src/frontend.cpp',
                'src/mutex.cpp',
                'src/pool.cpp',
                'src/shared_object.cpp',
                'src/utils.cpp',
            ],
            'conditions': [
                ['use_sqlite', {
                    'dependencies': [ '../sqlite/sqlite.gyp:*'],
                    'defines': ['CPPDB_WITH_SQLITE3'],
                    'sources': ['drivers/sqlite3_backend.cpp'],
                }],
                ['use_odbc', {
                    'defines': ['CPPDB_WITH_ODBC'],
                    'sources': ['drivers/odbc_backend.cpp'],
                }],
                ['use_mysql', {
                    'defines': ['CPPDB_WITH_MYSQL'],
                    'sources': ['drivers/mysql_backend.cpp'],
               }],
                ['use_postgres', {
                    'defines': ['CPPDB_WITH_PQ'],
                    'sources': ['drivers/postgres_backend.cpp'],
                }],
            ],
        },
    ],
}