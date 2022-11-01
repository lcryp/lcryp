#define YYBISON 30802
#define YYBISON_VERSION "3.8.2"
#define YYSKELETON_NAME "yacc.c"
#define YYPURE 0
#define YYPUSH 0
#define YYPULL 1
#line 99 "src/engine/jamgram.y"
#include "jam.h"
#include "lists.h"
#include "parse.h"
#include "scan.h"
#include "compile.h"
#include "object.h"
#include "rules.h"
# define YYINITDEPTH 5000
# define YYMAXDEPTH 10000
# define F0 -1
static PARSE * P0 = nullptr;
# define S0 (OBJECT *)0
# define pappend( l,r )    	parse_make( PARSE_APPEND,l,r,P0,S0,S0,0 )
# define peval( c,l,r )	    parse_make( PARSE_EVAL,l,r,P0,S0,S0,c )
# define pfor( s,l,r,x )    parse_make( PARSE_FOREACH,l,r,P0,s,S0,x )
# define pif( l,r,t )       parse_make( PARSE_IF,l,r,t,S0,S0,0 )
# define pincl( l )         parse_make( PARSE_INCLUDE,l,P0,P0,S0,S0,0 )
# define plist( s )         parse_make( PARSE_LIST,P0,P0,P0,s,S0,0 )
# define plocal( l,r,t )    parse_make( PARSE_LOCAL,l,r,t,S0,S0,0 )
# define pmodule( l,r )     parse_make( PARSE_MODULE,l,r,P0,S0,S0,0 )
# define pclass( l,r )      parse_make( PARSE_CLASS,l,r,P0,S0,S0,0 )
# define pnull()            parse_make( PARSE_NULL,P0,P0,P0,S0,S0,0 )
# define pon( l,r )         parse_make( PARSE_ON,l,r,P0,S0,S0,0 )
# define prule( s,p )       parse_make( PARSE_RULE,p,P0,P0,s,S0,0 )
# define prules( l,r )      parse_make( PARSE_RULES,l,r,P0,S0,S0,0 )
# define pset( l,r,a )      parse_make( PARSE_SET,l,r,P0,S0,S0,a )
# define pset1( l,r,t,a )   parse_make( PARSE_SETTINGS,l,r,t,S0,S0,a )
# define psetc( s,p,a,l )   parse_make( PARSE_SETCOMP,p,a,P0,s,S0,l )
# define psete( s,l,s1,f )  parse_make( PARSE_SETEXEC,l,P0,P0,s,s1,f )
# define pswitch( l,r )     parse_make( PARSE_SWITCH,l,r,P0,S0,S0,0 )
# define pwhile( l,r )      parse_make( PARSE_WHILE,l,r,P0,S0,S0,0 )
# define preturn( l )       parse_make( PARSE_RETURN,l,P0,P0,S0,S0,0 )
# define pbreak()           parse_make( PARSE_BREAK,P0,P0,P0,S0,S0,0 )
# define pcontinue()        parse_make( PARSE_CONTINUE,P0,P0,P0,S0,S0,0 )
# define pnode( l,r )       parse_make( F0,l,r,P0,S0,S0,0 )
# define psnode( s,l )      parse_make( F0,l,P0,P0,s,S0,0 )
#line 116 "src/engine/jamgram.cpp"
# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif
#include "jamgram.hpp"
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,
  YYSYMBOL_YYerror = 1,
  YYSYMBOL_YYUNDEF = 2,
  YYSYMBOL__BANG_t = 3,
  YYSYMBOL__BANG_EQUALS_t = 4,
  YYSYMBOL__AMPER_t = 5,
  YYSYMBOL__AMPERAMPER_t = 6,
  YYSYMBOL__LPAREN_t = 7,
  YYSYMBOL__RPAREN_t = 8,
  YYSYMBOL__PLUS_EQUALS_t = 9,
  YYSYMBOL__COLON_t = 10,
  YYSYMBOL__SEMIC_t = 11,
  YYSYMBOL__LANGLE_t = 12,
  YYSYMBOL__LANGLE_EQUALS_t = 13,
  YYSYMBOL__EQUALS_t = 14,
  YYSYMBOL__RANGLE_t = 15,
  YYSYMBOL__RANGLE_EQUALS_t = 16,
  YYSYMBOL__QUESTION_EQUALS_t = 17,
  YYSYMBOL__LBRACKET_t = 18,
  YYSYMBOL__RBRACKET_t = 19,
  YYSYMBOL_ACTIONS_t = 20,
  YYSYMBOL_BIND_t = 21,
  YYSYMBOL_BREAK_t = 22,
  YYSYMBOL_CASE_t = 23,
  YYSYMBOL_CLASS_t = 24,
  YYSYMBOL_CONTINUE_t = 25,
  YYSYMBOL_DEFAULT_t = 26,
  YYSYMBOL_ELSE_t = 27,
  YYSYMBOL_EXISTING_t = 28,
  YYSYMBOL_FOR_t = 29,
  YYSYMBOL_IF_t = 30,
  YYSYMBOL_IGNORE_t = 31,
  YYSYMBOL_IN_t = 32,
  YYSYMBOL_INCLUDE_t = 33,
  YYSYMBOL_LOCAL_t = 34,
  YYSYMBOL_MODULE_t = 35,
  YYSYMBOL_ON_t = 36,
  YYSYMBOL_PIECEMEAL_t = 37,
  YYSYMBOL_QUIETLY_t = 38,
  YYSYMBOL_RETURN_t = 39,
  YYSYMBOL_RULE_t = 40,
  YYSYMBOL_SWITCH_t = 41,
  YYSYMBOL_TOGETHER_t = 42,
  YYSYMBOL_UPDATED_t = 43,
  YYSYMBOL_WHILE_t = 44,
  YYSYMBOL__LBRACE_t = 45,
  YYSYMBOL__BAR_t = 46,
  YYSYMBOL__BARBAR_t = 47,
  YYSYMBOL__RBRACE_t = 48,
  YYSYMBOL_ARG = 49,
  YYSYMBOL_STRING = 50,
  YYSYMBOL_YYACCEPT = 51,
  YYSYMBOL_run = 52,
  YYSYMBOL_block = 53,
  YYSYMBOL_rules = 54,
  YYSYMBOL_55_1 = 55,
  YYSYMBOL_56_2 = 56,
  YYSYMBOL_null = 57,
  YYSYMBOL_assign_list_opt = 58,
  YYSYMBOL_59_3 = 59,
  YYSYMBOL_arglist_opt = 60,
  YYSYMBOL_local_opt = 61,
  YYSYMBOL_else_opt = 62,
  YYSYMBOL_rule = 63,
  YYSYMBOL_64_4 = 64,
  YYSYMBOL_65_5 = 65,
  YYSYMBOL_66_6 = 66,
  YYSYMBOL_67_7 = 67,
  YYSYMBOL_68_8 = 68,
  YYSYMBOL_69_9 = 69,
  YYSYMBOL_70_10 = 70,
  YYSYMBOL_71_11 = 71,
  YYSYMBOL_72_12 = 72,
  YYSYMBOL_73_13 = 73,
  YYSYMBOL_74_14 = 74,
  YYSYMBOL_75_15 = 75,
  YYSYMBOL_76_16 = 76,
  YYSYMBOL_77_17 = 77,
  YYSYMBOL_78_18 = 78,
  YYSYMBOL_79_19 = 79,
  YYSYMBOL_80_20 = 80,
  YYSYMBOL_81_21 = 81,
  YYSYMBOL_82_22 = 82,
  YYSYMBOL_83_23 = 83,
  YYSYMBOL_84_24 = 84,
  YYSYMBOL_85_25 = 85,
  YYSYMBOL_86_26 = 86,
  YYSYMBOL_assign = 87,
  YYSYMBOL_expr = 88,
  YYSYMBOL_89_27 = 89,
  YYSYMBOL_90_28 = 90,
  YYSYMBOL_91_29 = 91,
  YYSYMBOL_92_30 = 92,
  YYSYMBOL_93_31 = 93,
  YYSYMBOL_94_32 = 94,
  YYSYMBOL_95_33 = 95,
  YYSYMBOL_96_34 = 96,
  YYSYMBOL_97_35 = 97,
  YYSYMBOL_98_36 = 98,
  YYSYMBOL_99_37 = 99,
  YYSYMBOL_100_38 = 100,
  YYSYMBOL_101_39 = 101,
  YYSYMBOL_cases = 102,
  YYSYMBOL_case = 103,
  YYSYMBOL_104_40 = 104,
  YYSYMBOL_105_41 = 105,
  YYSYMBOL_lol = 106,
  YYSYMBOL_list = 107,
  YYSYMBOL_listp = 108,
  YYSYMBOL_arg = 109,
  YYSYMBOL_110_42 = 110,
  YYSYMBOL_func = 111,
  YYSYMBOL_112_43 = 112,
  YYSYMBOL_113_44 = 113,
  YYSYMBOL_114_45 = 114,
  YYSYMBOL_eflags = 115,
  YYSYMBOL_eflag = 116,
  YYSYMBOL_bindlist = 117,
  YYSYMBOL_118_46 = 118
};
typedef enum yysymbol_kind_t yysymbol_kind_t;
#ifdef short
# undef short
#endif
#ifndef __PTRDIFF_MAX__
# include <limits.h>
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h>
#  define YY_STDINT_H
# endif
#endif
#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif
#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif
#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif
#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif
#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h>
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif
#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h>
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif
#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))
#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))
typedef yytype_uint8 yy_state_t;
typedef int yy_state_fast_t;
#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h>
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif
#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif
#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E)
#endif
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value)
#endif
#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif
#define YY_ASSERT(E) ((void) (0 && (E)))
#if !defined yyoverflow
# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h>
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h>
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h>
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif
# ifdef YYSTACK_ALLOC
#  define YYSTACK_FREE(Ptr) do {  ; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM 4032
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h>
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T);
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *);
#   endif
#  endif
# endif
#endif
#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)
# define YYCOPY_NEEDED 1
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)
#endif
#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif
#define YYFINAL  42
#define YYLAST   242
#define YYNTOKENS  51
#define YYNNTS  68
#define YYNRULES  121
#define YYNSTATES  207
#define YYMAXUTOK   305
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50
};
#if YYDEBUG
static const yytype_int16 yyrline[] =
{
       0,   146,   146,   148,   159,   161,   165,   167,   169,   169,
     169,   174,   177,   177,   179,   183,   186,   189,   192,   195,
     198,   200,   202,   202,   204,   204,   206,   206,   208,   208,
     208,   210,   210,   212,   214,   216,   216,   216,   218,   218,
     218,   220,   220,   220,   222,   222,   222,   224,   224,   224,
     226,   226,   226,   228,   228,   228,   228,   230,   233,   235,
     232,   244,   246,   248,   250,   257,   259,   259,   261,   261,
     263,   263,   265,   265,   267,   267,   269,   269,   271,   271,
     273,   273,   275,   275,   277,   277,   279,   279,   281,   281,
     283,   283,   295,   296,   300,   300,   300,   309,   311,   321,
     326,   327,   331,   333,   333,   342,   342,   344,   344,   346,
     346,   357,   358,   362,   364,   366,   368,   370,   372,   382,
     383,   383
};
#endif
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])
#if YYDEBUG || 0
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "_BANG_t",
  "_BANG_EQUALS_t", "_AMPER_t", "_AMPERAMPER_t", "_LPAREN_t", "_RPAREN_t",
  "_PLUS_EQUALS_t", "_COLON_t", "_SEMIC_t", "_LANGLE_t",
  "_LANGLE_EQUALS_t", "_EQUALS_t", "_RANGLE_t", "_RANGLE_EQUALS_t",
  "_QUESTION_EQUALS_t", "_LBRACKET_t", "_RBRACKET_t", "ACTIONS_t",
  "BIND_t", "BREAK_t", "CASE_t", "CLASS_t", "CONTINUE_t", "DEFAULT_t",
  "ELSE_t", "EXISTING_t", "FOR_t", "IF_t", "IGNORE_t", "IN_t", "INCLUDE_t",
  "LOCAL_t", "MODULE_t", "ON_t", "PIECEMEAL_t", "QUIETLY_t", "RETURN_t",
  "RULE_t", "SWITCH_t", "TOGETHER_t", "UPDATED_t", "WHILE_t", "_LBRACE_t",
  "_BAR_t", "_BARBAR_t", "_RBRACE_t", "ARG", "STRING", "$accept", "run",
  "block", "rules", "$@1", "$@2", "null", "assign_list_opt", "$@3",
  "arglist_opt", "local_opt", "else_opt", "rule", "$@4", "$@5", "$@6",
  "$@7", "$@8", "$@9", "$@10", "$@11", "$@12", "$@13", "$@14", "$@15",
  "$@16", "$@17", "$@18", "$@19", "$@20", "$@21", "$@22", "$@23", "$@24",
  "$@25", "$@26", "assign", "expr", "$@27", "$@28", "$@29", "$@30", "$@31",
  "$@32", "$@33", "$@34", "$@35", "$@36", "$@37", "$@38", "$@39", "cases",
  "case", "$@40", "$@41", "lol", "list", "listp", "arg", "@42", "func",
  "$@43", "$@44", "$@45", "eflags", "eflag", "bindlist", "$@46", YY_NULLPTR
};
static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif
#define YYPACT_NINF (-119)
#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)
#define YYTABLE_NINF (-25)
#define yytable_value_is_error(Yyn) \
  0
