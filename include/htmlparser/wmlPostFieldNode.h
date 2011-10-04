#ifndef _XML_CONVERTOR_WMLPOSTFIELDNODE_INCLUDE_H_
#define _XML_CONVERTOR_WMLPOSTFIELDNODE_INCLUDE_H_

#include "xmlnode.h"

class wmlPostFieldNode : public xmlNode
{
public:
    wmlPostFieldNode(int32_t level, const std::string& name)
        : xmlNode(level, name)
    {
        ;
    }
    virtual std::string toString();
};

#endif
