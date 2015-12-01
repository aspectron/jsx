{
    'targets': [
        {
            'target_name': 'sqlite',
            'type': 'static_library',
            'msvs_guid': '3E40728F-DBFB-4956-80F5-56CCD3F1AF1C',
            'direct_dependent_settings': {
                'include_dirs': ['.'],
            },
            'sources': [
                'sqlite3.h',
                'sqlite3.c',
            ],
        },
    ],
}