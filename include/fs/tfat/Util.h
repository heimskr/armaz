#pragma once

#include "Kernel.h"

#include <optional>
#include <string>

#define DEBUG_EVERYTHING
#define DEBUG_EXTRA
#define DEBUG_SETPTR
#define DEBUG_FATFIND
#define DEBUG_DIRREAD
#define DEBUG_NEWFILE
#define DEBUG_UTIMENS
#define INDENT_WIDTH 2

extern int debug_enable;
extern int debug_disable_method;
extern int debug_disable_external;
extern char indentation[81];
void dbg (const char *source, int line, const char *s, const char *s1);
void dbg2(const char *source, int line, const char *s, const char *s1, const char *s2);
void dbgn(const char *source, int line, const char *s, const char *s1, int64_t n);
void dbgh(const char *source, int line, const char *s, const char *s1, int64_t n);
void dbgtrace(const char *source, int line, const char *s);
void indent(int offset);

namespace Armaz::ThornFAT::Util {
	std::optional<std::string> pathFirst(std::string path, std::string *remainder);
	/**
	 * Returns the last component of a path.
	 * Examples: foo => foo, /foo => foo, /foo/ => foo, /foo/bar => bar, /foo/bar/ => bar
	 * basename() from libgen.h is similar to this function, but I wanted a function that allocates memory automatically
	 * and that doesn't return "/" or "."
	 */
	std::optional<std::string> pathLast(const char *);

	std::optional<std::string> pathParent(const char *);

	inline size_t blocks2count(const size_t blocks, const size_t block_size) {
		// The blocksize is in bytes, so we divide it by sizeof(block) to get the number of words.
		// Multiply that by the number of blocks to get how many block_t's would take up that many blocks.
		return blocks * (size_t) (block_size * sizeof(char) / sizeof(block_t));
	}
}

