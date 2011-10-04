#ifndef _XML_CONVERTOR_XMLNODE_INCLUDE_H_
#define _XML_CONVERTOR_XMLNODE_INCLUDE_H_

#include <string>
#include <sstream>
#include <map>
#include <stdlib.h>
#include <vector>

#include "tagDef.h"

using namespace std;

extern int32_t memAllocCounter;

typedef enum _node_type
{
    NT_NORMAL = 0,
    NT_TEXT
}NodeType;

class xmlNode
{
public:
    xmlNode(int32_t level, const std::string& name, const NodeType& type = NT_NORMAL);
    virtual ~xmlNode();
    std::string getName() {
        return _name;
    }
    NodeType getType() {
        return _type;
    }
    void setType(const NodeType& type) {
        _type = type;
    }
    void setContents(const std::string contents) {
        _contents += contents;
    }
    std::string& getContents() {
        return _contents;
    }
    int setAttr(const std::string& name, const std::string& value) {
        _attrs[name] = value;
        return 0;
    }
    int removeAttr(const std::string& name) {
        std::map<std::string, std::string>::iterator it = _attrs.find(name);
        if (it != _attrs.end()) {
            _attrs.erase(it);
            return 0;
        } else {
            return -1;
        }
    }
    std::string getAttr(const std::string& name) {
        std::map<std::string, std::string>::iterator it = _attrs.find(name);
        if (it != _attrs.end()) {
            return it->second;
        } else {
            return "";
        }
    }

    int setParent(xmlNode* parent) {
        _parent = parent;
        return 0;
    }
    xmlNode* getParent() {
        return _parent;
    }
    int32_t getLevel() {
        return _level;
    }
    void adjustLevel(int32_t newLevel) {
        _level = newLevel;
        std::vector<xmlNode*>::iterator it = _children.begin();
        for (; it != _children.end(); ++it) {
            if (*it) {
                (*it)->adjustLevel(newLevel + 1);
            }
        }
    }

    int addChild(xmlNode* node) {
        if (NULL == node) {
            return -1;
        }
        _children.push_back(node);
        return 0;
    }
    xmlNode* getFirstChild() {
        _iterator = 0;
        return at(_iterator);
    }
    xmlNode* getNextChild() {
        return at(++_iterator);
    }
    xmlNode* findChild(const std::string& name) {
        xmlNode* result = NULL;
        int total = _children.size();
        for (int i = 0; i < total; i++) {
            result = at(i);
            if (result && result->getName() == name) {
                return result;
            }
        }
        return NULL;
    }

    virtual std::string toString();

protected:
    xmlNode* at(int32_t index) {
        if (index >= 0 && index < _children.size()) {
            return _children[index];
        } else {
            return NULL;
        }
    }
    std::map<std::string, std::string> _attrs;
    std::string _name;
    std::string _contents;
    std::string _cdata;
    xmlNode* _parent;
    std::vector<xmlNode*> _children;
    int32_t _iterator;
    int32_t _level;
    NodeType _type;
};

#endif