static const yytype_int16 yypact[] =
{
     140,  -119,  -119,     1,  -119,     2,   -18,  -119,  -119,   -23,
    -119,    -9,  -119,  -119,  -119,   140,    12,    31,  -119,     4,
     140,    77,   -17,   186,  -119,  -119,  -119,  -119,    -7,     3,
    -119,  -119,  -119,  -119,   177,  -119,  -119,     3,    -5,  -119,
    -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,    33,  -119,
    -119,    -9,  -119,    29,  -119,  -119,  -119,  -119,  -119,  -119,
      35,  -119,    14,    50,    -9,    34,  -119,  -119,    23,    39,
      52,    53,    40,  -119,    66,    45,    94,  -119,    67,    30,
    -119,  -119,  -119,    16,  -119,  -119,  -119,    47,  -119,  -119,
    -119,  -119,     3,     3,  -119,  -119,  -119,  -119,  -119,  -119,
    -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,    84,
    -119,  -119,  -119,    51,  -119,  -119,    32,   105,  -119,  -119,
    -119,  -119,  -119,   140,  -119,  -119,  -119,    68,     3,     3,
       3,     3,     3,     3,     3,     3,   140,     3,     3,  -119,
    -119,  -119,   140,    95,   140,   110,  -119,  -119,  -119,  -119,
    -119,    69,    73,    87,  -119,    89,   139,   139,  -119,  -119,
      89,  -119,  -119,    90,   226,   226,  -119,  -119,   140,    91,
    -119,    97,    95,    98,  -119,  -119,  -119,  -119,  -119,  -119,
    -119,  -119,   108,  -119,  -119,    88,  -119,  -119,  -119,   141,
     177,   145,   102,   140,   177,  -119,   149,  -119,  -119,  -119,
    -119,   115,  -119,  -119,  -119,   140,  -119
};
static const yytype_int8 yydefact[] =
{
       2,   103,   111,     0,    47,     0,    18,    41,    22,     8,
      44,     0,    31,    38,    50,    11,   102,     0,     3,     0,
       6,     0,     0,     0,    33,   100,    34,    17,     0,     0,
     100,   100,   100,   102,    18,   100,   100,     0,     0,     5,
       4,   100,     1,    53,     7,    62,    61,    63,     0,    28,
      26,     0,   105,     0,   118,   115,   117,   116,   114,   113,
     119,   112,     0,    97,    99,     0,    88,    90,     0,    65,
       0,    11,     0,    57,     0,     0,    51,    21,     0,     0,
      64,   100,   100,     0,   100,   104,   120,     0,    48,   100,
     101,    35,     0,     0,    68,    78,    80,    70,    72,    66,
      74,    76,    42,    82,    84,    86,    23,    12,    14,     0,
      45,    32,    39,     0,    25,    54,     0,     0,   109,   107,
     106,   100,    58,    11,    98,   100,    89,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    11,     0,     0,   100,
     100,     9,    11,    92,    11,    16,    29,    27,   100,   100,
     121,     0,     0,     0,    91,    69,    79,    81,    71,    73,
      67,    75,    77,     0,    83,    85,    87,    13,    11,     0,
      94,     0,    92,     0,   100,    55,   100,   110,   108,    59,
      49,    36,    20,    10,    46,     0,    40,    93,    52,     0,
      18,     0,     0,    11,    18,    43,     0,    15,    56,    30,
      60,     0,    19,    95,    37,    11,    96
};
static const yytype_int16 yypgoto[] =
{
    -119,  -119,  -118,    25,  -119,  -119,    96,  -119,  -119,  -119,
     160,  -119,   -33,  -119,  -119,  -119,  -119,  -119,  -119,  -119,
    -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,
    -119,  -119,  -119,  -119,  -119,  -119,    55,    -4,  -119,  -119,
    -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,
    -119,     5,  -119,  -119,  -119,   -27,   -28,  -119,     0,  -119,
    -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119
};
static const yytype_uint8 yydefgoto[] =
{
       0,    17,    38,    39,    31,   168,    40,   109,   140,   175,
      19,   195,    20,    30,    41,    82,    81,   176,    35,   125,
     193,    36,   143,    29,   136,    32,   142,    25,   123,    37,
     113,    79,   145,   190,   151,   192,    50,    68,   133,   128,
     131,   132,   134,   135,   129,   130,   137,   138,   139,    92,
      93,   171,   172,   185,   205,    62,    63,    64,    69,    22,
      53,    84,   149,   148,    23,    61,    87,   121
};
static const yytype_int16 yytable[] =
{
      21,    73,    70,    71,    72,   152,    66,    74,    75,     1,
      67,    34,    24,    26,    78,    21,    27,   -17,   163,    51,
      21,     1,   -24,   -24,   169,    18,   173,    94,    95,    96,
     -24,    42,    52,    76,    21,    97,    98,    99,   100,   101,
      33,    45,    65,    77,    43,    44,    46,    80,    85,    47,
     183,    83,    33,   116,   117,   118,    86,   120,    48,    88,
      89,   -24,   124,   106,    90,   119,    91,   107,   102,   103,
     104,   105,    94,    95,    96,   201,   154,   111,   114,   115,
      97,    98,    99,   100,   101,   110,    45,   206,   126,   127,
     112,    46,   122,   150,    47,   141,   144,   153,    94,    95,
      96,    97,    98,    48,   100,   101,    97,    98,    99,   100,
     101,   166,   167,    49,   103,   104,   147,   174,   170,   179,
     177,   180,   178,    21,   155,   156,   157,   158,   159,   160,
     161,   162,   181,   164,   165,   194,    21,   196,   182,   184,
     103,   104,    21,    94,    21,   186,   188,   189,   191,   197,
     200,    97,    98,    99,   100,   101,   199,   198,     1,   203,
       2,   202,     3,   204,     4,     5,    28,   108,    21,     6,
       7,   146,     0,     8,     9,    10,    11,   187,     0,    12,
     -18,    13,     0,     0,    14,    15,     0,     0,     0,    16,
      21,     0,     0,    21,    21,     1,     0,     2,     0,     3,
       0,     4,     5,     0,     0,    21,     6,     7,     0,     0,
       8,    27,    10,    11,    54,     0,    12,    55,    13,     0,
       0,    14,    15,    56,    57,     0,    16,     0,    58,    59,
      94,    95,    96,     0,     0,    60,     0,     0,    97,    98,
      99,   100,   101
};
static const yytype_int16 yycheck[] =
{
       0,    34,    30,    31,    32,   123,     3,    35,    36,    18,
       7,    11,    11,    11,    41,    15,    34,    40,   136,    36,
      20,    18,    10,    11,   142,     0,   144,     4,     5,     6,
      18,     0,    49,    37,    34,    12,    13,    14,    15,    16,
      49,     9,    49,    48,    40,    20,    14,    14,    19,    17,
     168,    51,    49,    81,    82,    39,    21,    84,    26,    45,
      10,    49,    89,    11,    64,    49,    32,    14,    45,    46,
      47,    32,     4,     5,     6,   193,     8,    11,    11,    49,
      12,    13,    14,    15,    16,    45,     9,   205,    92,    93,
      45,    14,    45,   121,    17,    11,    45,   125,     4,     5,
       6,    12,    13,    26,    15,    16,    12,    13,    14,    15,
      16,   139,   140,    36,    46,    47,    11,     7,    23,    50,
     148,    48,   149,   123,   128,   129,   130,   131,   132,   133,
     134,   135,    45,   137,   138,    27,   136,    49,    48,    48,
      46,    47,   142,     4,   144,    48,    48,   174,   176,     8,
      48,    12,    13,    14,    15,    16,    11,   190,    18,    10,
      20,   194,    22,    48,    24,    25,     6,    71,   168,    29,
      30,   116,    -1,    33,    34,    35,    36,   172,    -1,    39,
      40,    41,    -1,    -1,    44,    45,    -1,    -1,    -1,    49,
     190,    -1,    -1,   193,   194,    18,    -1,    20,    -1,    22,
      -1,    24,    25,    -1,    -1,   205,    29,    30,    -1,    -1,
      33,    34,    35,    36,    28,    -1,    39,    31,    41,    -1,
      -1,    44,    45,    37,    38,    -1,    49,    -1,    42,    43,
       4,     5,     6,    -1,    -1,    49,    -1,    -1,    12,    13,
      14,    15,    16
};
static const yytype_int8 yystos[] =
{
       0,    18,    20,    22,    24,    25,    29,    30,    33,    34,
      35,    36,    39,    41,    44,    45,    49,    52,    54,    61,
      63,   109,   110,   115,    11,    78,    11,    34,    61,    74,
      64,    55,    76,    49,   109,    69,    72,    80,    53,    54,
      57,    65,     0,    40,    54,     9,    14,    17,    26,    36,
      87,    36,    49,   111,    28,    31,    37,    38,    42,    43,
      49,   116,   106,   107,   108,    49,     3,     7,    88,   109,
     107,   107,   107,    63,   107,   107,    88,    48,   106,    82,
      14,    67,    66,   109,   112,    19,    21,   117,    45,    10,
     109,    32,   100,   101,     4,     5,     6,    12,    13,    14,
      15,    16,    45,    46,    47,    32,    11,    14,    57,    58,
      45,    11,    45,    81,    11,    49,   107,   107,    39,    49,
     106,   118,    45,    79,   106,    70,    88,    88,    90,    95,
      96,    91,    92,    89,    93,    94,    75,    97,    98,    99,
      59,    11,    77,    73,    45,    83,    87,    11,   114,   113,
     107,    85,    53,   107,     8,    88,    88,    88,    88,    88,
      88,    88,    88,    53,    88,    88,   107,   107,    56,    53,
      23,   102,   103,    53,     7,    60,    68,   107,   106,    50,
      48,    45,    48,    53,    48,   104,    48,   102,    48,   106,
      84,   107,    86,    71,    27,    62,    49,     8,    63,    11,
      48,    53,    63,    10,    48,   105,    53
};
static const yytype_int8 yyr1[] =
{
       0,    51,    52,    52,    53,    53,    54,    54,    55,    56,
      54,    57,    59,    58,    58,    60,    60,    61,    61,    62,
      62,    63,    64,    63,    65,    63,    66,    63,    67,    68,
      63,    69,    63,    63,    63,    70,    71,    63,    72,    73,
      63,    74,    75,    63,    76,    77,    63,    78,    79,    63,
      80,    81,    63,    82,    83,    84,    63,    63,    85,    86,
      63,    87,    87,    87,    87,    88,    89,    88,    90,    88,
      91,    88,    92,    88,    93,    88,    94,    88,    95,    88,
      96,    88,    97,    88,    98,    88,    99,    88,   100,    88,
     101,    88,   102,   102,   104,   105,   103,   106,   106,   107,
     108,   108,   109,   110,   109,   112,   111,   113,   111,   114,
     111,   115,   115,   116,   116,   116,   116,   116,   116,   117,
     118,   117
};
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     1,     1,     1,     1,     2,     0,     0,
       7,     0,     0,     3,     1,     3,     0,     1,     0,     2,
       0,     3,     0,     4,     0,     4,     0,     5,     0,     0,
       8,     0,     4,     2,     2,     0,     0,    10,     0,     0,
       7,     0,     0,     8,     0,     0,     7,     0,     0,     7,
       0,     0,     7,     0,     0,     0,     8,     3,     0,     0,
       9,     1,     1,     1,     2,     1,     0,     4,     0,     4,
       0,     4,     0,     4,     0,     4,     0,     4,     0,     4,
       0,     4,     0,     4,     0,     4,     0,     4,     0,     3,
       0,     4,     0,     2,     0,     0,     6,     1,     3,     1,
       0,     2,     1,     0,     4,     0,     3,     0,     5,     0,
       5,     0,     2,     1,     1,     1,     1,     1,     1,     0,
       0,     3
};
enum { YYENOMEM = -2 };
#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)
#define YYERRCODE YYUNDEF
#if YYDEBUG
# ifndef YYFPRINTF
#  include <stdio.h>
#  define YYFPRINTF fprintf
# endif
# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)
static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}
static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));
  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}