#define FDF "%ld"
#define SCS "%s%c%s"
#define SLS "%s%ld%s"
#define SDS "%s%d%s"
#define SUS "%s%u%s"
#define SSS "%s%s%s"
#define DSR A_DIM  "%s"   A_XDIM
#define DCR A_DIM  "%c"   A_XDIM
#define DLR A_DIM  "%ld"  A_XDIM
#define BDR A_BOLD "%d"   A_XBOLD
#define BOR A_BOLD "0%o"  A_XBOLD
#define BXR A_BOLD "0x%x" A_XBOLD
#define BLR A_BOLD "%ld"  A_XBOLD
#define BSR A_BOLD "%s"   A_XBOLD
#define BUR A_BOLD "%u"   A_XBOLD
#define BULR A_BOLD "%lu" A_XBOLD
#define BZR A_BOLD "%zu"  A_XBOLD
#define BFDR A_BOLD FDF   A_XBOLD
#define IBS(s)  A_BOLD       s A_XBOLD   // inline bold string
#define IDS(s)  A_DIM        s A_XDIM    // inline dim string
#define IBDS(s) A_BOLD A_DIM s A_XDIM    // inline bold & dim string
#define IUS(s)  A_UNDER      s A_XUNDER  // inline underlined string
#define IIS(s)  A_ITALIC     s A_XITALIC // inline italic string
#define IRS(s)  A_RED        s A_XCOLOR  // inline red string
#define IOS(s)  A_ORANGE     s A_XCOLOR  // inline orange string
#define IYS(s)  A_YELLOW     s A_XCOLOR  // inline yellow string
#define ICHS(s) A_CHARTREUSE s A_XCOLOR  // inline chartreuse string
#define IGS(s)  A_GREEN      s A_XCOLOR  // inline green string
#define ICS(s)  A_CYAN       s A_XCOLOR  // inline cyan string
#define ILS(s)  A_BLUE       s A_XCOLOR  // inline blue string
#define IMS(s)  A_MAGENTA    s A_XCOLOR  // inline magenta string
#define IPS(s)  A_PINK       s A_XCOLOR  // inline pink string
#define IKS(s)  A_SKY        s A_XCOLOR  // inline sky string
#define SCSS SCS " " // (post-)spaced string-char-string
#define DARR IDS("->") // dim arrow
#define DEQ  IDS("=")  // dim equals
#define UDARR  A_DIM UARR  A_XDIM
#define UDLARR A_DIM ULARR A_XDIM
#define UDBARR A_DIM UBARR A_XDIM
#define SUDARR  " " UDARR  " "
#define SUDLARR " " UDLARR " "
#define SUDBARR " " UDBARR " "
#define BSTR A_DIM "\"" A_XDIM BSR A_DIM "\"" A_XDIM
#define DO IDS("{")
#define DL IDS(":")
#define DM IDS(",")
#define DC IDS("}")
#define DS IDS(";")
#define DLS IDS(":") " " // dim colon, space
#define DMS IDS(",") " " // dim comma, space
#define CSS DLS BSTR     // colon, space, string
#define SDEQ " " IDS("=") " "// spaced dim equals
#define CHECKMARK "\xe2\x9c\x93"
#define VOLTAGE   "\xe2\x9a\xa1\xef\xb8\x8e"
#define YVOLTAGE IYS(VOLTAGE) " "
#define EMDASH    "\xe2\x80\x94"
#define XMARK     "\xe2\x9c\x98"
#define UBARR     "\xe2\x9f\xb7" // Unicode bidirectional arrow
#define ULARR     "\xe2\x86\x90" // Unicode left arrow
#define UARR      "\xe2\x86\x92" // Unicode (right) arrow
#define BXV       "\xe2\x94\x82" // vertical box character
#define RAINBOW9(c1, c2, c3, c4, c5, c6, c7, c8, c9) A_RED c1 A_ORANGE c2 A_YELLOW c3 A_CHARTREUSE c4 A_GREEN c5 A_CYAN c6 A_BLUE c7 A_MAGENTA c8 A_PINK c9
#define A_SOMEWHERE IDS("~") RAINBOW9("s", "o", "m", "e", "w", "h", "e", "r", "e") IDS("~")
#define A_RESET      "\e[0m"
#define A_BOLD       "\e[1m"
#define A_XBOLD      "\e[22m"
#define A_DIM        "\e[2m"
#define A_XDIM       "\e[22m"
#define A_ITALIC     "\e[3m"
#define A_XITALIC    "\e[23m"
#define A_UNDER      "\e[4m"
#define A_XUNDER     "\e[24m"
#define A_RED        "\e[31m"
#define A_ORANGE     "\e[38;5;208m"
#define A_YELLOW     "\e[33m"
#define A_CHARTREUSE "\e[38;5;112m"
#define A_GREEN      "\e[32m"
#define A_CYAN       "\e[36m"
#define A_BLUE       "\e[34m"
#define A_MAGENTA    "\e[38;5;128m"
#define A_PINK       "\e[38;5;219m"
#define A_SKY        "\e[38;5;153m"
#define A_XCOLOR     "\e[39m"
#define A_RESETLINE  "\e[2K\e[G"
#define SUB         IDS("\u2514") " "
#define S_INFO    "\e[34mi\e[0m "
#define S_WAITING "\e[2m\xe2\x80\xa6\e[0m "
#define S_DONE    "\e[1;32m\xe2\x9c\x93\e[0m "
#define S_WARN    "\e[1;33m!\e[0m "
#define S_ERROR   "\e[1;31mX\e[0m "

