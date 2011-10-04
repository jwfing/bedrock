#ifndef _XML_CONVERTOR_WMLDONODE_INCLUDE_H_
#define _XML_CONVERTOR_WMLDONODE_INCLUDE_H_

#include "xmlnode.h"

class wmlDoNode : public xmlNode
{
public:
    wmlDoNode(int32_t level, const std::string& name)
        : xmlNode(level, name)
    {
        ;
    }
    virtual std::string toString();
};

#endif
