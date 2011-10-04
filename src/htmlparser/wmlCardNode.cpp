#include "wmlCardNode.h"

std::string wmlCardNode::toString()
{
    _name = HTML_ELEM_DIV;
    removeAttr(WML_ATTR_TITLE);
    return xmlNode::toString();
}
