#include "regex.h"
#include <stdlib.h>

DECLARE_HB_NAMESPACE(common)

regex::regex()
{
    _init = false;
    _match = NULL;
    _nmatch = 0;
}

regex::~regex()
{
    if (true == _init)
    {
        destroy();
    }
}

bool regex::init(const char* pattern, int flags)
{
    bool retval = true;
    int ret = 0;
    if (false == _init)
    {
        ret = regcomp(&_reg, pattern, flags);
        if (0 != ret)
        {
            DF_WRITE_LOG(UL_LOG_WARNING, "failed to call regcomp --- ret : %d", ret);
            retval = false;
        }
        else
        {
            _nmatch = _reg.re_nsub + 1;
            _match = (regmatch_t*)malloc(sizeof(regmatch_t) * _nmatch);
            if (NULL == _match)
            {
                DF_WRITE_LOG(UL_LOG_FATAL, "no memory to create the regmatch object");
                regfree(&_reg);
                retval = false;
            }
            else
            {
                _init = true;
                retval = true;
            }
        }
    }
    else
    {
        DF_WRITE_LOG(UL_LOG_WARNING, "re-initialized.");
    }
    return retval;
}

bool regex::match(const char* text, int flags)
{
    bool retval = false;
    int ret = 0;
    if (_init)
    {
        ret = regexec(&_reg, text, _nmatch, _match, flags);
        if (REG_NOMATCH == ret)
        {
            retval = false;
        }
        else if (ret != 0)
        {
            retval = false;
        }
        else
        {
            retval = true;
        }
    }
    else
    {
        DF_WRITE_LOG(UL_LOG_WARNING, "un-initialized.");
    }
    return retval;
}

void regex::destroy(void)
{
    if (_init)
    {
        regfree(&_reg);
        if (NULL != _match)
        {
            free(_match);
            _match = NULL;
        }
        _nmatch = 0;
        _init = false;
    }
}

END_DECLARE_HB_NAMESPACE(common)