static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}
# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)
static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}
# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)
int yydebug;
#else
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif
#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif
static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}
int yychar;
YYSTYPE yylval;
int yynerrs;
int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    int yyerrstatus = 0;
    YYPTRDIFF_T yystacksize = YYINITDEPTH;
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;
  int yyn;
  int yyresult;
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  YYSTYPE yyval;
#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))
  int yylen = 0;
  YYDPRINTF ((stderr, "Starting parse\n"));
  yychar = YYEMPTY;
  goto yysetstate;
yynewstate:
  yyssp++;
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);
  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      YYPTRDIFF_T yysize = yyssp - yyss + 1;
# if defined yyoverflow
      {
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;
      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END
      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif
  if (yystate == YYFINAL)
    YYACCEPT;
  goto yybackup;
yybackup:
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }
  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  if (yyerrstatus)
    yyerrstatus--;
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  yychar = YYEMPTY;
  goto yynewstate;
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;
yyreduce:
  yylen = yyr2[yyn];
  yyval = yyvsp[1-yylen];
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 3:
#line 149 "src/engine/jamgram.y"
                { parse_save( yyvsp[0].parse ); }
#line 1384 "src/engine/jamgram.cpp"
    break;
  case 4:
#line 160 "src/engine/jamgram.y"
                { yyval.parse = yyvsp[0].parse; }
