%module containsModule 

%{
#define SWIG_FILE_WITH_INIT
#include "strtree_contains.hpp"
using namespace xf::data_analytics::geospatial;
%}

%include "carrays.i"
%include "std_string.i"
%include "std_vector.i"
%include "numpy.i"

%init %{
    import_array();
%}

%apply (int* ARGOUT_ARRAY1, int DIM1) {(int* rangevec, int n)}

%include "strtree_contains.hpp"
%array_class(int, intArray);
%array_class(double, doubleArray);

namespace std {
    %template(IntVector) vector<int>;
    %template(IntVector2) vector<vector<int> >;
}
