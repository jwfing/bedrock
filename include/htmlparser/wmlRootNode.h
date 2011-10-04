#ifndef _XML_CONVERTOR_WMLROOTNODE_INCLUDE_H_
#define _XML_CONVERTOR_WMLROOTNODE_INCLUDE_H_

#include "xmlnode.h"

class wmlRootNode : public xmlNode
{
public:
    wmlRootNode(int32_t level, const std::string& name)
        : xmlNode(level, name)
    {
        ;
    }
    virtual std::string toString();
};

#endif