#define FILELINE __FILE__, __LINE__
#define REPORT printf("[%s:%d]\n", FILELINE)
#define BASEINDENT ""
#define LL BXV
#define LR BXV
#define DBGL { if (DEBUG_ENABLED) { printf(IDS("├─────────────────────────────────┼──┼─────────────┤") "\n"); FLOG; } }
#define MKTAG(fs, ls)   IDS(LL)  fs "%" str(TAG_WIDTH) "s" A_RESET " " ls "%4d" A_RESET IDS(LR) // identifies location
#define MKHEADER(style) IDS(LL " ") style "%" str(HEADER_WIDTH) "s" A_RESET " "         IDS(LR) // identifies function
#define MKCTAG(color) MKTAG(A_BOLD color, color)
#define MKCHEADER(color) MKHEADER(color)
#define MKPAIR(t, h) t "  " h "  " BASEINDENT "%s"
#define LOGHEADER MKHEADER(A_RESET)
#define LOGTAG MKTAG(A_BOLD A_DIM, A_DIM)
#define LOGPAIR LOGTAG "  " LOGHEADER " " BASEINDENT "%s"
#define LOGSET(s) FILELINE, s, indentation
#define DBGLOGSET(s) source, line, (s), indentation
#define LOGEND A_RESET "\n"
#define CUSTOMH(s, c) A_RESET c s A_RESET A_DIM
#define DIMH(s) CUSTOMH(s, A_DIM)
#define DEBUG_ENABLED (debug_enable == 1 && debug_disable_method == 0 && debug_disable_external != 1)
#define FREE(p) { if (p != NULL) { free(p); p = NULL; } }
#define DEBUG_DEBUG_ENABLED() printf("debug_enable == %d (1), debug_disable_method == %d (0), debug_disable_external " \
                                     "== %d (!1), logfile == %s (!NULL)\n -> %sabled\n", debug_enable,                 \
                                     debug_disable_method, debug_disable_external, logfile == NULL? "NULL" : "valid",  \
                                     DEBUG_ENABLED? "en" : "dis")
