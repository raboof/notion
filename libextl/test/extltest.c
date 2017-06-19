#include "../luaextl.c"
#include "../readconfig.c"

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


int main()
{
    int result = 0;

    extl_init();

    result += test_tostring();

    fprintf(stderr, "Result: %d\n", result);

    return result;
}