#line 1390 "src/engine/jamgram.cpp"
    break;
  case 5:
#line 162 "src/engine/jamgram.y"
                { yyval.parse = yyvsp[0].parse; }
#line 1396 "src/engine/jamgram.cpp"
    break;
  case 6:
#line 166 "src/engine/jamgram.y"
                { yyval.parse = yyvsp[0].parse; }
#line 1402 "src/engine/jamgram.cpp"
    break;
  case 7:
#line 168 "src/engine/jamgram.y"
                { yyval.parse = prules( yyvsp[-1].parse, yyvsp[0].parse ); }
#line 1408 "src/engine/jamgram.cpp"
    break;
  case 8:
#line 169 "src/engine/jamgram.y"
                  { yymode( SCAN_ASSIGN ); }
#line 1414 "src/engine/jamgram.cpp"
    break;
  case 9:
#line 169 "src/engine/jamgram.y"
                                                                           { yymode( SCAN_NORMAL ); }
#line 1420 "src/engine/jamgram.cpp"
    break;
  case 10:
#line 170 "src/engine/jamgram.y"
                { yyval.parse = plocal( yyvsp[-4].parse, yyvsp[-3].parse, yyvsp[0].parse ); }
#line 1426 "src/engine/jamgram.cpp"
    break;
  case 11:
