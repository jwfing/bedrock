#include "wmlRootNode.h"

std::string wmlRootNode::toString()
{
    _name = HTML_ELEM_ROOT;
    xmlNode* head = NULL;
    xmlNode* body = NULL;
    std::vector<xmlNode*> bodyNodes;
    std::string title = "";

    std::vector<xmlNode*>::iterator nodeIter = _children.begin();
    for (; nodeIter != _children.end(); ++nodeIter) {
        if (!(*nodeIter)) {
            continue;
        }
        if ((*nodeIter)->getName() == HTML_ELEM_HEAD) {
            head = *nodeIter;
        } else {
            bodyNodes.push_back(*nodeIter);
            if (title.length() == 0) {
                title = (*nodeIter)->getAttr(HTML_ELEM_TITLE);
            }
        }
    }
    if (NULL == head) {
        head = new xmlNode(_level + 1, HTML_ELEM_HEAD);
    }
    if (title.length() > 0) {
        xmlNode* titleNode = new xmlNode(head->getLevel() + 1, HTML_ELEM_TITLE);
        xmlNode* sub = new xmlNode(head->getLevel() + 2, "", NT_TEXT);
        sub->setContents(title);
        sub->setParent(titleNode);
        titleNode->addChild(sub);
        titleNode->setParent(head);
        head->addChild(titleNode);
    }
    body = new xmlNode(_level + 1, HTML_ELEM_BODY);
    nodeIter = bodyNodes.begin();
    for (; bodyNodes.end() !=  nodeIter; ++nodeIter) {
        if (*nodeIter) {
            (*nodeIter)->setParent(body);
            (*nodeIter)->adjustLevel(body->getLevel() + 1);
            body->addChild(*nodeIter);
        }
    }
    _children.clear();
    _children.push_back(head);
    _children.push_back(body);

    return xmlNode::toString();
}
