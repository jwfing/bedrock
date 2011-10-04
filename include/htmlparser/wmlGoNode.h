#ifndef _XML_CONVERTOR_WMLGONODE_INCLUDE_H_
#define _XML_CONVERTOR_WMLGONODE_INCLUDE_H_

#include "xmlnode.h"

class wmlGoNode : public xmlNode
{
public:
    wmlGoNode(int32_t level, const std::string& name)
        : xmlNode(level, name)
    {
        ;
    }
    virtual std::string toString();
};

#endif