#line 174 "src/engine/jamgram.y"
        { yyval.parse = pnull(); }
#line 1432 "src/engine/jamgram.cpp"
    break;
  case 12:
#line 177 "src/engine/jamgram.y"
                            { yymode( SCAN_PUNCT ); }
#line 1438 "src/engine/jamgram.cpp"
    break;
  case 13:
#line 178 "src/engine/jamgram.y"
                { yyval.parse = yyvsp[0].parse; yyval.number = ASSIGN_SET; }
#line 1444 "src/engine/jamgram.cpp"
    break;
  case 14:
#line 180 "src/engine/jamgram.y"
                { yyval.parse = yyvsp[0].parse; yyval.number = ASSIGN_APPEND; }
#line 1450 "src/engine/jamgram.cpp"
    break;
  case 15:
#line 184 "src/engine/jamgram.y"
                { yyval.parse = yyvsp[-1].parse; }
#line 1456 "src/engine/jamgram.cpp"
    break;
  case 16:
#line 186 "src/engine/jamgram.y"
                { yyval.parse = P0; }
#line 1462 "src/engine/jamgram.cpp"
    break;
  case 17:
#line 190 "src/engine/jamgram.y"
                { yyval.number = 1; }
#line 1468 "src/engine/jamgram.cpp"
    break;
  case 18:
#line 192 "src/engine/jamgram.y"
                { yyval.number = 0; }
#line 1474 "src/engine/jamgram.cpp"
    break;
  case 19:
#line 196 "src/engine/jamgram.y"
                { yyval.parse = yyvsp[0].parse; }
#line 1480 "src/engine/jamgram.cpp"
    break;
  case 20:
#line 198 "src/engine/jamgram.y"
                { yyval.parse = pnull(); }
#line 1486 "src/engine/jamgram.cpp"
    break;
  case 21:
#line 201 "src/engine/jamgram.y"
                { yyval.parse = yyvsp[-1].parse; }
#line 1492 "src/engine/jamgram.cpp"
    break;
  case 22:
#line 202 "src/engine/jamgram.y"
                    { yymode( SCAN_PUNCT ); }
#line 1498 "src/engine/jamgram.cpp"
    break;
  case 23:
#line 203 "src/engine/jamgram.y"
                { yyval.parse = pincl( yyvsp[-1].parse ); yymode( SCAN_NORMAL ); }
#line 1504 "src/engine/jamgram.cpp"
    break;
  case 24:
#line 204 "src/engine/jamgram.y"
              { yymode( SCAN_PUNCT ); }
#line 1510 "src/engine/jamgram.cpp"
    break;
  case 25:
#line 205 "src/engine/jamgram.y"
                { yyval.parse = prule( yyvsp[-3].string, yyvsp[-1].parse ); yymode( SCAN_NORMAL ); }
#line 1516 "src/engine/jamgram.cpp"
    break;
  case 26:
#line 206 "src/engine/jamgram.y"
                     { yymode( SCAN_PUNCT ); }
#line 1522 "src/engine/jamgram.cpp"
    break;
  case 27:
