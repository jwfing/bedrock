#include "wmlAnchorNode.h"

std::string wmlAnchorNode::toString()
{
    xmlNode* goNode = findChild(WML_NODE_GO);
    xmlNode* text = findChild("");
    if (NULL != goNode) {
        xmlNode* submit = new xmlNode(goNode->getLevel() + 1, HTML_ELEM_INPUT);
        submit->setAttr(HTML_ATTR_TYPE, HTML_ATTRV_SUBMIT);
        if (NULL != text) {
            submit->setAttr(HTML_ATTR_VALUE, text->getContents());
        } else {
            submit->setAttr(HTML_ATTR_VALUE, _contents);
        }
        submit->setParent(goNode);
        goNode->addChild(submit);
        return goNode->toString();
    }
    return "";
}
