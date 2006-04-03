/*
 * ----------------------------------------------------------------------
 *  Rappture Library Entity Reference Translation Header
 *
 *  Begin Character Entity Translator
 *
 *  The next section of code implements routines used to translate
 *  character entity references into their corresponding strings.
 *
 *  Examples:
 *
 *        &amp;          "&"
 *        &lt;           "<"
 *        &gt;           ">"
 *        &nbsp;         " "
 *
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *  Also see below text for additional information on usage and redistribution
 *
 * ======================================================================
 */
/*----------------------------------------------------------------------------
|
|   Copyright (c) 2000  Jochen Loewer (loewerj@hotmail.com)
|-----------------------------------------------------------------------------
|
|
| !! EXPERIMENTAL / pre alpha !!
|   A simple (hopefully fast) HTML parser to build up a DOM structure
|   in memory.
|   Based on xmlsimple.c.
| !! EXPERIMENTAL / pre alpha !!
|
|
|   The contents of this file are subject to the Mozilla Public License
|   Version 1.1 (the "License"); you may not use this file except in
|   compliance with the License. You may obtain a copy of the License at
|   http://www.mozilla.org/MPL/
|
|   Software distributed under the License is distributed on an "AS IS"
|   basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
|   License for the specific language governing rights and limitations
|   under the License.
|
|   The Original Code is tDOM.
|
|   The Initial Developer of the Original Code is Jochen Loewer
|   Portions created by Jochen Loewer are Copyright (C) 1998, 1999
|   Jochen Loewer. All Rights Reserved.
|
|   Contributor(s):
|
|
|   written by Jochen Loewer
|   October 2000
|
|   ------------------------------------------------------------------------
|
|   A parser for XML.
|
|   Copyright (C) 1998 D. Richard Hipp
|
|   This library is free software; you can redistribute it and/or
|   modify it under the terms of the GNU Library General Public
|   License as published by the Free Software Foundation; either
|   version 2 of the License, or (at your option) any later version.
|
|   This library is distributed in the hope that it will be useful,
|   but WITHOUT ANY WARRANTY; without even the implied warranty of
|   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|   Library General Public License for more details.
|
|   You should have received a copy of the GNU Library General Public
|   License along with this library; if not, write to the
|   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
|   Boston, MA  02111-1307, USA.
|
|   Author contact information:
|     drh@acm.org
|     http://www.hwaci.com/drh/
|
\---------------------------------------------------------------------------*/

#include "RpEntityRef.h"
#include <cctype>
#include <cstring>

int RpEntityRef::bErNeedsInit = 1;
Er* RpEntityRef::apErHash[ER_HASH_SIZE];

/*----------------------------------------------------------------------------
|   ErHash  --
|
|       Hash an entity reference name.  The value returned is an
|       integer between 0 and Er_HASH_SIZE-1, inclusive.
|
\---------------------------------------------------------------------------*/
int 
RpEntityRef::ErHash( const char *zName)
{
    int h = 0;      /* The hash value to be returned */
    char c;         /* The next character in the name being hashed */

    while( (c=*zName)!=0 ){
        h = h<<5 ^ h ^ c;
        zName++;
    }
    if( h<0 ) h = -h;
    return h % ER_HASH_SIZE;

} /* ErHash */




/*----------------------------------------------------------------------------
|   ErInit --
|
|       Initialize the entity reference hash table
|
\---------------------------------------------------------------------------*/
void 
RpEntityRef::ErInit (void)
{
    int i;  /* For looping thru the list of entity references */
    int h;  /* The hash on a entity */

    for(i=0; ((unsigned)i)<sizeof(er_sequences)/sizeof(er_sequences[0]); i++){
        // hash the reference name
        h = ErHash(er_sequences[i].zName);
        er_sequences[i].pNextForw = apErHash[h];
        apErHash[h] = &er_sequences[i];
    }

} /* ErInit */


