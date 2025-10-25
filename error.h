#ifndef ERROR_H
#define ERROR_H

typedef enum{
    S_OK = 0,       //everything is ok
    S_FALSE = 1,    //something went wrong, but is not fatal
    S_PARTIAL = 2,
    E_FAIL = -1,    //fatal error
}HRESULT;

typedef enum{
    S_RUNNING,
    S_END,
    S_TERMINATE
} PROCESS;

int SUCCEEDED(HRESULT hr);
int FAILED(HRESULT hr);

#endif