#line 207 "src/engine/jamgram.y"
                { yyval.parse = pset( yyvsp[-4].parse, yyvsp[-1].parse, yyvsp[-3].number ); yymode( SCAN_NORMAL ); }
#line 1528 "src/engine/jamgram.cpp"
    break;
  case 28:
#line 208 "src/engine/jamgram.y"
                   { yymode( SCAN_ASSIGN ); }
#line 1534 "src/engine/jamgram.cpp"
    break;
  case 29:
#line 208 "src/engine/jamgram.y"
                                                          { yymode( SCAN_PUNCT ); }
#line 1540 "src/engine/jamgram.cpp"
    break;
  case 30:
#line 209 "src/engine/jamgram.y"
                { yyval.parse = pset1( yyvsp[-7].parse, yyvsp[-4].parse, yyvsp[-1].parse, yyvsp[-3].number ); yymode( SCAN_NORMAL ); }
#line 1546 "src/engine/jamgram.cpp"
    break;
  case 31:
#line 210 "src/engine/jamgram.y"
                   { yymode( SCAN_PUNCT ); }
#line 1552 "src/engine/jamgram.cpp"
    break;
  case 32:
#line 211 "src/engine/jamgram.y"
                { yyval.parse = preturn( yyvsp[-1].parse ); yymode( SCAN_NORMAL ); }
#line 1558 "src/engine/jamgram.cpp"
    break;
  case 33:
#line 213 "src/engine/jamgram.y"
        { yyval.parse = pbreak(); }
#line 1564 "src/engine/jamgram.cpp"
    break;
  case 34:
#line 215 "src/engine/jamgram.y"
        { yyval.parse = pcontinue(); }
#line 1570 "src/engine/jamgram.cpp"
    break;
  case 35:
#line 216 "src/engine/jamgram.y"
                                   { yymode( SCAN_PUNCT ); }
#line 1576 "src/engine/jamgram.cpp"
    break;
  case 36:
#line 216 "src/engine/jamgram.y"
                                                                            { yymode( SCAN_NORMAL ); }
#line 1582 "src/engine/jamgram.cpp"
    break;
  case 37:
#line 217 "src/engine/jamgram.y"
                { yyval.parse = pfor( yyvsp[-7].string, yyvsp[-4].parse, yyvsp[-1].parse, yyvsp[-8].number ); }
#line 1588 "src/engine/jamgram.cpp"
    break;
  case 38:
#line 218 "src/engine/jamgram.y"
                   { yymode( SCAN_PUNCT ); }
#line 1594 "src/engine/jamgram.cpp"
    break;
  case 39:
#line 218 "src/engine/jamgram.y"
                                                            { yymode( SCAN_NORMAL ); }
#line 1600 "src/engine/jamgram.cpp"
    break;
  case 40:
#line 219 "src/engine/jamgram.y"
                { yyval.parse = pswitch( yyvsp[-4].parse, yyvsp[-1].parse ); }
#line 1606 "src/engine/jamgram.cpp"
    break;
  case 41:
#line 220 "src/engine/jamgram.y"
               { yymode( SCAN_CONDB ); }
#line 1612 "src/engine/jamgram.cpp"
    break;
  case 42:
#line 220 "src/engine/jamgram.y"
                                                        { yymode( SCAN_NORMAL ); }
#line 1618 "src/engine/jamgram.cpp"
    break;
  case 43:
#line 221 "src/engine/jamgram.y"
                { yyval.parse = pif( yyvsp[-5].parse, yyvsp[-2].parse, yyvsp[0].parse ); }
#line 1624 "src/engine/jamgram.cpp"
    break;
  case 44:
#line 222 "src/engine/jamgram.y"
                   { yymode( SCAN_PUNCT ); }
#line 1630 "src/engine/jamgram.cpp"
    break;
  case 45:
#line 222 "src/engine/jamgram.y"
                                                            { yymode( SCAN_NORMAL ); }
#line 1636 "src/engine/jamgram.cpp"
    break;
  case 46:
#line 223 "src/engine/jamgram.y"
                { yyval.parse = pmodule( yyvsp[-4].parse, yyvsp[-1].parse ); }
#line 1642 "src/engine/jamgram.cpp"
    break;
  case 47:
#line 224 "src/engine/jamgram.y"
                  { yymode( SCAN_PUNCT ); }
#line 1648 "src/engine/jamgram.cpp"
    break;
  case 48:
#line 224 "src/engine/jamgram.y"
                                                          { yymode( SCAN_NORMAL ); }
#line 1654 "src/engine/jamgram.cpp"
    break;
  case 49:
#line 225 "src/engine/jamgram.y"
                { yyval.parse = pclass( yyvsp[-4].parse, yyvsp[-1].parse ); }
#line 1660 "src/engine/jamgram.cpp"
    break;
  case 50:
#line 226 "src/engine/jamgram.y"
                  { yymode( SCAN_CONDB ); }
#line 1666 "src/engine/jamgram.cpp"
    break;
  case 51:
#line 226 "src/engine/jamgram.y"
                                                 { yymode( SCAN_NORMAL ); }
#line 1672 "src/engine/jamgram.cpp"
    break;
  case 52:
#line 227 "src/engine/jamgram.y"
                { yyval.parse = pwhile( yyvsp[-4].parse, yyvsp[-1].parse ); }
#line 1678 "src/engine/jamgram.cpp"
    break;
  case 53:
#line 228 "src/engine/jamgram.y"
                        { yymode( SCAN_PUNCT ); }
#line 1684 "src/engine/jamgram.cpp"
    break;
  case 54:
#line 228 "src/engine/jamgram.y"
                                                      { yymode( SCAN_PARAMS ); }
#line 1690 "src/engine/jamgram.cpp"
    break;
  case 55:
#line 228 "src/engine/jamgram.y"
                                                                                             { yymode( SCAN_NORMAL ); }
#line 1696 "src/engine/jamgram.cpp"
    break;
  case 56:
#line 229 "src/engine/jamgram.y"
                { yyval.parse = psetc( yyvsp[-4].string, yyvsp[0].parse, yyvsp[-2].parse, yyvsp[-7].number ); }
#line 1702 "src/engine/jamgram.cpp"
    break;
  case 57:
#line 231 "src/engine/jamgram.y"
                { yyval.parse = pon( yyvsp[-1].parse, yyvsp[0].parse ); }