/*----------------------------------------------------------------------------
|    TranslateEntityRefs  --
|
|        Translate entity references and character references in the string
|        "z".  "z" is overwritten with the translated sequence.
|
|        Unrecognized entity references are unaltered.
|
|        Example:
|
|          input =    "AT&amp;T &gt MCI"
|          output =   "AT&T > MCI"
|
|        transDir - translation direction
|           FORWARD_TRANS is from &lt; to <
|           BACKWARD_TRANS is from < to &lt;
|
\---------------------------------------------------------------------------*/
void 
RpEntityRef::TranslateEntityRefs (char *z, int *newLen)
{
    int from;    /* Read characters from this position in z[] */
    int to;      /* Write characters into this position in z[] */
    int h;       /* A hash on the entity reference */
    char *zVal;  /* The substituted value */
    Er *p;       /* For looping down the entity reference collision chain */
    int value;

    from = to = 0;

    if (bErNeedsInit) {
        if (bErNeedsInit) {
            ErInit();
            bErNeedsInit = 0;
        }
    }

    while (z[from]) {
        if (z[from]=='&') {
            int i = from+1;
            int c;

            if (z[i] == '#') {
                /*---------------------------------------------
                |   convert character reference
                \--------------------------------------------*/
                value = 0;
                if (z[++i] == 'x') {
                    i++;
                    while ((c=z[i]) && (c!=';')) {
                        value = value * 16;
                        if ((c>='0') && (c<='9')) {
                            value += c-'0';
                        } else
                        if ((c>='A') && (c<='F')) {
                            value += c-'A' + 10;
                        } else
                        if ((c>='a') && (c<='f')) {
                            value += c-'a' + 10;
                        } else {
                            /* error */
                        }
                        i++;
                    }
                } else {
                    while ((c=z[i]) && (c!=';')) {
                        value = value * 10;
                        if ((c>='0') && (c<='9')) {
                            value += c-'0';
                        } else {
                            /* error */
                        }
                        i++;
                    }
                }
                if (z[i]!=';') {
                    /* error */
                }
                from = i+1;
#if RpER8Bits
                z[to++] = value;
#else 
                if (value < 0x80) {
                    z[to++] = value;
                } else if (value <= 0x7FF) {
                    z[to++] = (char) ((value >> 6) | 0xC0);
                    z[to++] = (char) ((value | 0x80) & 0xBF);
                } else if (value <= 0xFFFF) {
                    z[to++] = (char) ((value >> 12) | 0xE0);
                    z[to++] = (char) (((value >> 6) | 0x80) & 0xBF);
                    z[to++] = (char) ((value | 0x80) & 0xBF);
                } else {
                    /* error */
                }
#endif
            } else {
                while (z[i] && isalpha(z[i])) {
                   i++;
                }
                c = z[i];
                z[i] = 0;
                h = ErHash(&z[from+1]);
                p = apErHash[h];
                while (p && strncmp(p->zName,&z[from+1],strlen(p->zName))!=0 ) {
                    p = p->pNextForw;
                }
                z[i] = c;
                if (p) {
                    zVal = p->zValue;
                    while (*zVal) {
                        z[to++] = *(zVal++);
                    }
                    from = i;
                    if (c==';') from++;
                } else {
                    z[to++] = z[from++];
                }
            }
        } else {
            z[to++] = z[from++];
        }
    }
    z[to] = 0;
    *newLen = to;
}
/*----------------------------------------------------------------------------
|   End Of Character Entity Translator
\---------------------------------------------------------------------------*/


