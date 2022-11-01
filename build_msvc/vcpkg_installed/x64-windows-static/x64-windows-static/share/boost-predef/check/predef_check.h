#include <boost/predef/detail/test_def.h>
const char * str_token(const char ** str, const char * space)
{
    unsigned span;
    char * token;
    for (; **str != 0; *str += 1)
    {
        if (0 == strchr(space, **str))
        {
            break;
        }
    }
    span = strcspn(*str, space);
    token = (char *)malloc(span+1);
    strncpy(token, *str, span);
    token[span] = 0;
    for (*str += span; **str != 0; *str += 1)
    {
        if (0 == strchr(space, **str))
        {
            break;
        }
    }
    return token;
}
const char * whitespace = " ";
const char * dot = ".";
int main(int argc, const char ** argv)
{
    unsigned x = 0;
    int argi = 1;
    create_predef_entries();
#if 0
    qsort(generated_predef_info,generated_predef_info_count,
        sizeof(predef_info),predef_info_compare);
    for (x = 0; x < generated_predef_info_count; ++x)
    {
        printf("%s: %d\n", generated_predef_info[x].name, generated_predef_info[x].value);
    }
#endif
    int result = -1;
    for (argi = 1; argi < argc; ++argi)
    {
        const char * exp = argv[argi];
        const char * exp_name = str_token(&exp, whitespace);
        const char * exp_op = str_token(&exp, whitespace);
        const char * exp_val = str_token(&exp, whitespace);
        unsigned exp_version = 0;
        if (*exp_val != 0)
        {
            exp = exp_val;
            const char * exp_val_a = str_token(&exp, dot);
            const char * exp_val_b = str_token(&exp, dot);
            const char * exp_val_c = str_token(&exp, dot);
            exp_version = BOOST_VERSION_NUMBER(atoi(exp_val_a), atoi(exp_val_b),atoi(exp_val_c));
        }
        for (x = 0; x < generated_predef_info_count; ++x)
        {
            if (*exp_op == 0 &&
                generated_predef_info[x].value > 0 &&
                strcmp(exp_name, generated_predef_info[x].name) == 0)
            {
                result = 0;
                break;
            }
            else if (*exp_op == 0 &&
                generated_predef_info[x].value == 0 &&
                strcmp(exp_name, generated_predef_info[x].name) == 0)
            {
                return argi;
            }
            else if (*exp_op != 0 && *exp_val != 0 &&
                strcmp(exp_name, generated_predef_info[x].name) == 0)
            {
                result = 0;
                if (0 == strcmp(">",exp_op) && !(generated_predef_info[x].value > exp_version)) return argi;
                if (0 == strcmp("<",exp_op) && !(generated_predef_info[x].value < exp_version)) return argi;
                if (0 == strcmp(">=",exp_op) && !(generated_predef_info[x].value >= exp_version)) return argi;
                if (0 == strcmp("<=",exp_op) && !(generated_predef_info[x].value <= exp_version)) return argi;
                if (0 == strcmp("==",exp_op) && !(generated_predef_info[x].value == exp_version)) return argi;
                if (0 == strcmp("!=",exp_op) && !(generated_predef_info[x].value != exp_version)) return argi;
            }
        }
    }
    return result;
}