#line 1708 "src/engine/jamgram.cpp"
    break;
  case 58:
#line 233 "src/engine/jamgram.y"
                { yymode( SCAN_STRING ); }
#line 1714 "src/engine/jamgram.cpp"
    break;
  case 59:
#line 235 "src/engine/jamgram.y"
                { yymode( SCAN_NORMAL ); }
#line 1720 "src/engine/jamgram.cpp"
    break;
  case 60:
#line 237 "src/engine/jamgram.y"
                { yyval.parse = psete( yyvsp[-6].string,yyvsp[-5].parse,yyvsp[-2].string,yyvsp[-7].number ); }
#line 1726 "src/engine/jamgram.cpp"
    break;
  case 61:
#line 245 "src/engine/jamgram.y"
                { yyval.number = ASSIGN_SET; }
#line 1732 "src/engine/jamgram.cpp"
    break;
  case 62:
#line 247 "src/engine/jamgram.y"
                { yyval.number = ASSIGN_APPEND; }
#line 1738 "src/engine/jamgram.cpp"
    break;
  case 63:
#line 249 "src/engine/jamgram.y"
                { yyval.number = ASSIGN_DEFAULT; }
#line 1744 "src/engine/jamgram.cpp"
    break;
  case 64:
#line 251 "src/engine/jamgram.y"
                { yyval.number = ASSIGN_DEFAULT; }
#line 1750 "src/engine/jamgram.cpp"
    break;
  case 65:
#line 258 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_EXISTS, yyvsp[0].parse, pnull() ); yymode( SCAN_COND ); }
#line 1756 "src/engine/jamgram.cpp"
    break;
  case 66:
#line 259 "src/engine/jamgram.y"
                         { yymode( SCAN_CONDB ); }
#line 1762 "src/engine/jamgram.cpp"
    break;
  case 67:
#line 260 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_EQUALS, yyvsp[-3].parse, yyvsp[0].parse ); }
#line 1768 "src/engine/jamgram.cpp"
    break;
  case 68:
#line 261 "src/engine/jamgram.y"
                              { yymode( SCAN_CONDB ); }
#line 1774 "src/engine/jamgram.cpp"
    break;
  case 69:
#line 262 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_NOTEQ, yyvsp[-3].parse, yyvsp[0].parse ); }
#line 1780 "src/engine/jamgram.cpp"
    break;
  case 70:
#line 263 "src/engine/jamgram.y"
                         { yymode( SCAN_CONDB ); }
#line 1786 "src/engine/jamgram.cpp"
    break;
  case 71:
#line 264 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_LESS, yyvsp[-3].parse, yyvsp[0].parse ); }
#line 1792 "src/engine/jamgram.cpp"
    break;
  case 72:
#line 265 "src/engine/jamgram.y"
                                { yymode( SCAN_CONDB ); }
#line 1798 "src/engine/jamgram.cpp"
    break;
  case 73:
#line 266 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_LESSEQ, yyvsp[-3].parse, yyvsp[0].parse ); }
#line 1804 "src/engine/jamgram.cpp"
    break;
  case 74:
#line 267 "src/engine/jamgram.y"
                         { yymode( SCAN_CONDB ); }
#line 1810 "src/engine/jamgram.cpp"
    break;
  case 75:
#line 268 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_MORE, yyvsp[-3].parse, yyvsp[0].parse ); }
#line 1816 "src/engine/jamgram.cpp"
    break;
  case 76:
#line 269 "src/engine/jamgram.y"
                                { yymode( SCAN_CONDB ); }
#line 1822 "src/engine/jamgram.cpp"
    break;
  case 77:
#line 270 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_MOREEQ, yyvsp[-3].parse, yyvsp[0].parse ); }
#line 1828 "src/engine/jamgram.cpp"
    break;
  case 78:
#line 271 "src/engine/jamgram.y"
                        { yymode( SCAN_CONDB ); }
#line 1834 "src/engine/jamgram.cpp"
    break;
  case 79:
#line 272 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_AND, yyvsp[-3].parse, yyvsp[0].parse ); }
#line 1840 "src/engine/jamgram.cpp"
    break;
  case 80:
#line 273 "src/engine/jamgram.y"
                             { yymode( SCAN_CONDB ); }
#line 1846 "src/engine/jamgram.cpp"
    break;
  case 81:
#line 274 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_AND, yyvsp[-3].parse, yyvsp[0].parse ); }
#line 1852 "src/engine/jamgram.cpp"
    break;
  case 82:
#line 275 "src/engine/jamgram.y"
                      { yymode( SCAN_CONDB ); }
#line 1858 "src/engine/jamgram.cpp"
    break;
  case 83:
#line 276 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_OR, yyvsp[-3].parse, yyvsp[0].parse ); }
#line 1864 "src/engine/jamgram.cpp"
    break;
  case 84:
#line 277 "src/engine/jamgram.y"
                         { yymode( SCAN_CONDB ); }
#line 1870 "src/engine/jamgram.cpp"
    break;
  case 85:
#line 278 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_OR, yyvsp[-3].parse, yyvsp[0].parse ); }
#line 1876 "src/engine/jamgram.cpp"
    break;
  case 86:
#line 279 "src/engine/jamgram.y"
                   { yymode( SCAN_PUNCT ); }
#line 1882 "src/engine/jamgram.cpp"
    break;
  case 87:
#line 280 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_IN, yyvsp[-3].parse, yyvsp[0].parse ); yymode( SCAN_COND ); }
#line 1888 "src/engine/jamgram.cpp"
    break;
  case 88:
#line 281 "src/engine/jamgram.y"
                  { yymode( SCAN_CONDB ); }
#line 1894 "src/engine/jamgram.cpp"
    break;
  case 89:
#line 282 "src/engine/jamgram.y"
                { yyval.parse = peval( EXPR_NOT, yyvsp[0].parse, pnull() ); }
#line 1900 "src/engine/jamgram.cpp"
    break;
  case 90:
#line 283 "src/engine/jamgram.y"
                    { yymode( SCAN_CONDB ); }
#line 1906 "src/engine/jamgram.cpp"
    break;
  case 91:
#line 284 "src/engine/jamgram.y"
                { yyval.parse = yyvsp[-1].parse; }
#line 1912 "src/engine/jamgram.cpp"
    break;
  case 92:
#line 295 "src/engine/jamgram.y"
                { yyval.parse = P0; }