#define IFLOG if (1)
// #define IFLOGDBG if (DEBUG_ENABLED)
#define IFLOGDBG if (1)
#define FLOG
#ifdef DEBUG_EVERYTHING
#define HELLO(s) { printf(": %s(%s)\n", __func__, s); }
#else
#define HELLO(s) { }
#endif
#define  DBG(s, s1)		dbg(FILELINE, (s), (s1))
#define DBG2(s, s1, s2)	dbg2(FILELINE, (s), (s1), (s2))
#define DBGN(s, s1, n)	dbgn(FILELINE, (s), (s1), (n))
#define DBGH(s, s1, n)	dbgh(FILELINE, (s), (s1), (n))
#define DBGT(s)			dbgtrace(FILELINE, (s))
#define LOGPRINT(a...) { IFLOGDBG { printf(a); } }
#define DBGF(s, f, a...) { LOGPRINT(LOGPAIR " " f "\n", LOGSET(s), a); }
#define DIE(s, f, a...) { printf(MKPAIR(MKCTAG(A_RED), MKCHEADER(A_RED)) DIE_PREFIX f LOGEND, LOGSET(s), a); Kernel::perish(); }
#define DIES(s, m) { DIE(s, "%s", m); }
#define CHECK(s, f, a...) { if (status) { EXIT; DIE(s, f, a); } }
// #define CHECKS(s, e) CHECK(s, "%s: %s", e, STRERR(errno))
#define CHECKS(s, e) CHECK(s, "%s", e)
#define WARN(s, f, a...) LOGPRINT(MKPAIR(MKCTAG(A_YELLOW), MKCHEADER(A_YELLOW)) A_YELLOW f LOGEND, LOGSET(s), a)
#define WARNS(s, m)      WARN(s, "%s", m)
#define SUCC(s, f, a...) LOGPRINT(MKPAIR(MKCTAG(A_GREEN), MKCHEADER(A_GREEN)) A_GREEN f LOGEND, LOGSET(s), a)
#define SUCCS(s, m)      SUCC(s, "%s", m)
#define ERR(s, f, a...) CDBG(A_RED, s, f, a)
#define ERRS(s, m)      CDBGS(A_RED, s, m)
#define LINEUP() LOGPRINT("\e[A")
#define CDBG(c, s, f, a...) LOGPRINT(MKPAIR(MKCTAG(A_DIM c), MKCHEADER(c)) c f LOGEND, LOGSET(s), a)
#define CDBGS(c, s, m)      CDBG(c, s, "%s", m)
#define CDBGN(c, s, s1, n)  CDBG(c, s, "%s %d", s1, n)
#define CCDBG(c1, c2, s, f, a...) LOGPRINT(MKPAIR(MKCTAG(c1), MKCHEADER(c2)) f LOGEND, LOGSET(s), a)
#define CCDBGS(c1, c2, s, m)      CCDBG(c1, c2, s, "%s", m)
#define CCDBGN(c1, c2, s, s1, n)  CCDBG(c1, c2, s, "%s %d", s1, n)
#define DDBG(s, f, a...) CDBG(A_DIM, s, IDS(f), a)
#define DDBGS(s, m)      DDBG(s, "%s", m)
#ifdef DEBUG_EXTRA
#define DBGE   DBG
#define DBG2E  DBG2
#define DBGNE  DBGN
#define DBGHE  DBGH
#define DBGLE  DBGL
#define DBGFE  DBGF
#define WARNE  WARN
#define SUCCE  SUCC
#define SUCCSE SUCCS
#define DBGTE  DBGT
#define DBG_ONE()  { }
#define DBG_OFFE() { }
#else
#define DBGE(s, s1)       { }
#define DBG2E(s, s1, s2)  { }
#define DBGNE(s, s1, n)   { }
#define DBGHE(s, s1, n)   { }
#define DBGFE(s, f, a...) { }
#define WARNE(s, f, a...) { }
#define SUCCE(s, f, a...) { }
#define SUCCSE(s, m)      { }
#define DBGTE(s)          { }
#define DBGLE() { }
#define DBG_ONE DBG_ON
#define DBG_OFFE DBG_OFF
#endif
#define STRERR(n) strerror((n) < 0? -(n) : (n))
#ifdef DEBUG_SETPTR
#define SETPTR(ptr, to) { if ((ptr)) { CDBG(A_CHARTREUSE, "setptr", BSR SUDLARR BSR, str(ptr), str(to)); *(ptr) = (to); } }
#else
#define SETPTR(ptr, to) { if ((ptr)) { *(ptr) = (to); } }
#endif
#define CPYPTR(ptr, to, type) { if ((ptr)) { *(ptr) = (type *) malloc(sizeof(type)); memcpy(*(ptr), (to), sizeof(type)); } }
#ifndef MIN
#define MIN(x, y) ((y) < (x)? (y) : (x))
#endif
#ifndef MAX
#define MAX(x, y) ((x) < (y)? (y) : (x))
#endif
#define PLURALX(n, sg, pl) (n), ((n) == 1? (sg) : (pl))
#define PLURAL(n, s) PLURALX((n), "", (s))
#define PLURALS(n)   PLURAL( (n), "s")
#define PLURALY(n)   PLURALX((n), "y", "ies")
// #define RETNERRC    { int errcpy = errno; errno = 0; return -errcpy; }
// #define RETNERR()   { if (errno)         return -errno;   }
// #define IFERRNO(b)  { if (errno) { b;    return -errno; } }
// #define IFERRNOC(b) { if (errno) { b;    RETNERRC;      } }
#define IFSTATUS(b) { if (status < 0) { b; return status; } }
// #define RETNERRX()   IFERRNO({ EXIT; })
// #define IFERRNOXC(b) IFERRNOC({ b; EXIT; })
#define CHECKW(s1, s2, n) WARN(s1, s2 ": " BDR " (%s)", n, STRERR(n))
#define SCHECK(s1, s2)     IFSTATUS({ DBG_ON(); CHECKW(s1, s2, status); })
#define SCHECKX(s1, s2)    IFSTATUS({ DBG_ON(); CHECKW(s1, s2, status); EXIT; })
#define SCHECKE(s1, s2)    IFSTATUS({ CHECKW(s1, s2, status); DBG_ON(); })
#define SCHECKB(s1, s2, b) IFSTATUS({ CHECKW(s1, s2, status); b; })
#define SCHECKEX(s1, s2)   IFSTATUS({ CHECKW(s1, s2, status); DBG_ON(); EXIT; })
// #define ECHECK(s1, s2)     IFERRNO ({ DBG_ON(); CHECKW(s1, s2, errno); })
// #define ECHECKXC(s1, s2)   IFERRNOC({ DBG_ON(); CHECKW(s1, s2, errno); EXIT; })
#define CHECKBLOCKDIE(s, s1) { if (block < 1) { DIE(s, "%s: " BDR, s1, block);               } }
#define CHECKBLOCKRET(s, s1) { if (block < 1) { WARN(s, s1 ": " BDR, block); return -ESPIPE; } }
#define CHECKSEEK(s, s1, n)  { if (n < 0)     { WARN(s, s1 ": " BLR, n);     return -ESPIPE; } }
#define CHECKBLOCK CHECKBLOCKDIE
#define UNUSED(x) (void) (x)
#define METHOD_MID " called for "
#define METHOD_SUFFIX IDS("()")
#define XMETHOD(s, c) (c) (s) A_XCOLOR METHOD_SUFFIX METHOD_MID
#define BMETHOD(s)  IBS(s)  METHOD_SUFFIX METHOD_MID
#define CMETHOD(s)  ICS(s)  METHOD_SUFFIX METHOD_MID
#define MMETHOD(s)  IMS(s)  METHOD_SUFFIX METHOD_MID
#define CHMETHOD(s) ICHS(s) METHOD_SUFFIX METHOD_MID
#define PMETHOD(s)  IPS(s)  METHOD_SUFFIX METHOD_MID
#define OMETHOD(s)  IOS(s)  METHOD_SUFFIX METHOD_MID
#define RMETHOD(s)  IRS(s)  METHOD_SUFFIX METHOD_MID
#define KMETHOD(s)  IKS(s)  METHOD_SUFFIX METHOD_MID
#define BDMETHOD(s) IBDS(s) METHOD_SUFFIX METHOD_MID
#define METHOD_OFF(i) { debug_disable_method |=  (1 << i); }
#define METHOD_ON(i)  { debug_disable_method &= ~(1 << i); }
#define INDEX_GETATTR 0
#define INDEX_FATFIND 1
#define INDEX_DIRREAD 2
#define INDEX_RELEASE 3
#define INDEX_NEWFILE 4
#define INDEX_UTIMENS 5
#if INDENT_WIDTH != 0
#ifdef TRACE_INDENT
#define ENTER { IFLOG { printf("\e[A\e[165G" IDS("+%s") "\n", __FUNCTION__); } printf(IDS("+%s") "\n", __FUNCTION__); indent(1); }
#define EXIT  { IFLOG { printf("\e[A\e[165G" IDS("-%s") "\n", __FUNCTION__); } printf(IDS("-%s") "\n", __FUNCTION__); indent(-1); if (indentation[0] == '\0') printf("\n"); }
#else
#define ENTER indent(1)
#define EXIT  indent(-1)
#endif
#else
#define ENTER
#define EXIT
#endif
#define DBG_ON()  { debug_enable = 1; }
#define DBG_OFF() { debug_enable = 0; }
#define str(x) xstr(x)
#define xstr(x) #x
#ifndef DEBUG_FATFIND
#define FF_EXIT { METHOD_ON(INDEX_FATFIND); EXIT; }
#else
#define FF_EXIT { EXIT; }
#endif
#ifndef DEBUG_DIRREAD
#define DR_EXIT { METHOD_ON(INDEX_DIRREAD); EXIT; }
#else
#define DR_EXIT { EXIT; }
#endif
#ifndef DEBUG_NEWFILE
#define NF_EXIT { METHOD_ON(INDEX_NEWFILE); EXIT; }
#else
#define NF_EXIT { EXIT; }
#endif
#ifndef DEBUG_UTIMENS
#define UT_EXIT { METHOD_ON(INDEX_UTIMENS); }
#else
#define UT_EXIT { }
#endif
#define OPENH    A_RESET ICS("       open") A_DIM
#define UTIMENSH A_RESET IBS("    utimens") A_DIM
#define RELEASEH A_RESET IMS("    release") A_DIM
#define RENAMEH  A_RESET ICS("     rename") A_DIM
#define CREATEH  A_RESET IPS("     create") A_DIM
#define MKDIRH   A_RESET IPS("      mkdir") A_DIM
#define WRITEH   A_RESET IPS("      write") A_DIM
#define READDIRH A_RESET IOS("    readdir") A_DIM
#define READH    A_RESET IOS("       read") A_DIM
#define UNLINKH  A_RESET IRS("     unlink") A_DIM
#define RMDIRH   A_RESET IRS("       read") A_DIM
#define EXTRAH   A_RESET " " RAINBOW9("~", "~", "e", "x", "t", "r", "a", "~", "~") " " A_DIM
#ifdef ENABLE_SPAM
#define SKULL_COLOR "\e[1m"
#define SEGFAULT_COLOR "\e[38;5;88m"
#define BIG_SKULL "\e[38;5;88;1m    ..............\n\e[38;5;196;1m   ::::::::::::::::::\n\e[38;5;202;1m  :::::::::::::::\n\e[38;5;208;1m :::`::::::: :::     :    " A_RESET A_BOLD A_ITALIC "YIKES!" A_RESET " That's a crash!" A_RESET "\e[38;5;208;1m" "\n\e[38;5;142;1m :::: ::::: :::::    :    " A_RESET "Looks like you're having some trouble writing code." A_RESET "\e[38;5;142;1m" "\n\e[38;5;40;1m :`   :::::;     :..~~    " A_RESET "Try again and do it right this time. " A_DIM ":~)" A_RESET "\e[38;5;40;1m" "\n\e[38;5;44;1m :   ::  :::.     :::.\n\e[38;5;39;1m :...`:, :::::...:::\n\e[38;5;27;1m::::::.  :::::::::'      " A_RESET "Watch out for memleaks too. " A_DIM ";P" A_RESET "\e[38;5;27;1m" "\n\e[38;5;92;1m ::::::::|::::::::  !\n\e[38;5;88;1m :;;;;;;;;;;;;;;;;']}\n\e[38;5;196;1m ;--.--.--.--.--.-\n\e[38;5;202;1m  \\/ \\/ \\/ \\/ \\/ \\/\n\e[38;5;208;1m     :::       ::::\n\e[38;5;142;1m      :::      ::\n\e[38;5;40;1m     :\\:      ::\n\e[38;5;44;1m   /\\::    /\\:::    \n\e[38;5;39;1m ^.:^:.^^^::`::\n\e[38;5;27;1m ::::::::.::::\n\e[38;5;92;1m  .::::::::::"
#define BIG_SEGFAULT "  \u2588\u2588\u2588\u2588\u2588\u2588 \u2593\u2588\u2588\u2588\u2588\u2588   \u2584\u2588\u2588\u2588\u2588   \u2588\u2588\u2588\u2588\u2588\u2592\u2584\u2584\u2584       \u2588    \u2588\u2588  \u2588\u2588\u2593  \u2584\u2584\u2584\u2588\u2588\u2588\u2588\u2588\u2593\n\u2592\u2588\u2588    \u2592 \u2593\u2588   \u2580  \u2588\u2588\u2592 \u2580\u2588\u2592\u2593\u2588\u2588   \u2592\u2592\u2588\u2588\u2588\u2588\u2584     \u2588\u2588  \u2593\u2588\u2588\u2592\u2593\u2588\u2588\u2592  \u2593  \u2588\u2588\u2592 \u2593\u2592\n\u2591 \u2593\u2588\u2588\u2584   \u2592\u2588\u2588\u2588   \u2592\u2588\u2588\u2591\u2584\u2584\u2584\u2591\u2592\u2588\u2588\u2588\u2588 \u2591\u2592\u2588\u2588  \u2580\u2588\u2584  \u2593\u2588\u2588  \u2592\u2588\u2588\u2591\u2592\u2588\u2588\u2591  \u2592 \u2593\u2588\u2588\u2591 \u2592\u2591\n  \u2592   \u2588\u2588\u2592\u2592\u2593\u2588  \u2584 \u2591\u2593\u2588  \u2588\u2588\u2593\u2591\u2593\u2588\u2592  \u2591\u2591\u2588\u2588\u2584\u2584\u2584\u2584\u2588\u2588 \u2593\u2593\u2588  \u2591\u2588\u2588\u2591\u2592\u2588\u2588\u2591  \u2591 \u2593\u2588\u2588\u2593 \u2591 \n\u2592\u2588\u2588\u2588\u2588\u2588\u2588\u2592\u2592\u2591\u2592\u2588\u2588\u2588\u2588\u2592\u2591\u2592\u2593\u2588\u2588\u2588\u2580\u2592\u2591\u2592\u2588\u2591    \u2593\u2588   \u2593\u2588\u2588\u2592\u2592\u2592\u2588\u2588\u2588\u2588\u2588\u2593 \u2591\u2588\u2588\u2588\u2588\u2588\u2588\u2592\u2592\u2588\u2588\u2592 \u2591 \n\u2592 \u2592\u2593\u2592 \u2592 \u2591\u2591\u2591 \u2592\u2591 \u2591 \u2591\u2592   \u2592  \u2592 \u2591    \u2592\u2592   \u2593\u2592\u2588\u2591\u2591\u2592\u2593\u2592 \u2592 \u2592 \u2591 \u2592\u2591\u2593  \u2591\u2592 \u2591\u2591   \n\u2591 \u2591\u2592  \u2591 \u2591 \u2591 \u2591  \u2591  \u2591   \u2591  \u2591       \u2592   \u2592\u2592 \u2591\u2591\u2591\u2592\u2591 \u2591 \u2591 \u2591 \u2591 \u2592  \u2591  \u2591    \n\u2591  \u2591  \u2591     \u2591   \u2591 \u2591   \u2591  \u2591 \u2591     \u2591   \u2592    \u2591\u2591\u2591 \u2591 \u2591   \u2591 \u2591   \u2591      \n      \u2591     \u2591  \u2591      \u2591              \u2591  \u2591   \u2591         \u2591  \u2591       "
#define DIE_PREFIX
#define SEGFAULT_EXTRA "\e[B\n\e[14C\e[sWatch out for those tricky segfaults.\n\e[u\e[B\e[8C\e[34;4mhttps://xkcd.com/371/\e[0m"
static const char BigSkull[] = BIG_SKULL A_RESET;
static const char BigSegfault[] = BIG_SEGFAULT A_RESET SEGFAULT_EXTRA;
#else
#define DIE_PREFIX "FATAL:"
#endif
#define WRENTRYH     DIMH("write_entry")
#define RSUPERBLOCKH      "rsuperblock"
#define ZEROOUTFREEH      "zerooutfree"
#define FILEREADH           "readFile"
#define FATFINDH     A_RESET ICHS("       find") A_DIM
#define NEWFILEH     A_RESET  IPS("    newfile") A_DIM
#define RESIZEH      A_RESET  ICS("     resize") A_DIM
#define COMPACTH     A_RESET  IKS("    compact") A_DIM
#define REMOVEH      A_RESET  IRS("     remove") A_DIM
#define COUNTALLOCH  A_RESET  IKS("count_alloc") A_DIM
#define FORGETH      A_RESET  IRS("     forget") A_DIM
#define GETROOTH     A_RESET  IKS("   get_root") A_DIM
inline int IS_X(const char *p, const char *x) {
	auto l = Armaz::ThornFAT::Util::pathLast(p);
	return l.has_value() && *l == x;
}
#define IS_PAUSE(p) (IS_X((p), ".pause") || IS_X((p), ".stop") || IS_X((p), ".sigstop"))
#define IS_DOT(p) ((2 < strlen((p)) && (p)[0] == '/' && (p)[1] == '.') || IS_X((p), ".comp") || IS_PAUSE((p)))
#ifdef DEBUG_DRIVER
#define IGNORE_ECHO(p, n) { if (strcmp((p), "/.echo") == 0) { return n; } }
#define IGNORE_DOT(p, n)  { if (IS_DOT((p))) { return n; } }
#else
#define IGNORE_ECHO(p, n) { }
#define IGNORE_DOT(p, n) { }
#endif
#define PCFINDPATHH  A_RESET IMS("pc_findpath") A_DIM
#define PCINSERTH    A_RESET IMS("  pc_" A_PINK "insert") A_DIM
#define PCDOWNH      A_RESET IMS("    pc_" A_ORANGE "down") A_DIM
#define PCUPH        A_RESET IMS("      pc_" A_ORANGE "up") A_DIM
#define PCTRYLOCKH   A_RESET IMS(" pc_" A_ORANGE "trylock") A_DIM
#define PCALIVEDNH   A_RESET IOS(" alive_down") A_DIM
#define PCALIVEUPH   A_RESET IOS("   alive_up") A_DIM
#define PCRMVITEMH   A_RESET IMS("pc_" A_RED "rmv_item") A_DIM
#define PCRMVFDH     A_RESET IMS("  pc_" A_RED "rmv_fd") A_DIM
#define PCSAVEH      A_RESET IMS("    pc_save") A_DIM
#define PCFINDFDH    A_RESET IMS(" pc_" A_CHARTREUSE "find_fd") A_DIM
#define PCFINDOFFH   A_RESET IMS("pc_" A_CHARTREUSE "find_off") A_DIM
#define PCLINKH      A_RESET IMS("    pc_link") A_DIM
#define PCNEWH       A_RESET IMS("     pc_" A_PINK "new") A_DIM
#define FD_VALID(fd) ((fd) != UINT64_MAX)



