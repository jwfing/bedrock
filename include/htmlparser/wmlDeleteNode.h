#ifndef _XML_CONVERTOR_WMLDELETENODE_INCLUDE_H_
#define _XML_CONVERTOR_WMLDELETENODE_INCLUDE_H_

#include "xmlnode.h"

class wmlDeleteNode : public xmlNode
{
public:
    wmlDeleteNode(int32_t level, const std::string& name)
        : xmlNode(level, name)
    {
        ;
    }
    virtual std::string toString();
};

#endif

