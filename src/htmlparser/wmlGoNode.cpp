#include "wmlGoNode.h"

std::string wmlGoNode::toString()
{
    _name = HTML_ELEM_FORM;
    std::string href = getAttr(WML_ATTR_HREF);
    if (href.length() > 0) {
        removeAttr(WML_ATTR_HREF);
        setAttr(HTML_ATTR_ACTION, href);
    }
    return xmlNode::toString();
}
