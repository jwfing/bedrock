#include "wmlDoNode.h"

std::string wmlDoNode::toString()
{
    std::string accept = getAttr(WML_ATTR_ACCEPT);
    if (accept.length() <= 0) {
        return "";
    }
    _name = HTML_ELEM_INPUT;
    setAttr(HTML_ATTR_TYPE, HTML_ATTRV_SUBMIT);
    return xmlNode::toString();
}
