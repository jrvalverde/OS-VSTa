#define YYBYACC 1
#line 8 "expr.y"
#ifdef YYDEBUG
int yydebug = 1;
#endif
#line 10 "y.tab.c"
#define MINUS 257
#define LE 258
#define GE 259
#define EQ 260
#define NE 261
#define ID 262
#define INT 263
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,
};
short yylen[] = {                                         2,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    2,
    2,    3,    1,    1,
};
short yydefred[] = {                                      0,
    0,   13,   14,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   12,    0,
    0,    0,    0,    0,    0,    0,    0,    9,
};
short yydgoto[] = {                                       6,
};
short yysindex[] = {                                    -24,
  -24,    0,    0,  -24,  -24,  -32,  -18,  -18,  -38,  -24,
  -24,  -24,  -24,  -24,  -24,  -24,  -24,  -24,    0,  -26,
  -26,  -18,  -18,  -18,  -18,  -18,  -18,    0,
};
short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    1,    8,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   62,
   64,   18,   25,   32,   39,   47,   54,    0,
};
short yygindex[] = {                                    109,
};
#define YYTABLESIZE 243
short yytable[] = {                                      18,
   11,    0,   19,   10,   12,   18,   13,   10,   11,   10,
   12,   18,   13,    0,   11,    5,   12,    1,   13,   18,
    1,    0,    0,    0,    2,    0,    0,    0,    0,    0,
    0,    5,    0,    0,    0,    0,    0,    0,    6,    0,
    0,   11,   11,   11,    0,   11,    7,   11,   10,   10,
   10,    0,   10,    8,   10,    0,    0,    0,    1,    1,
    1,    4,    1,    3,    1,    2,    2,    2,    0,    2,
    0,    2,    5,    5,    5,    0,    5,    0,    5,    6,
    6,    6,    0,    6,    0,    6,    0,    7,    7,    7,
    0,    7,    0,    7,    8,    8,    8,    0,    8,    0,
    8,    4,    4,    4,    3,    3,    0,    0,    4,    7,
    3,    0,    8,    9,    0,    0,    0,    0,   20,   21,
   22,   23,   24,   25,   26,   27,   28,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   14,
   15,   16,   17,    0,    0,   14,   15,   16,   17,    0,
    0,   14,   15,   16,   17,    0,    0,    2,    3,   14,
   15,   16,   17,
};
short yycheck[] = {                                      38,
    0,   -1,   41,   42,   43,   38,   45,    0,   47,   42,
   43,   38,   45,   -1,   47,   40,   43,    0,   45,   38,
   45,   -1,   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,
   -1,    0,   -1,   -1,   -1,   -1,   -1,   -1,    0,   -1,
   -1,   41,   42,   43,   -1,   45,    0,   47,   41,   42,
   43,   -1,   45,    0,   47,   -1,   -1,   -1,   41,   42,
   43,    0,   45,    0,   47,   41,   42,   43,   -1,   45,
   -1,   47,   41,   42,   43,   -1,   45,   -1,   47,   41,
   42,   43,   -1,   45,   -1,   47,   -1,   41,   42,   43,
   -1,   45,   -1,   47,   41,   42,   43,   -1,   45,   -1,
   47,  126,   41,   42,   41,   42,   -1,   -1,   47,    1,
   47,   -1,    4,    5,   -1,   -1,   -1,   -1,   10,   11,
   12,   13,   14,   15,   16,   17,   18,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  258,
  259,  260,  261,   -1,   -1,  258,  259,  260,  261,   -1,
   -1,  258,  259,  260,  261,   -1,   -1,  262,  263,  258,
  259,  260,  261,
};
#define YYFINAL 6
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 263
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,"'&'",0,"'('","')'","'*'","'+'",0,"'-'",0,"'/'",0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'~'",0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"MINUS","LE","GE","EQ","NE","ID","INT",
};
char *yyrule[] = {
"$accept : expr",
"expr : expr '+' expr",
"expr : expr '-' expr",
"expr : expr '/' expr",
"expr : expr '*' expr",
"expr : expr LE expr",
"expr : expr GE expr",
"expr : expr EQ expr",
"expr : expr NE expr",
"expr : expr '&' expr",
"expr : '~' expr",
"expr : '-' expr",
"expr : '(' expr ')'",
"expr : ID",
"expr : INT",
};
#endif
#ifndef YYSTYPE
typedef int YYSTYPE;
#endif
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#ifdef YYSTACKSIZE
#ifndef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#endif
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE
#define YYABORT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("yydebug: state %d, reading %d (%s)\n", yystate,
                    yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("yydebug: state %d, shifting to state %d\n",
                    yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("yydebug: state %d, error recovery shifting\
 to state %d\n", *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("yydebug: error recovery discarding state %d\n",
                            *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("yydebug: state %d, error recovery discards token %d (%s)\n",
                    yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("yydebug: state %d, reducing by rule %d (%s)\n",
                yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
#line 16 "expr.y"
{yyval = yyvsp[-2] + yyvsp[0];}
break;
case 2:
#line 18 "expr.y"
{yyval = yyvsp[-2] - yyvsp[0];}
break;
case 3:
#line 20 "expr.y"
{yyval = yyvsp[-2] / yyvsp[0];}
break;
case 4:
#line 22 "expr.y"
{yyval = yyvsp[-2] * yyvsp[0];}
break;
case 5:
#line 24 "expr.y"
{yyval = yyvsp[-2] <= yyvsp[0];}
break;
case 6:
#line 26 "expr.y"
{yyval = yyvsp[-2] >= yyvsp[0];}
break;
case 7:
#line 28 "expr.y"
{yyval = yyvsp[-2] == yyvsp[0];}
break;
case 8:
#line 30 "expr.y"
{yyval = yyvsp[-2] != yyvsp[0];}
break;
case 9:
#line 32 "expr.y"
{yyval = yyvsp[-2] & yyvsp[0];}
break;
case 10:
#line 34 "expr.y"
{yyval = ~yyvsp[0];}
break;
case 11:
#line 36 "expr.y"
{yyval = 0-yyvsp[0];}
break;
case 12:
#line 38 "expr.y"
{yyval = yyvsp[-1];}
break;
#line 350 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("yydebug: after reduction, shifting from state 0 to\
 state %d\n", YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("yydebug: state %d, reading %d (%s)\n",
                        YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("yydebug: after reduction, shifting from state %d \
to state %d\n", *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
