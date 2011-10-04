#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include <iostream>
#include "wmlConvertor.h"

using namespace std;

#define BUFFSIZE        8192
int main(int argc, char *argv[])
{
    std::stringstream contents;
    char Buff[BUFFSIZE];
    char* pBuff = NULL;
    bool atBeginofPage = true;
    for (;;) {
        int done;
        int len;
        len = (int)fread(Buff, 1, BUFFSIZE, stdin);
        if (ferror(stdin)) {
            fprintf(stderr, "Read error\n");
            exit(-1);
        }
        Buff[len] = '\0';
        pBuff = Buff;
        while(atBeginofPage && *pBuff
              && (*pBuff == ' ' || *pBuff == '\t'  || *pBuff == '\r' || *pBuff == '\n')) {
            pBuff++;
        }
        if (atBeginofPage && strlen(pBuff) > 0) {
            atBeginofPage = false;
        }
        contents << pBuff;
        done = feof(stdin);
        if (done)
          break;
    }
    {
        wmlConvertor conv(contents.str());
        cout << conv.toHTMLString();
    }
//    cout << endl;
//    cout << "memAC:" << memAllocCounter << endl;
    return 0;
}
