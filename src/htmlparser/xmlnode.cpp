#include "xmlnode.h"

int32_t memAllocCounter = 0;

xmlNode::xmlNode(int32_t level, const std::string& name, const NodeType& type)
    : _name(name), _contents(""), _cdata(""), _parent(NULL), _iterator(-1), _level(level), _type(type)
{
    memAllocCounter++;
}

xmlNode::~xmlNode()
{
    _attrs.clear();
    _parent = NULL;
    memAllocCounter--;
    std::vector<xmlNode*>::iterator it = _children.begin();
    for (; it != _children.end(); ++it)
    {
        if (*it) {
            delete *it;
            *it = NULL;
        }
    }
    _children.clear();
}

std::string xmlNode::toString() {
    std::stringstream buf;
    for (int32_t i = 0; i < _level; i++) {
         buf << "  ";
    }
    std::string prefix = buf.str();
    if (_type == NT_NORMAL) {
        buf << "<" << _name;
        std::stringstream attrs;
        std::map<std::string, std::string>::iterator it = _attrs.begin();
        for (; it != _attrs.end(); ++it) {
            buf << " " << it->first << "=\"" << it->second << "\"";
        }
        if (_children.size() <= 0 && _contents.length() <= 0) {
            buf << "/>" <<endl;
        } else {
            buf << ">" << endl;
//            if (_contents.length() > 0) {
//                buf << prefix << "  " << _contents << endl;
//            }
            std::vector<xmlNode*>::iterator nodeIter = _children.begin();
            for (; nodeIter != _children.end(); ++nodeIter) {
                if (*nodeIter) {
                    buf << (*nodeIter)->toString();
                }
            }
            buf << prefix << "</" << _name << ">" << endl;
        }
    } else {
        buf << _contents << endl;
    }
    return buf.str();
}
