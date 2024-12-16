#pragma once

#ifndef SAFE_COM_RELEASE
#define SAFE_COM_RELEASE(pInterface) \
    do {                            \
        if (pInterface != NULL) {   \
            pInterface->Release();  \
            pInterface = NULL;      \
        }                           \
    } while (0)
#endif // SAFE_COM_RELEASE

#ifndef SAFE_OBJECT_DELETE
#define SAFE_OBJECT_DELETE(pObject) \
    do {                            \
        if (pObject != NULL) {      \
            delete pObject;         \
            pObject = NULL;         \
        }                           \
    } while (0)
#endif // SAFE_OBJECT_DELETE

#ifndef UNUSED_VAR
#define UNUSED_VAR(var)             \
    do {                            \
        ((void)(var));              \
    } while (0)
#endif

#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(var)       \
    {                               \
        var = var;                  \
    }
#endif
