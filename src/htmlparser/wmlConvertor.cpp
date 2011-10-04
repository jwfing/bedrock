#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>

#include "wmlConvertor.h"

#include "wmlRootNode.h"
#include "wmlAnchorNode.h"
#include "wmlCardNode.h"
#include "wmlDeleteNode.h"
#include "wmlDoNode.h"
#include "wmlGoNode.h"
#include "wmlPostFieldNode.h"

using namespace std;

std::set<std::string> generateDeleteNames()
{
    std::set<std::string> result;
    result.insert("timer");
    result.insert("setvar");
    result.insert("noop");
    result.insert("prev");
    result.insert("refresh");
    result.insert("onevent");
    result.insert("template");
    result.insert("access");
    return result;
}

std::set<std::string> wmlConvertor::_DELETENODES = generateDeleteNames();

wmlConvertor::wmlConvertor(const std::string& contents)
    : _level(0), _contents(contents), _root(NULL)
{
}

wmlConvertor::~wmlConvertor()
{
    if (NULL != _root) {
        delete _root;
        _root = NULL;
    }
}

bool wmlConvertor::isDeleteTag(const char* name)
{
    if (_DELETENODES.find(name) != _DELETENODES.end()) {
        return true;
    }
    return false;
}

void XMLCALL wmlConvertor::xmlDecl(void* data, const char* version,
       const char* encoding, int standalone)
{
    wmlConvertor* convertor = (wmlConvertor*)data;
    if (NULL == convertor) {
        return;
    }
    stringstream ss;
    ss << "<?xml ";
    if (NULL != version) {
        ss << " version=\"" << version << "\"";
    }
    if (NULL != encoding) {
        ss << " encoding=\"" << encoding << "\"";
    }
    ss << "?>";
    convertor->setXmlDecl(ss.str());
}

void XMLCALL wmlConvertor::startDocType(void* data, const char* docType,
       const char* sysid, const char* pubid, int hasSubject)
{
    wmlConvertor* convertor = (wmlConvertor*)data;
    if (NULL == convertor) {
        return;
    }
    convertor->setDocType(
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\""
        " \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">");
}

void XMLCALL wmlConvertor::endDocType(void* data)
{
    wmlConvertor* convertor = (wmlConvertor*)data;
    if (NULL == convertor) {
        return;
    }
}

void XMLCALL wmlConvertor::outputCharData(void* data, const char* s, int len)
{
    wmlConvertor* convertor = (wmlConvertor*)data;
    if (NULL == convertor) {
        return;
    }
    while (len >= 1 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\n' || s[len - 1] == '\r' )) {
        len--;
    }
    xmlNode* node = convertor->peekLastNode();
    if (NULL != node) {
        std::stringstream ss;
        for (int i = 0; i < len; i++) {
            ss << s[i];
        }
        xmlNode* sub = new xmlNode(node->getLevel() + 1, "", NT_TEXT);
        sub->setParent(node);
        node->addChild(sub);
        sub->setContents(ss.str());
    }
}

void XMLCALL wmlConvertor::startElement(void *data, const char *el, const char **attr)
{
    wmlConvertor* convertor = (wmlConvertor*)data;
    if (NULL == convertor) {
        return;
    }
    xmlNode* newNode = NULL;
    if (0 == strcmp(el, "wml")) {
        newNode = new wmlRootNode(convertor->getLevel(), el);
    } else if (0 == strcmp(el, "card")) {
        newNode = new wmlCardNode(convertor->getLevel(), el);
    } else if (0 == strcmp(el, "anchor")) {
        newNode = new wmlAnchorNode(convertor->getLevel(), el);
    } else if (0 == strcmp(el, "go")) {
        newNode = new wmlGoNode(convertor->getLevel(), el);
    } else if (0 == strcmp(el, "do")) {
        newNode = new wmlDoNode(convertor->getLevel(), el);
    } else if (0 == strcmp(el, "postfield")) {
        newNode = new wmlPostFieldNode(convertor->getLevel(), el);
    } else if (isDeleteTag(el)) {
        newNode = new wmlDeleteNode(convertor->getLevel(), el);
    } else {
        newNode = new xmlNode(convertor->getLevel(), el);
    }

    for (int i = 0; attr[i]; i += 2) {
        newNode->setAttr(attr[i], attr[i + 1]);
    }
    xmlNode* parent = convertor->peekLastNode();
    if (NULL != parent) {
        parent->addChild(newNode);
        newNode->setParent(parent);
    }
    convertor->pushLastNode(newNode);
    convertor->addLevel();
}

void XMLCALL wmlConvertor::endElement(void *data, const char *el)
{
    wmlConvertor* convertor = (wmlConvertor*)data;
    if (NULL == convertor) {
        return;
    }
    convertor->popLastNode();
    convertor->decLevel();
}

int wmlConvertor::convert()
{
    if (NULL != _root) {
        delete _root;
        _root = NULL;
    }
    _nodes.clear();
    _level = 0;
    _declare = "";
    _docType = "";

    XML_Parser parser = XML_ParserCreate(NULL);
    if (!parser) {
        fprintf(stderr, "%s\n", "couldn't allocate memory for parser.");
        return -1;
    }

    XML_SetUserData(parser, this);
    XML_SetXmlDeclHandler(parser, xmlDecl);
    XML_SetDoctypeDeclHandler(parser, startDocType, endDocType);
    XML_SetElementHandler(parser, startElement, endElement);
    XML_SetCharacterDataHandler(parser, outputCharData);

    const char* buff = _contents.c_str();
    int32_t len = _contents.length();
    if (XML_Parse(parser, buff, len, 1) == XML_STATUS_ERROR) {
        fprintf(stderr, "Parse error at line %lu:\n%s\n",
                XML_GetCurrentLineNumber(parser),
                XML_ErrorString(XML_GetErrorCode(parser)));
        XML_ParserFree(parser);
        return -1;
    } else {
        XML_ParserFree(parser);
        return 0;
    }
}

std::string wmlConvertor::toHTMLString()
{
    if (0 != convert()) {
        return "";
    } else if (NULL == _root){
        return "";
    } else {
        return _declare + "\n" + _docType + "\n" + _root->toString();
    }
}

std::string wmlConvertor::toHTMLString(const std::string& contents)
{
    _contents = contents;
    return toHTMLString();
}
