#ifndef CPPDB_COL_TYPE_H
#define CPPDB_COL_TYPE_H

///
/// The namespace of all data related to the cppdb api
///

namespace cppdb {

///
/// Column Data type.
///
enum column_type
{
	null_type,
	int_type,
	real_type,
	string_type,
	blob_type,
};

} // cppdb

#endif