void 
RpEntityRef::appendEscaped (
    std::string& value,
    std::string& retStr
//    int         htmlEntities,
)
{
// #define AP(c)  *b++ = c;
// #define AE(s)  pc1 = s; while(*pc1) *b++ = *pc1++;
    unsigned int pos = 0;

    retStr = "";

    if (!value.empty()) {

        while (pos < value.length()) {
            if (value.at(pos) == '"') {
                retStr += "&quot;";
            }
            else if (value.at(pos) == '&') {
                retStr += "&amp;";
            }
            else if (value.at(pos) == '<') {
                retStr += "&lt;";
            }
            else if (value.at(pos) == '>') {
                retStr += "&gt;";
            }
            else if (value.at(pos) == '\n') {
                retStr = "&#xA;";
            } 
            else
            {
                retStr += value.at(pos);

            // this deals with all of the html entities
            /*
                charDone = 0;
                if (htmlEntities) {
                    charDone = 1;
#if RpER8Bits
                    switch ((unsigned int)*pc)
#else···········
                    Tcl_UtfToUniChar(pc, &uniChar);
                    switch (uniChar)·
#endif
                    {
                    case 0240: AE("&nbsp;");    break;·····
                    case 0241: AE("&iexcl;");   break;····
                    case 0242: AE("&cent;");    break;·····
                    case 0243: AE("&pound;");   break;····
                    case 0244: AE("&curren;");  break;···
                    case 0245: AE("&yen;");     break;······
                    case 0246: AE("&brvbar;");  break;···
                    case 0247: AE("&sect;");    break;·····
                    case 0250: AE("&uml;");     break;······
                    case 0251: AE("&copy;");    break;·····
                    case 0252: AE("&ordf;");    break;·····
                    case 0253: AE("&laquo;");   break;····
                    case 0254: AE("&not;");     break;······
                    case 0255: AE("&shy;");     break;······
                    case 0256: AE("&reg;");     break;······
                    case 0257: AE("&macr;");    break;·····
                    case 0260: AE("&deg;");     break;······
                    case 0261: AE("&plusmn;");  break;···
                    case 0262: AE("&sup2;");    break;·····
                    case 0263: AE("&sup3;");    break;·····
                    case 0264: AE("&acute;");   break;····
                    case 0265: AE("&micro;");   break;····
                    case 0266: AE("&para;");    break;·····
                    case 0267: AE("&middot;");  break;···
                    case 0270: AE("&cedil;");   break;····
                    case 0271: AE("&sup1;");    break;·····
                    case 0272: AE("&ordm;");    break;·····
                    case 0273: AE("&raquo;");   break;····
                    case 0274: AE("&frac14;");  break;···
                    case 0275: AE("&frac12;");  break;···
                    case 0276: AE("&frac34;");  break;···
                    case 0277: AE("&iquest;");  break;···
                    case 0300: AE("&Agrave;");  break;···
                    case 0301: AE("&Aacute;");  break;···
                    case 0302: AE("&Acirc;");   break;····
                    case 0303: AE("&Atilde;");  break;···
                    case 0304: AE("&Auml;");    break;·····
                    case 0305: AE("&Aring;");   break;····
                    case 0306: AE("&AElig;");   break;····
                    case 0307: AE("&Ccedil;");  break;···
                    case 0310: AE("&Egrave;");  break;···
                    case 0311: AE("&Eacute;");  break;···
                    case 0312: AE("&Ecirc;");   break;····
                    case 0313: AE("&Euml;");    break;·····
                    case 0314: AE("&Igrave;");  break;···
                    case 0315: AE("&Iacute;");  break;···
                    case 0316: AE("&Icirc;");   break;····
                    case 0317: AE("&Iuml;");    break;·····
                    case 0320: AE("&ETH;");     break;······
                    case 0321: AE("&Ntilde;");  break;···
                    case 0322: AE("&Ograve;");  break;···
                    case 0323: AE("&Oacute;");  break;···
                    case 0324: AE("&Ocirc;");   break;····
                    case 0325: AE("&Otilde;");  break;···
                    case 0326: AE("&Ouml;");    break;·····
                    case 0327: AE("&times;");   break;····
                    case 0330: AE("&Oslash;");  break;···
                    case 0331: AE("&Ugrave;");  break;···
                    case 0332: AE("&Uacute;");  break;···
                    case 0333: AE("&Ucirc;");   break;····
                    case 0334: AE("&Uuml;");    break;·····
                    case 0335: AE("&Yacute;");  break;···
                    case 0336: AE("&THORN;");   break;····
                    case 0337: AE("&szlig;");   break;····
                    case 0340: AE("&agrave;");  break;···
                    case 0341: AE("&aacute;");  break;···
                    case 0342: AE("&acirc;");   break;····
                    case 0343: AE("&atilde;");  break;···
                    case 0344: AE("&auml;");    break;·····
                    case 0345: AE("&aring;");   break;····
                    case 0346: AE("&aelig;");   break;····
                    case 0347: AE("&ccedil;");  break;···
                    case 0350: AE("&egrave;");  break;···
                    case 0351: AE("&eacute;");  break;···
                    case 0352: AE("&ecirc;");   break;····
                    case 0353: AE("&euml;");    break;·····
                    case 0354: AE("&igrave;");  break;···
                    case 0355: AE("&iacute;");  break;···
                    case 0356: AE("&icirc;");   break;····
                    case 0357: AE("&iuml;");    break;·····
                    case 0360: AE("&eth;");     break;······
                    case 0361: AE("&ntilde;");  break;···
                    case 0362: AE("&ograve;");  break;···
                    case 0363: AE("&oacute;");  break;···
                    case 0364: AE("&ocirc;");   break;····
                    case 0365: AE("&otilde;");  break;···
                    case 0366: AE("&ouml;");    break;·····
                    case 0367: AE("&divide;");  break;···
                    case 0370: AE("&oslash;");  break;···
                    case 0371: AE("&ugrave;");  break;···
                    case 0372: AE("&uacute;");  break;···
                    case 0373: AE("&ucirc;");   break;····
                    case 0374: AE("&uuml;");    break;·····
                    case 0375: AE("&yacute;");  break;···
                    case 0376: AE("&thorn;");   break;····
                    case 0377: AE("&yuml;");    break;·····
#if !TclOnly8Bits
                    // "Special" chars, according to XHTML xhtml-special.ent
                    case 338:  AE("&OElig;");   break;
                    case 339:  AE("&oelig;");   break;
                    case 352:  AE("&Scaron;");  break;
                    case 353:  AE("&scaron;");  break;
                    case 376:  AE("&Yuml;");    break;
                    case 710:  AE("&circ;");    break;
                    case 732:  AE("&tilde;");   break;
                    case 8194: AE("&ensp;");    break;
                    case 8195: AE("&emsp;");    break;
                    case 8201: AE("&thinsp;");  break;
                    case 8204: AE("&zwnj;");    break;
                    case 8205: AE("&zwj;");     break;
                    case 8206: AE("&lrm;");     break;
                    case 8207: AE("&rlm;");     break;
                    case 8211: AE("&ndash;");   break;
                    case 8212: AE("&mdash;");   break;
                    case 8216: AE("&lsquo;");   break;
                    case 8217: AE("&rsquo;");   break;
                    case 8218: AE("&sbquo;");   break;
                    case 8220: AE("&ldquo;");   break;
                    case 8221: AE("&rdquo;");   break;
                    case 8222: AE("&bdquo;");   break;
                    case 8224: AE("&dagger;");  break;
                    case 8225: AE("&Dagger;");  break;
                    case 8240: AE("&permil;");  break;
                    case 8249: AE("&lsaquo;");  break;
                    case 8250: AE("&rsaquo;");  break;
                    case 8364: AE("&euro;");    break;
                    // "Symbol" chars, according to XHTML xhtml-symbol.ent 
                    case 402:  AE("&fnof;");    break;·····
                    case 913:  AE("&Alpha;");   break;····
                    case 914:  AE("&Beta;");    break;·····
                    case 915:  AE("&Gamma;");   break;····
                    case 916:  AE("&Delta;");   break;····
                    case 917:  AE("&Epsilon;"); break;··
                    case 918:  AE("&Zeta;");    break;·····
                    case 919:  AE("&Eta;");     break;······
                    case 920:  AE("&Theta;");   break;····
                    case 921:  AE("&Iota;");    break;·····
                    case 922:  AE("&Kappa;");   break;····
                    case 923:  AE("&Lambda;");  break;···
                    case 924:  AE("&Mu;");      break;·······
                    case 925:  AE("&Nu;");      break;·······
                    case 926:  AE("&Xi;");      break;·······
                    case 927:  AE("&Omicron;"); break;··
                    case 928:  AE("&Pi;");      break;·······
                    case 929:  AE("&Rho;");     break;······
                    case 931:  AE("&Sigma;");   break;····
                    case 932:  AE("&Tau;");     break;······
                    case 933:  AE("&Upsilon;"); break;··
                    case 934:  AE("&Phi;");     break;······
                    case 935:  AE("&Chi;");     break;······
                    case 936:  AE("&Psi;");     break;······
                    case 937:  AE("&Omega;");   break;····
                    case 945:  AE("&alpha;");   break;····
                    case 946:  AE("&beta;");    break;·····
                    case 947:  AE("&gamma;");   break;····
                    case 948:  AE("&delta;");   break;····
                    case 949:  AE("&epsilon;"); break;··
                    case 950:  AE("&zeta;");    break;·····
                    case 951:  AE("&eta;");     break;······
                    case 952:  AE("&theta;");   break;····
                    case 953:  AE("&iota;");    break;·····
                    case 954:  AE("&kappa;");   break;····
                    case 955:  AE("&lambda;");  break;···
                    case 956:  AE("&mu;");      break;·······
                    case 957:  AE("&nu;");      break;·······
                    case 958:  AE("&xi;");      break;·······
                    case 959:  AE("&omicron;"); break;··
                    case 960:  AE("&pi;");      break;·······
                    case 961:  AE("&rho;");     break;······
                    case 962:  AE("&sigmaf;");  break;···
                    case 963:  AE("&sigma;");   break;····
                    case 964:  AE("&tau;");     break;······
                    case 965:  AE("&upsilon;"); break;··
                    case 966:  AE("&phi;");     break;······
                    case 967:  AE("&chi;");     break;······
                    case 968:  AE("&psi;");     break;······
                    case 969:  AE("&omega;");   break;····
                    case 977:  AE("&thetasym;");break;·
                    case 978:  AE("&upsih;");   break;····
                    case 982:  AE("&piv;");     break;······
                    case 8226: AE("&bull;");    break;·····
                    case 8230: AE("&hellip;");  break;···
                    case 8242: AE("&prime;");   break;····
                    case 8243: AE("&Prime;");   break;····
                    case 8254: AE("&oline;");   break;····
                    case 8260: AE("&frasl;");   break;····
                    case 8472: AE("&weierp;");  break;···
                    case 8465: AE("&image;");   break;····
                    case 8476: AE("&real;");    break;·····
                    case 8482: AE("&trade;");   break;····
                    case 8501: AE("&alefsym;"); break;··
                    case 8592: AE("&larr;");    break;·····
                    case 8593: AE("&uarr;");    break;·····
                    case 8594: AE("&rarr;");    break;·····
                    case 8595: AE("&darr;");    break;·····
                    case 8596: AE("&harr;");    break;·····
                    case 8629: AE("&crarr;");   break;····
                    case 8656: AE("&lArr;");    break;·····
                    case 8657: AE("&uArr;");    break;·····
                    case 8658: AE("&rArr;");    break;·····
                    case 8659: AE("&dArr;");    break;·····
                    case 8660: AE("&hArr;");    break;·····
                    case 8704: AE("&forall;");  break;···
                    case 8706: AE("&part;");    break;·····
                    case 8707: AE("&exist;");   break;····
                    case 8709: AE("&empty;");   break;····
                    case 8711: AE("&nabla;");   break;····
                    case 8712: AE("&isin;");    break;·····
                    case 8713: AE("&notin;");   break;····
                    case 8715: AE("&ni;");      break;·······
                    case 8719: AE("&prod;");    break;·····
                    case 8721: AE("&sum;");     break;······
                    case 8722: AE("&minus;");   break;····
                    case 8727: AE("&lowast;");  break;···
                    case 8730: AE("&radic;");   break;····
                    case 8733: AE("&prop;");    break;·····
                    case 8734: AE("&infin;");   break;····
                    case 8736: AE("&ang;");     break;······
                    case 8743: AE("&and;");     break;······
                    case 8744: AE("&or;");      break;·······
                    case 8745: AE("&cap;");     break;······
                    case 8746: AE("&cup;");     break;······
                    case 8747: AE("&int;");     break;······
                    case 8756: AE("&there4;");  break;···
                    case 8764: AE("&sim;");     break;······
                    case 8773: AE("&cong;");    break;·····
                    case 8776: AE("&asymp;");   break;····
                    case 8800: AE("&ne;");      break;·······
                    case 8801: AE("&equiv;");   break;····
                    case 8804: AE("&le;");      break;·······
                    case 8805: AE("&ge;");      break;·······
                    case 8834: AE("&sub;");     break;······
                    case 8835: AE("&sup;");     break;······
                    case 8836: AE("&nsub;");    break;·····
                    case 8838: AE("&sube;");    break;·····
                    case 8839: AE("&supe;");    break;·····
                    case 8853: AE("&oplus;");   break;····
                    case 8855: AE("&otimes;");  break;···
                    case 8869: AE("&perp;");    break;·····
                    case 8901: AE("&sdot;");    break;·····
                    case 8968: AE("&lceil;");   break;····
                    case 8969: AE("&rceil;");   break;····
                    case 8970: AE("&lfloor;");  break;···
                    case 8971: AE("&rfloor;");  break;···
                    case 9001: AE("&lang;");    break;·····
                    case 9002: AE("&rang;");    break;·····
                    case 9674: AE("&loz;");     break;······
                    case 9824: AE("&spades;");  break;···
                    case 9827: AE("&clubs;");   break;····
                    case 9829: AE("&hearts;");  break;···
                    case 9830: AE("&diams;");   break;····
#endif····················
                    default: charDone = 0;·
                    }
#if !TclOnly8Bits
                    if (charDone) {
                        clen = UTF8_CHAR_LEN(*pc);
                        pc += (clen - 1);
                    }
#endif      
                }
#if TclOnly8Bits
                if (!charDone) {
                    if (escapeNonASCII && ((unsigned char)*pc > 127)) {
                        AP('&') AP('#')
                        sprintf(charRef, "%d", (unsigned char)*pc);
                        for (i = 0; i < 3; i++) {
                            AP(charRef[i]);
                        }
                        AP(';')
                    } else {
                        AP(*pc);
                    }
                }
#else       
                if (!charDone) {
                    if ((unsigned char)*pc > 127) {
                        clen = UTF8_CHAR_LEN(*pc);
                        if (!clen) {
                            domPanic("tcldom_AppendEscaped: can only handle "
                                     "UTF-8 chars up to 3 bytes length");
                        }
                        if (escapeNonASCII) {
                            Tcl_UtfToUniChar(pc, &uniChar);
                            AP('&') AP('#')
                                sprintf(charRef, "%d", uniChar);
                            for (i = 0; i < strlen(charRef); i++) {
                                AP(charRef[i]);
                            }
                            AP(';')
                            pc += (clen - 1);
                        } else {
                            for (i = 0; i < clen; i++) {
                                AP(*pc);
                                pc++;
                            }
                            pc--;
                        }
                    } else {
                        AP(*pc);
                    }
                }
#endif
            */
            }
            pos++;
        }
    }
    else {
        retStr = value;
    }
}

