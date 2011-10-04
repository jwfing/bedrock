#ifndef _HYPERBIRDER_COMMON_REGEX_H_
#define _HYPERBIRDER_COMMON_REGEX_H_

#include <regex.h>
#include "comm_def.h"

DECLARE_HB_NAMESPACE(common)

class regex
{
public:
    regex();
    ~regex();

    bool init(const char* pattern, int flags);

    bool match(const char* text, int flags);

    void destroy(void);

private:
    bool _init;
    regmatch_t* _match;
    regex_t _reg;
    size_t _nmatch;
};

END_DECLARE_HB_NAMESPACE(common)

#endif