#define CATCH_SIGSEGV // Coredumps won't be produced if this is enabled, but at least you'll get a cool message.
#define DEBUG_DRIVER  // Whether to perform certain actions in response to touching or writing various files.
#define ENABLE_SPAM   // Whether to print ASCII art when a crash is detected.
#define INDENT_MAX   80 // The maximum number of spaces of indentation in the debug output.
#define INDENT_WIDTH 2  // The number of spaces per indentation level in the debug output.
#define DEBUG_STRAIGHT  // Whether to use vertical box drawing characters instead of brackets in the debug output.
#define DEBUG_EXTRA     // Whether to enable less important debug messages.
// #define DEBUG_SETPTR // Whether to print all uses of SETPTR.
// #define DEBUG_LOCKS  // Whether to print messages when mutexes are locked or unlocked.

// When a directory is found to have more FAT blocks allocated to it than its size requires,
// SHRINK_DIRS will make file_read free up the extra FAT blocks.
#define SHRINK_DIRS

//////// Some methods are really spammy, so they're silent unless specifically enabled with their corresponding flags.
// #define DEBUG_GETATTR
// #define DEBUG_FATFIND
// #define DEBUG_DIRREAD
// #define DEBUG_RELEASE
// #define DEBUG_NEWFILE
// #define DEBUG_UTIMENS

//////// If defined, every method in driver.c will print its name to stdout as soon as it's called.
// #define DEBUG_EVERYTHING

#define TAG_WIDTH 8
#define HEADER_WIDTH 11
#define NEWFILE_SKIP_MAX 4
#define READDIR_MAX_INCLUDE 4
#define DEFAULT_BLOCKSIZE 8192

//////// These are options for fdstress.
// #define FDC_DEBUG_ON    // If undefined, the driver's logging will be externally disabled while fdstress is running.
// #define FDC_PAUSEAT 900 // If this is defined, fdstress will SIGSTOP itself once it reaches this number.
// #define FDC_SHOW_ALL    // Whether to print everything to the console instead of overwriting the same line.

//////// When specified, a constant value will be used instead of the current time.
// #define CONSTANT_TIME 837726600

//////// These options are ones that I almost never have on.
//////// They exist strictly for the convenience of anyone who uses this program.
// #define NOANSI           // This one doesn't even work anymore.
// #define NOWARN
// #define UTIMENS_CTIME    // See driver_utimens() for an explanation.
// #define RECKLESS
// #define ENABLE_BACKTRACE // Never implemented.
// #define TRACE_INDENT     // Useful for debugging indentation if some function seems to be missing an EXIT.
