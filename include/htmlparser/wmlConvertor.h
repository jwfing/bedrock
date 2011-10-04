#ifndef _WMLCONVERTOR_INCLUDE_H_
#define _WMLCONVERTOR_INCLUDE_H_

#include <expat.h>
#include <string>
#include <set>
#include <deque>

#include "xmlnode.h"

class wmlConvertor
{
public:
    wmlConvertor(const std::string& contents);
    virtual ~wmlConvertor();
    std::string toHTMLString();
    std::string toHTMLString(const std::string& contents);
    void setXmlDecl(const std::string& decl) {
        _declare = decl;
    }
    void setDocType(const std::string& docType) {
        _docType = docType;
    }
    int32_t getLevel() const
    {
        return _level;
    }
    void addLevel() {
        ++_level;
    }
    void decLevel() {
        --_level;
    }
    xmlNode* peekLastNode() {
        if (_nodes.size() > 0) {
            return _nodes.back();
        } else {
            return NULL;
        }
    }
    void popLastNode() {
        _nodes.pop_back();
    }
    void pushLastNode(xmlNode* node) {
        if (NULL != node) {
            _nodes.push_back(node);
            if (_nodes.size() == 1) {
                _root = node;
            }
        }
    }

protected:
    static void XMLCALL
        xmlDecl(void* data, const char* version,
           const char* encoding, int standalone);
    static void XMLCALL
        startDocType(void* data, const char* docType,
           const char* sysid, const char* pubid, int hasSubject);
    static void XMLCALL
        endDocType(void* data);
    static void XMLCALL
        outputCharData(void* data, const char* s, int len);
    static void XMLCALL
        startElement(void *data, const char *el, const char **attr);
    static void XMLCALL
        endElement(void *data, const char *el);
    static bool isDeleteTag(const char* name);
    int convert();

private:
    static std::set<std::string> _DELETENODES;
    int32_t _level;
    std::deque<xmlNode*> _nodes;
    xmlNode* _root;
    std::string _declare;
    std::string _docType;
    std::string _contents;
};

#endif
