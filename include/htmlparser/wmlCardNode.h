#ifndef _XML_CONVERTOR_WMLCardNODE_INCLUDE_H_
#define _XML_CONVERTOR_WMLCardNODE_INCLUDE_H_

#include "xmlnode.h"

class wmlCardNode : public xmlNode
{
public:
    wmlCardNode(int32_t level, const std::string& name)
        : xmlNode(level, name)
    {
        ;
    }
    virtual std::string toString();
};

#endif
