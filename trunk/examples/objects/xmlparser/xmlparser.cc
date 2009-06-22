#include <stdio.h>
#include <expat.h>
#include <string.h>
#include "RpInt.h"
#include "RpChain.h"
#include "RpPath.h"
#include "RpNumber.h"

#define BUFFSIZE        8192

char Buff[BUFFSIZE];

typedef struct {
    int skip;
    int depth;
    Rappture::Path *path;
    Rp_Chain *c;
    void *lastObj;
    int lastObjType;
    Rappture::Object *v;
} Parseinfo;

void
init_info(Parseinfo *info) {
    info->skip = 0;
    info->depth = 1;
    info->path = new Rappture::Path();
    info->c = Rp_ChainCreate();
    info->lastObj = NULL;
    info->lastObjType = 0;
    info->v = NULL;
}

int
should_skip(Parseinfo *inf, const char *el, const char **attr) {
    int skip = 0;

    if ( (*el == 's') && (strcmp("string",el) == 0) ) {
        skip = 1;
    }

    return skip;
}

static void XMLCALL
start(void *data, const char *el, const char **attr)
{
    Parseinfo *inf = (Parseinfo *) data;

    if ( (*el == 'i') && (strcmp("input",el) == 0) ) {
        inf->path->add(el);
    } else if ( (*el == 'n') && (strcmp("number",el) == 0) ) {
        inf->path->add(el);
        // null units kills number
        // Rappture::Number *n = new Rappture::Number(inf->path->path(NULL),NULL,0.0);
        Rappture::Number *n = new Rappture::Number(inf->path->path(),"",0.0);
        Rp_ChainAppend(inf->c,n);
        inf->v = n;
    } else {
        // add the tags to the hash table of the last object we created.
        // this means new objects must be declared here,
        // or they will become part of the last recognized object
        // this could lead to major problems.
        // we have no way to ignore stuff we don't recognize

        inf->path->add(el);
        if (inf->v) {
            // inf->v->property(el,);
        }
    }

    inf->depth++;
}

static void XMLCALL
end(void *data, const char *el)
{
    Parseinfo *inf = (Parseinfo *) data;
    inf->path->del();
    inf->depth--;
    // pop object off the end of the inf->c chain
    // add object to the tree object
}

void
rawstart(void *data, const char *el, const char **attr) {
    Parseinfo *inf = (Parseinfo *) data;

    if (! inf->skip) {
        if (should_skip(inf, el, attr)) {
            inf->skip = inf->depth;
        }
        else {
            start(inf, el, attr);     /* This does rest of start handling */
        }
    }

    inf->depth++;
}  /* End of rawstart */

void
rawend(void *data, const char *el) {
    Parseinfo *inf = (Parseinfo *) data;

    inf->depth--;

    if (! inf->skip) {
        end(inf, el);              /* This does rest of end handling */
    }

    if (inf->skip == inf->depth) {
        inf->skip = 0;
    }
}  /* End rawend */

/*
void
char_handler(void* data, XML_Char const* s, int len)
{
    int total = 0;
    int total_old = 0;
    scew_element* current = NULL;
    Parseinfo *inf = (Parseinfo *) data;

    if (inf == NULL) {
        return;
    }

    if (inf->v == NULL)
    {
        return;
    }

    if (current->contents != NULL)
    {
        total_old = scew_strlen(current->contents);
    }
    total = (total_old + len + 1) * sizeof(XML_Char);
    current->contents = (XML_Char*) realloc(current->contents, total);

    if (total_old == 0)
    {
        current->contents[0] = '\0';
    }

    scew_strncat(current->contents, s, len);
}
*/


int
main(int argc, char *argv[])
{
    Parseinfo info;
    XML_Parser p = XML_ParserCreate(NULL);
    if (! p) {
        fprintf(stderr, "Couldn't allocate memory for parser\n");
        exit(-1);
    }

    init_info(&info);
    XML_SetUserData(p,&info);

    XML_SetElementHandler(p, rawstart, rawend);
    //XML_SetDefaultHandler(p, char_handler);

    for (;;) {
        int done;
        int len;

        len = fread(Buff, 1, BUFFSIZE, stdin);
        if (ferror(stdin)) {
            fprintf(stderr, "Read error\n");
            exit(-1);
        }
        done = feof(stdin);

        if (XML_Parse(p, Buff, len, done) == XML_STATUS_ERROR) {
            fprintf(stderr, "Parse error at line %lu:\n%s\n",
                    XML_GetCurrentLineNumber(p),
                    XML_ErrorString(XML_GetErrorCode(p)));
            exit(-1);
        }

        if (done) {
            break;
        }
    }

    Rp_ChainLink *lv = NULL;
    Rappture::Number *n = NULL;
    lv = Rp_ChainFirstLink(info.c);
    while (lv) {
        n = (Rappture::Number *) Rp_ChainGetValue(lv);
        printf("%s\n",n->xml());
        lv = Rp_ChainNextLink(lv);
    }

    return 0;
}
