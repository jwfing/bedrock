#ifndef _XML_CONVERTOR_WMLANCHORNODE_INCLUDE_H_
#define _XML_CONVERTOR_WMLANCHORNODE_INCLUDE_H_

#include "xmlnode.h"

class wmlAnchorNode : public xmlNode
{
public:
    wmlAnchorNode(int32_t level, const std::string& name)
        : xmlNode(level, name)
    {
        ;
    }
    virtual std::string toString();
};

#endif
