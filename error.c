#include "error.h"
#include "debugmalloc.h"

int P_SUCCEEDED(P_HRESULT hr){
    return (hr >= 0) ? 1 : 0;
}

int P_FAILED(P_HRESULT hr){
    return (hr < 0) ? 1 : 0;
}