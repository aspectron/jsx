{
    'targets': [
        {
            'target_name': 'expat',
            'type': 'static_library',
            'msvs_guid': 'B9335095-4269-4C51-9C18-D58DD5B21A9E',
            'defines': ['XML_STATIC'],
            'conditions': [
                [ 'OS=="win"', { 'defines': ['COMPILED_FROM_DSP'] }, { 'defines': ['HAVE_EXPAT_CONFIG_H'] } ],
            ],
            'include_dirs': ['.', 'lib'],
            'direct_dependent_settings': {
                'defines': ['XML_STATIC'],
                'include_dirs': ['lib'],
            },
            'sources': [
                'lib/expat.h',
                'lib/xmlparse.c',
                'lib/xmlrole.c',
                'lib/xmltok.c',
            ],
        },
    ],
}