#line 1918 "src/engine/jamgram.cpp"
    break;
  case 93:
#line 297 "src/engine/jamgram.y"
                { yyval.parse = pnode( yyvsp[-1].parse, yyvsp[0].parse ); }
#line 1924 "src/engine/jamgram.cpp"
    break;
  case 94:
#line 300 "src/engine/jamgram.y"
                 { yymode( SCAN_CASE ); }
#line 1930 "src/engine/jamgram.cpp"
    break;
  case 95:
#line 300 "src/engine/jamgram.y"
                                                       { yymode( SCAN_NORMAL ); }
#line 1936 "src/engine/jamgram.cpp"
    break;
  case 96:
#line 301 "src/engine/jamgram.y"
                { yyval.parse = psnode( yyvsp[-3].string, yyvsp[0].parse ); }
#line 1942 "src/engine/jamgram.cpp"
    break;
  case 97:
#line 310 "src/engine/jamgram.y"
                { yyval.parse = pnode( P0, yyvsp[0].parse ); }
#line 1948 "src/engine/jamgram.cpp"
    break;
  case 98:
#line 312 "src/engine/jamgram.y"
                { yyval.parse = pnode( yyvsp[0].parse, yyvsp[-2].parse ); }
#line 1954 "src/engine/jamgram.cpp"
    break;
  case 99:
#line 322 "src/engine/jamgram.y"
                { yyval.parse = yyvsp[0].parse; }
#line 1960 "src/engine/jamgram.cpp"
    break;
  case 100:
#line 326 "src/engine/jamgram.y"
                { yyval.parse = pnull(); }
#line 1966 "src/engine/jamgram.cpp"
    break;
  case 101:
#line 328 "src/engine/jamgram.y"
                { yyval.parse = pappend( yyvsp[-1].parse, yyvsp[0].parse ); }
#line 1972 "src/engine/jamgram.cpp"
    break;
  case 102:
#line 332 "src/engine/jamgram.y"
                { yyval.parse = plist( yyvsp[0].string ); }
#line 1978 "src/engine/jamgram.cpp"
    break;
  case 103:
#line 333 "src/engine/jamgram.y"
                      { yyval.number = yymode( SCAN_CALL ); }
#line 1984 "src/engine/jamgram.cpp"
    break;
  case 104:
#line 334 "src/engine/jamgram.y"
                { yyval.parse = yyvsp[-1].parse; yymode( yyvsp[-2].number ); }
#line 1990 "src/engine/jamgram.cpp"
    break;
  case 105:
#line 342 "src/engine/jamgram.y"
              { yymode( SCAN_PUNCT ); }
#line 1996 "src/engine/jamgram.cpp"
    break;
  case 106:
#line 343 "src/engine/jamgram.y"
                { yyval.parse = prule( yyvsp[-2].string, yyvsp[0].parse ); }
#line 2002 "src/engine/jamgram.cpp"
    break;
  case 107:
#line 344 "src/engine/jamgram.y"
                       { yymode( SCAN_PUNCT ); }
#line 2008 "src/engine/jamgram.cpp"
    break;
  case 108:
#line 345 "src/engine/jamgram.y"
                { yyval.parse = pon( yyvsp[-3].parse, prule( yyvsp[-2].string, yyvsp[0].parse ) ); }
#line 2014 "src/engine/jamgram.cpp"
    break;
  case 109:
#line 346 "src/engine/jamgram.y"
                            { yymode( SCAN_PUNCT ); }
#line 2020 "src/engine/jamgram.cpp"
    break;
  case 110:
#line 347 "src/engine/jamgram.y"
                { yyval.parse = pon( yyvsp[-3].parse, yyvsp[0].parse ); }
#line 2026 "src/engine/jamgram.cpp"
    break;
  case 111:
#line 357 "src/engine/jamgram.y"
                { yyval.number = 0; }
#line 2032 "src/engine/jamgram.cpp"
    break;
  case 112:
#line 359 "src/engine/jamgram.y"
                { yyval.number = yyvsp[-1].number | yyvsp[0].number; }
#line 2038 "src/engine/jamgram.cpp"
    break;
  case 113:
#line 363 "src/engine/jamgram.y"
                { yyval.number = EXEC_UPDATED; }
#line 2044 "src/engine/jamgram.cpp"
    break;
  case 114:
#line 365 "src/engine/jamgram.y"
                { yyval.number = EXEC_TOGETHER; }
#line 2050 "src/engine/jamgram.cpp"
    break;
  case 115:
#line 367 "src/engine/jamgram.y"
                { yyval.number = EXEC_IGNORE; }
#line 2056 "src/engine/jamgram.cpp"
    break;
  case 116:
#line 369 "src/engine/jamgram.y"
                { yyval.number = EXEC_QUIETLY; }
#line 2062 "src/engine/jamgram.cpp"
    break;
  case 117:
#line 371 "src/engine/jamgram.y"
                { yyval.number = EXEC_PIECEMEAL; }
#line 2068 "src/engine/jamgram.cpp"
    break;
  case 118:
#line 373 "src/engine/jamgram.y"
                { yyval.number = EXEC_EXISTING; }
#line 2074 "src/engine/jamgram.cpp"
    break;
  case 119:
#line 382 "src/engine/jamgram.y"
                { yyval.parse = pnull(); }
#line 2080 "src/engine/jamgram.cpp"
    break;
  case 120:
#line 383 "src/engine/jamgram.y"
                 { yymode( SCAN_PUNCT ); }
#line 2086 "src/engine/jamgram.cpp"
    break;
  case 121:
#line 384 "src/engine/jamgram.y"
                { yyval.parse = yyvsp[0].parse; }
#line 2092 "src/engine/jamgram.cpp"
    break;
#line 2096 "src/engine/jamgram.cpp"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);
  YYPOPSTACK (yylen);
  yylen = 0;
  *++yyvsp = yyval;
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }
  goto yynewstate;
yyerrlab:
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }
  if (yyerrstatus == 3)
    {
      if (yychar <= YYEOF)
        {
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }
  goto yyerrlab1;
yyerrorlab:
  if (0)
    YYERROR;
  ++yynerrs;
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;
yyerrlab1:
  yyerrstatus = 3;
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }
      if (yyssp == yyss)
        YYABORT;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);
  yystate = yyn;
  goto yynewstate;
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}
