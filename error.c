#include "error.h"

int SUCCEEDED(HRESULT hr){
    return (hr >= 0) ? 1 : 0;
}

int FAILED(HRESULT hr){
    return (hr < 0) ? 1 : 0;
}