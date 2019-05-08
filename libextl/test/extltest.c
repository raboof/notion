#include "../luaextl.c"
#include "../readconfig.h"
#include "libtu/errorlog.h"

static const char tostringstr[]=
    "local arg = {...}\n"
    "print('first arg is a ' .. type(arg[1]))\n"
    "local basis = arg[1]()\n"
    "local result\n"
    "print('first arg returned a ' .. type(basis))\n"
    "if type(basis)=='string' then\n"
    "    result = string.format('%q', basis)\n"
    "else\n"
    "    result = tostring(basis)\n"
    "end\n"
    "print('result is a ' .. type(result))\n"
    "print('result is ' .. result)\n"
    "return result\n";

int test_tostring()
{
    ExtlFn tostringfn;
    ExtlFn fn;
    ErrorLog el;
    char *retstr=NULL;

    errorlog_begin(&el);

    if(!extl_loadstring(tostringstr, &tostringfn))
        return 1;

    if(!extl_loadstring("print('testing\\n'); return 'test'", &fn))
        return 2;
    
    fprintf(stderr, "Calling extl_call...\n");
    if(!extl_call(tostringfn, "f", "s", fn, &retstr))
        return 3;

    if (retstr == NULL)
        return 4;

    if(strcmp("\"test\"", retstr) != 0){
        fprintf(stderr, "Result was '%s' instead of 'test'", retstr);
        return 5;
    }

    return 0;
}

int test_stack_get_bool()
{
    ErrorLog el;
    ExtlAny bool_any;
    bool bool_test;
    bool bool_push;

    errorlog_begin(&el);

    // true
    bool_push=TRUE;
    extl_stack_push(l_st, 'b', &bool_push);
    bool_test=FALSE;
    if(extl_stack_get(l_st, -1, 'b', FALSE, NULL, &bool_test) != TRUE)
        return 1;
    if (bool_test != TRUE)
        return 2;

    if(extl_stack_get(l_st, -1, 'a', FALSE, NULL, &bool_any) != TRUE)
        return 3;
    if (bool_any.type != 'b')
        return 4;
    if (bool_any.value.b != TRUE)
        return 5;

    // false
    bool_push=FALSE;
    extl_stack_push(l_st, 'b', &bool_push);
    bool_test=TRUE;
    if(extl_stack_get(l_st, -1, 'b', FALSE, NULL, &bool_test) != TRUE)
        return 6;
    if (bool_test != FALSE)
        return 7;

    if(extl_stack_get(l_st, -1, 'a', FALSE, NULL, &bool_any) != TRUE)
        return 8;
    if (bool_any.type != 'b')
        return 9;
    if (bool_any.value.b != FALSE)
        return 10;

    return 0;
}

int main()
{
    fprintf(stdout, "[TESTING] libextl ====\n");

    int result = 0;
    int err = 0;

    extl_init();

    fprintf(stdout, "[TEST] test_tostring: ");
    result = test_tostring();
    if (result != 0) {
        fprintf(stdout, "[ERROR]: %d\n", result);
        err += 1;
    } else {
        fprintf(stdout, "[OK]\n");
    }

    fprintf(stdout, "[TEST] test_stack_get_bool: ");
    result = test_stack_get_bool();
    if (result != 0) {
        fprintf(stdout, "[ERROR]: %d\n", result);
        err += 1;
    } else {
        fprintf(stdout, "[OK]\n");
    }

    extl_deinit();

    return err;
}
