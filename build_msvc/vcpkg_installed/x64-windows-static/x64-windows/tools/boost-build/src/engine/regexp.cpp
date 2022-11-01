#include "jam.h"
#include "regexp.h"
#include "output.h"
#include <stdio.h>
#include <ctype.h>
#ifndef ultrix
# include <stdlib.h>
#endif
#include <string.h>
#define END 0
#define BOL 1
#define EOL 2
#define ANY 3
#define ANYOF   4
#define ANYBUT  5
#define BRANCH  6
#define BACK    7
#define EXACTLY 8
#define NOTHING 9
#define STAR    10
#define PLUS    11
#define WORDA   12
#define WORDZ   13
#define OPEN    20
#define CLOSE   30
#define OP(p)   (*(p))
#define NEXT(p) (((*((p)+1)&0377)<<8) + (*((p)+2)&0377))
#define OPERAND(p)  ((p) + 3)
#ifndef CHARBITS
#define UCHARAT(p)  ((int32_t)*(const unsigned char *)(p))
#else
#define UCHARAT(p)  ((int32_t)*(p)&CHARBITS)
#endif
#define FAIL(m) { regerror(m); return(NULL); }
#define ISMULT(c)   ((c) == '*' || (c) == '+' || (c) == '?')
#define HASWIDTH    01
#define SIMPLE      02
#define SPSTART     04
#define WORST       0
static char *regparse;
static int32_t regnpar;
static char regdummy;
static char *regcode;
static int32_t regsize;
#ifndef STATIC
#define STATIC  static
#endif
STATIC char *reg( int32_t paren, int32_t *flagp );
STATIC char *regbranch( int32_t *flagp );
STATIC char *regpiece( int32_t *flagp );
STATIC char *regatom( int32_t *flagp );
STATIC char *regnode( int32_t op );
STATIC char *regnext( char *p );
STATIC void regc( int32_t b );
STATIC void reginsert( char op, char *opnd );
STATIC void regtail( char *p, char *val );
STATIC void regoptail( char *p, char *val );
#ifdef STRCSPN
STATIC int32_t strcspn();
#endif
regexp *
regcomp( const char *exp )
{
    regexp *r;
    char *scan;
    char *longest;
    int32_t len;
    int32_t flags;
    if (exp == NULL)
        FAIL("NULL argument");
#ifdef notdef
    if (exp[0] == '.' && exp[1] == '*') exp += 2;
#endif
    regparse = (char *)exp;
    regnpar = 1;
    regsize = 0;
    regcode = &regdummy;
    regc(MAGIC);
    if (reg(0, &flags) == NULL)
        return(NULL);
    if (regsize >= 32767L)
        FAIL("regexp too big");
    r = (regexp *)BJAM_MALLOC(sizeof(regexp) + regsize);
    if (r == NULL)
        FAIL("out of space");
    regparse = (char *)exp;
    regnpar = 1;
    regcode = r->program;
    regc(MAGIC);
    if (reg(0, &flags) == NULL)
        return(NULL);
    r->regstart = '\0';
    r->reganch = 0;
    r->regmust = NULL;
    r->regmlen = 0;
    scan = r->program+1;
    if (OP(regnext(scan)) == END) {
        scan = OPERAND(scan);
        if (OP(scan) == EXACTLY)
            r->regstart = *OPERAND(scan);
        else if (OP(scan) == BOL)
            r->reganch++;
        if (flags&SPSTART) {
            longest = NULL;
            len = 0;
            for (; scan != NULL; scan = regnext(scan))
                if (OP(scan) == EXACTLY && static_cast<int32_t>(strlen(OPERAND(scan))) >= len) {
                    longest = OPERAND(scan);
                    len = static_cast<int32_t>(strlen(OPERAND(scan)));
                }
            r->regmust = longest;
            r->regmlen = len;
        }
    }
    return(r);
}
static char *
reg(
    int32_t paren,
    int32_t *flagp )
{
    char *ret;
    char *br;
    char *ender;
    int32_t parno = 0;
    int32_t flags;
    *flagp = HASWIDTH;
    if (paren) {
        if (regnpar >= NSUBEXP)
            FAIL("too many ()");
        parno = regnpar;
        regnpar++;
        ret = regnode(OPEN+parno);
    } else
        ret = NULL;
    br = regbranch(&flags);
    if (br == NULL)
        return(NULL);
    if (ret != NULL)
        regtail(ret, br);
    else
        ret = br;
    if (!(flags&HASWIDTH))
        *flagp &= ~HASWIDTH;
    *flagp |= flags&SPSTART;
    while (*regparse == '|' || *regparse == '\n') {
        regparse++;
        br = regbranch(&flags);
        if (br == NULL)
            return(NULL);
        regtail(ret, br);
        if (!(flags&HASWIDTH))
            *flagp &= ~HASWIDTH;
        *flagp |= flags&SPSTART;
    }
    ender = regnode((paren) ? CLOSE+parno : END);
    regtail(ret, ender);
    for (br = ret; br != NULL; br = regnext(br))
        regoptail(br, ender);
    if (paren && *regparse++ != ')') {
        FAIL("unmatched ()");
    } else if (!paren && *regparse != '\0') {
        if (*regparse == ')') {
            FAIL("unmatched ()");
        } else
            FAIL("junk on end");
    }
    return(ret);
}
static char *
regbranch( int32_t *flagp )
{
    char *ret;
    char *chain;
    char *latest;
    int32_t flags;
    *flagp = WORST;
    ret = regnode(BRANCH);
    chain = NULL;
    while (*regparse != '\0' && *regparse != ')' &&
           *regparse != '\n' && *regparse != '|') {
        latest = regpiece(&flags);
        if (latest == NULL)
            return(NULL);
        *flagp |= flags&HASWIDTH;
        if (chain == NULL)
            *flagp |= flags&SPSTART;
        else
            regtail(chain, latest);
        chain = latest;
    }
    if (chain == NULL)
        (void) regnode(NOTHING);
    return(ret);
}
static char *
regpiece( int32_t *flagp )
{
    char *ret;
    char op;
    char *next;
    int32_t flags;
    ret = regatom(&flags);
    if (ret == NULL)
        return(NULL);
    op = *regparse;
    if (!ISMULT(op)) {
        *flagp = flags;
        return(ret);
    }
    if (!(flags&HASWIDTH) && op != '?')
        FAIL("*+ operand could be empty");
    *flagp = (op != '+') ? (WORST|SPSTART) : (WORST|HASWIDTH);
    if (op == '*' && (flags&SIMPLE))
        reginsert(STAR, ret);
    else if (op == '*') {
        reginsert(BRANCH, ret);
        regoptail(ret, regnode(BACK));
        regoptail(ret, ret);
        regtail(ret, regnode(BRANCH));
        regtail(ret, regnode(NOTHING));
    } else if (op == '+' && (flags&SIMPLE))
        reginsert(PLUS, ret);
    else if (op == '+') {
        next = regnode(BRANCH);
        regtail(ret, next);
        regtail(regnode(BACK), ret);
        regtail(next, regnode(BRANCH));
        regtail(ret, regnode(NOTHING));
    } else if (op == '?') {
        reginsert(BRANCH, ret);
        regtail(ret, regnode(BRANCH));
        next = regnode(NOTHING);
        regtail(ret, next);
        regoptail(ret, next);
    }
    regparse++;
    if (ISMULT(*regparse))
        FAIL("nested *?+");
    return(ret);
}
static char *
regatom( int32_t *flagp )
{
    char *ret;
    int32_t flags;
    *flagp = WORST;
    switch (*regparse++) {
    case '^':
        ret = regnode(BOL);
        break;
    case '$':
        ret = regnode(EOL);
        break;
    case '.':
        ret = regnode(ANY);
        *flagp |= HASWIDTH|SIMPLE;
        break;
    case '[': {
            int32_t classr;
            int32_t classend;
            if (*regparse == '^') {
                ret = regnode(ANYBUT);
                regparse++;
            } else
                ret = regnode(ANYOF);
            if (*regparse == ']' || *regparse == '-')
                regc(*regparse++);
            while (*regparse != '\0' && *regparse != ']') {
                if (*regparse == '-') {
                    regparse++;
                    if (*regparse == ']' || *regparse == '\0')
                        regc('-');
                    else {
                        classr = UCHARAT(regparse-2)+1;
                        classend = UCHARAT(regparse);
                        if (classr > classend+1)
                            FAIL("invalid [] range");
                        for (; classr <= classend; classr++)
                            regc(classr);
                        regparse++;
                    }
                } else
                    regc(*regparse++);
            }
            regc('\0');
            if (*regparse != ']')
                FAIL("unmatched []");
            regparse++;
            *flagp |= HASWIDTH|SIMPLE;
        }
        break;
    case '(':
        ret = reg(1, &flags);
        if (ret == NULL)
            return(NULL);
        *flagp |= flags&(HASWIDTH|SPSTART);
        break;
    case '\0':
    case '|':
    case '\n':
    case ')':
        FAIL("internal urp");
        break;
    case '?':
    case '+':
    case '*':
        FAIL("?+* follows nothing");
        break;
    case '\\':
        switch (*regparse++) {
        case '\0':
            FAIL("trailing \\");
            break;
        case '<':
            ret = regnode(WORDA);
            break;
        case '>':
            ret = regnode(WORDZ);
            break;
        default:
            goto de_fault;
        }
        break;
    de_fault:
    default:
        {
            char *regprev;
            char ch;
            regparse--;
            ret = regnode(EXACTLY);
            for ( regprev = 0 ; ; ) {
                ch = *regparse++;
                switch (*regparse) {
                default:
                    regc(ch);
                    break;
                case '.': case '[': case '(':
                case ')': case '|': case '\n':
                case '$': case '^':
                case '\0':
                magic:
                    regc(ch);
                    goto done;
                case '?': case '+': case '*':
                    if (!regprev)
                        goto magic;
                    regparse = regprev;
                    goto done;
                case '\\':
                    regc(ch);
                    switch (regparse[1]){
                    case '\0':
                    case '<':
                    case '>':
                        goto done;
                    default:
                        regprev = regparse;
                        regparse++;
                        continue;
                    }
                }
                regprev = regparse;
            }
        done:
            regc('\0');
            *flagp |= HASWIDTH;
            if (!regprev)
                *flagp |= SIMPLE;
        }
        break;
    }
    return(ret);
}
static char *
regnode( int32_t op )
{
    char *ret;
    char *ptr;
    ret = regcode;
    if (ret == &regdummy) {
        regsize += 3;
        return(ret);
    }
    ptr = ret;
    *ptr++ = op;
    *ptr++ = '\0';
    *ptr++ = '\0';
    regcode = ptr;
    return(ret);
}
static void
regc( int32_t b )
{
    if (regcode != &regdummy)
        *regcode++ = b;
    else
        regsize++;
}
static void
reginsert(
    char op,
    char *opnd )
{
    char *src;
    char *dst;
    char *place;
    if (regcode == &regdummy) {
        regsize += 3;
        return;
    }
    src = regcode;
    regcode += 3;
    dst = regcode;
    while (src > opnd)
        *--dst = *--src;
    place = opnd;
    *place++ = op;
    *place++ = '\0';
    *place++ = '\0';
}
static void
regtail(
    char *p,
    char *val )
{
    char *scan;
    char *temp;
    size_t offset;
    if (p == &regdummy)
        return;
    scan = p;
    for (;;) {
        temp = regnext(scan);
        if (temp == NULL)
            break;
        scan = temp;
    }
    if (OP(scan) == BACK)
        offset = scan - val;
    else
        offset = val - scan;
    *(scan+1) = (offset>>8)&0377;
    *(scan+2) = offset&0377;
}
static void
regoptail(
    char *p,
    char *val )
{
    if (p == NULL || p == &regdummy || OP(p) != BRANCH)
        return;
    regtail(OPERAND(p), val);
}
static const char *reginput;
static const char *regbol;
static const char **regstartp;
static const char **regendp;
STATIC int32_t regtry( regexp *prog, const char *string );
STATIC int32_t regmatch( char *prog );
STATIC int32_t regrepeat( char *p );
#ifdef DEBUG
int32_t regnarrate = 0;
void regdump();
STATIC char *regprop();
#endif
int32_t
regexec(
    regexp *prog,
    const char *string )
{
    char *s;
    if (prog == NULL || string == NULL) {
        regerror("NULL parameter");
        return(0);
    }
    if (UCHARAT(prog->program) != MAGIC) {
        regerror("corrupted program");
        return(0);
    }
    if ( prog->regmust != NULL )
    {
        s = (char *)string;
        while ( ( s = strchr( s, prog->regmust[ 0 ] ) ) != NULL )
        {
            if ( !strncmp( s, prog->regmust, prog->regmlen ) )
                break;
            ++s;
        }
        if ( s == NULL )
            return 0;
    }
    regbol = (char *)string;
    if ( prog->reganch )
        return regtry( prog, string );
    s = (char *)string;
    if (prog->regstart != '\0')
        while ((s = strchr(s, prog->regstart)) != NULL) {
            if (regtry(prog, s))
                return(1);
            s++;
        }
    else
        do {
            if ( regtry( prog, s ) )
                return( 1 );
        } while ( *s++ != '\0' );
    return 0;
}
static int32_t
regtry(
    regexp *prog,
    const char *string )
{
    int32_t i;
    const char * * sp;
    const char * * ep;
    reginput = string;
    regstartp = prog->startp;
    regendp = prog->endp;
    sp = prog->startp;
    ep = prog->endp;
    for ( i = NSUBEXP; i > 0; --i )
    {
        *sp++ = NULL;
        *ep++ = NULL;
    }
    if ( regmatch( prog->program + 1 ) )
    {
        prog->startp[ 0 ] = string;
        prog->endp[ 0 ] = reginput;
        return 1;
    }
    else
        return 0;
}
static int32_t
regmatch( char * prog )
{
    char * scan;
    char * next;
    scan = prog;
#ifdef DEBUG
    if (scan != NULL && regnarrate)
        err_printf("%s(\n", regprop(scan));
#endif
    while (scan != NULL) {
#ifdef DEBUG
        if (regnarrate)
            err_printf("%s...\n", regprop(scan));
#endif
        next = regnext(scan);
        switch (OP(scan)) {
        case BOL:
            if (reginput != regbol)
                return(0);
            break;
        case EOL:
            if (*reginput != '\0')
                return(0);
            break;
        case WORDA:
            if ((!isalnum(*reginput)) && *reginput != '_')
                return(0);
            if (reginput > regbol &&
                (isalnum(reginput[-1]) || reginput[-1] == '_'))
                return(0);
            break;
        case WORDZ:
            if (isalnum(*reginput) || *reginput == '_')
                return(0);
            break;
        case ANY:
            if (*reginput == '\0')
                return(0);
            reginput++;
            break;
        case EXACTLY: {
                size_t len;
                char *opnd;
                opnd = OPERAND(scan);
                if (*opnd != *reginput)
                    return(0);
                len = strlen(opnd);
                if (len > 1 && strncmp(opnd, reginput, len) != 0)
                    return(0);
                reginput += len;
            }
            break;
        case ANYOF:
            if (*reginput == '\0' || strchr(OPERAND(scan), *reginput) == NULL)
                return(0);
            reginput++;
            break;
        case ANYBUT:
            if (*reginput == '\0' || strchr(OPERAND(scan), *reginput) != NULL)
                return(0);
            reginput++;
            break;
        case NOTHING:
            break;
        case BACK:
            break;
        case OPEN+1:
        case OPEN+2:
        case OPEN+3:
        case OPEN+4:
        case OPEN+5:
        case OPEN+6:
        case OPEN+7:
        case OPEN+8:
        case OPEN+9: {
                int32_t no;
                const char *save;
                no = OP(scan) - OPEN;
                save = reginput;
                if (regmatch(next)) {
                    if (regstartp[no] == NULL)
                        regstartp[no] = save;
                    return(1);
                } else
                    return(0);
            }
            break;
        case CLOSE+1:
        case CLOSE+2:
        case CLOSE+3:
        case CLOSE+4:
        case CLOSE+5:
        case CLOSE+6:
        case CLOSE+7:
        case CLOSE+8:
        case CLOSE+9: {
                int32_t no;
                const char *save;
                no = OP(scan) - CLOSE;
                save = reginput;
                if (regmatch(next)) {
                    if (regendp[no] == NULL)
                        regendp[no] = save;
                    return(1);
                } else
                    return(0);
            }
            break;
        case BRANCH: {
                const char *save;
                if (OP(next) != BRANCH)
                    next = OPERAND(scan);
                else {
                    do {
                        save = reginput;
                        if (regmatch(OPERAND(scan)))
                            return(1);
                        reginput = save;
                        scan = regnext(scan);
                    } while (scan != NULL && OP(scan) == BRANCH);
                    return(0);
                }
            }
            break;
        case STAR:
        case PLUS: {
                char nextch;
                int32_t no;
                const char *save;
                int32_t min;
                nextch = '\0';
                if (OP(next) == EXACTLY)
                    nextch = *OPERAND(next);
                min = (OP(scan) == STAR) ? 0 : 1;
                save = reginput;
                no = regrepeat(OPERAND(scan));
                while (no >= min) {
                    if (nextch == '\0' || *reginput == nextch)
                        if (regmatch(next))
                            return(1);
                    no--;
                    reginput = save + no;
                }
                return(0);
            }
            break;
        case END:
            return(1);
            break;
        default:
            regerror("memory corruption");
            return(0);
            break;
        }
        scan = next;
    }
    regerror("corrupted pointers");
    return(0);
}
static int32_t
regrepeat( char *p )
{
    int32_t count = 0;
    const char *scan;
    char *opnd;
    scan = reginput;
    opnd = OPERAND(p);
    switch (OP(p)) {
    case ANY:
        count = int32_t(strlen(scan));
        scan += count;
        break;
    case EXACTLY:
        while (*opnd == *scan) {
            count++;
            scan++;
        }
        break;
    case ANYOF:
        while (*scan != '\0' && strchr(opnd, *scan) != NULL) {
            count++;
            scan++;
        }
        break;
    case ANYBUT:
        while (*scan != '\0' && strchr(opnd, *scan) == NULL) {
            count++;
            scan++;
        }
        break;
    default:
        regerror("internal foulup");
        count = 0;
        break;
    }
    reginput = scan;
    return(count);
}
static char *
regnext( char *p )
{
    int32_t offset;
    if (p == &regdummy)
        return(NULL);
    offset = NEXT(p);
    if (offset == 0)
        return(NULL);
    if (OP(p) == BACK)
        return(p-offset);
    else
        return(p+offset);
}
#ifdef DEBUG
STATIC char *regprop();
void
regdump( regexp *r )
{
    char *s;
    char op = EXACTLY;
    char *next;
    s = r->program + 1;
    while (op != END) {
        op = OP(s);
        out_printf("%2d%s", s-r->program, regprop(s));
        next = regnext(s);
        if (next == NULL)
            out_printf("(0)");
        else
            out_printf("(%d)", (s-r->program)+(next-s));
        s += 3;
        if (op == ANYOF || op == ANYBUT || op == EXACTLY) {
            while (*s != '\0') {
                out_putc(*s);
                s++;
            }
            s++;
        }
        out_putc('\n');
    }
    if (r->regstart != '\0')
        out_printf("start `%c' ", r->regstart);
    if (r->reganch)
        out_printf("anchored ");
    if (r->regmust != NULL)
        out_printf("must have \"%s\"", r->regmust);
    out_printf("\n");
}
static char *
regprop( char *op )
{
    char *p;
    static char buf[50];
    (void) strcpy(buf, ":");
    switch (OP(op)) {
    case BOL:
        p = "BOL";
        break;
    case EOL:
        p = "EOL";
        break;
    case ANY:
        p = "ANY";
        break;
    case ANYOF:
        p = "ANYOF";
        break;
    case ANYBUT:
        p = "ANYBUT";
        break;
    case BRANCH:
        p = "BRANCH";
        break;
    case EXACTLY:
        p = "EXACTLY";
        break;
    case NOTHING:
        p = "NOTHING";
        break;
    case BACK:
        p = "BACK";
        break;
    case END:
        p = "END";
        break;
    case OPEN+1:
    case OPEN+2:
    case OPEN+3:
    case OPEN+4:
    case OPEN+5:
    case OPEN+6:
    case OPEN+7:
    case OPEN+8:
    case OPEN+9:
        sprintf(buf+strlen(buf), "OPEN%d", OP(op)-OPEN);
        p = NULL;
        break;
    case CLOSE+1:
    case CLOSE+2:
    case CLOSE+3:
    case CLOSE+4:
    case CLOSE+5:
    case CLOSE+6:
    case CLOSE+7:
    case CLOSE+8:
    case CLOSE+9:
        sprintf(buf+strlen(buf), "CLOSE%d", OP(op)-CLOSE);
        p = NULL;
        break;
    case STAR:
        p = "STAR";
        break;
    case PLUS:
        p = "PLUS";
        break;
    case WORDA:
        p = "WORDA";
        break;
    case WORDZ:
        p = "WORDZ";
        break;
    default:
        regerror("corrupted opcode");
        break;
    }
    if (p != NULL)
        (void) strcat(buf, p);
    return(buf);
}
#endif
#ifdef STRCSPN
static int32_t
strcspn(
    char *s1,
    char *s2 )
{
    char *scan1;
    char *scan2;
    int32_t count;
    count = 0;
    for (scan1 = s1; *scan1 != '\0'; scan1++) {
        for (scan2 = s2; *scan2 != '\0';)
            if (*scan1 == *scan2++)
                return(count);
        count++;
    }
    return(count);
}
#endif
