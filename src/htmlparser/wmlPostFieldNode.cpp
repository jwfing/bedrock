#include "wmlPostFieldNode.h"

std::string wmlPostFieldNode::toString()
{
    _name = HTML_ELEM_INPUT;
    setAttr(HTML_ATTR_TYPE, HTML_ATTRV_HIDDEN);
    return xmlNode::toString();
}
