//Don't try using this in your own project, it's got a lot of Asar-specific tweaks. Use mathlib.cpp instead.
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "autoarray.h"
#include "scapegoat.hpp"
#include "libstr.h"
#include "libsmw.h"
#include "asar.h"

bool math_pri=true;
bool math_round=false;

extern bool emulatexkas;

//int bp(const char * str)
//{
//	throw str;
//	return 0;
//}
//#define error(str) bp(str)
#define error(str) throw str
static const char * str;

static long double getnumcore();
static long double getnum();
static long double eval(int depth);

bool confirmname(const char * name);

//label (bool foundLabel) (bool forwardLabel)
//userfunction

bool foundlabel;
bool forwardlabel;


struct funcdat {
	autoptr<char*> name;
	int numargs;
	autoptr<char*> argbuf;//this one isn't used, it's just to free it up
	autoptr<char**> arguments;
	autoptr<char*> content;
};
autoarray<funcdat> userfunc;
int numuserfunc=0;

const char * createuserfunc(const char * name, const char * arguments, const char * content)
{
	if (!confirmqpar(content)) return "Mismatched parentheses";
	for (int i=0;i<numuserfunc;i++)
	{
		if (!strcmp(name, userfunc[i].name))
		{
			return "Duplicate function name";
		}
	}
	funcdat& thisone=userfunc[numuserfunc];
	thisone.name=strdup(name);
	thisone.argbuf=strdup(arguments);
	thisone.arguments=qsplit(thisone.argbuf, ",", &(thisone.numargs));
	thisone.content=strdup(content);
	for (int i=0;thisone.arguments[i];i++)
	{
		if (!confirmname(thisone.arguments[i]))
		{
			userfunc.remove(numuserfunc);
			return "Invalid argument name";
		}
	}
	numuserfunc++;
	return NULL;
}

char ** funcargnames;
long double * funcargvals;
int numuserfuncargs;


int snestopc_pick(int addr);

static long double validaddr(long double in, long double len)
{
	int addr=snestopc_pick(in);
	if (addr<0 || addr+len-1>romlen_r) return 0;
	else return 1;
}

static long double read1(long double in)
{
	int addr=snestopc_pick(in);
	if (addr<0) error("read1(): Address doesn't map to ROM.");
	else if (addr+1>romlen_r) error("Address out of bounds.");
	else return
			 romdata_r[addr  ]     ;
	return 0;
}

static long double read2(long double in)
{
	int addr=snestopc_pick(in);
	if (addr<0) error("read2(): Address doesn't map to ROM.");
	else if (addr+2>romlen_r) error("Address out of bounds.");
	else return
			 romdata_r[addr  ]    |
			(romdata_r[addr+1]<< 8);
	return 0;
}

static long double read3(long double in)
{
	int addr=snestopc_pick(in);
	if (addr<0) error("read3(): Address doesn't map to ROM.");
	else if (addr+3>romlen_r) error("Address out of bounds.");
	else return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8)|
			(romdata_r[addr+2]<<16);
	return 0;
}

static long double read4(long double in)
{
	int addr=snestopc_pick(in);
	if (addr<0) error("read4(): Address doesn't map to ROM.");
	else if (addr+4>romlen_r) error("Address out of bounds.");
	else return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8)|
			(romdata_r[addr+2]<<16)|
			(romdata_r[addr+3]<<24);
	return 0;
}

static long double read1s(long double in, long double def)
{
	int addr=snestopc_pick(in);
	if (addr<0) return def;
	else if (addr+0>romlen_r) return def;
	else return
			 romdata_r[addr  ]     ;
	return 0;
}

static long double read2s(long double in, long double def)
{
	int addr=snestopc_pick(in);
	if (addr<0) return def;
	else if (addr+1>romlen_r) return def;
	else return
			 romdata_r[addr  ]    |
			(romdata_r[addr+1]<< 8);
	return 0;
}

static long double read3s(long double in, long double def)
{
	int addr=snestopc_pick(in);
	if (addr<0) return def;
	else if (addr+2>romlen_r) return def;
	else return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8)|
			(romdata_r[addr+2]<<16);
	return 0;
}

static long double read4s(long double in, long double def)
{
	int addr=snestopc_pick(in);
	if (addr<0) return def;
	else if (addr+3>romlen_r) return def;
	else return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8)|
			(romdata_r[addr+2]<<16)|
			(romdata_r[addr+3]<<24);
	return 0;
}

extern unsigned int table[256];

static long double getnumcore()
{
	if (*str=='(')
	{
		str++;
		long double rval=eval(0);
		if (*str!=')') error("Mismatched parentheses.");
		str++;
		return rval;
	}
	if (*str=='$')
	{
		if (!isxdigit(str[1])) error("Invalid hex value.");
		if (tolower(str[2])=='x') return -42;//let str get an invalid value so it'll throw an invalid operator later on
		return strtoul(str+1, (char**)&str, 16);
	}
	if (*str=='%')
	{
		if (str[1]!='0' && str[1]!='1') error("Invalid binary value.");
		return strtoul(str+1, (char**)&str, 2);
	}
	if (*str=='\'')
	{
		if (!str[1] || str[2]!='\'') error("Invalid character.");
		unsigned int rval=table[(unsigned char)str[1]];
		str+=3;
		return rval;
	}
	if (isdigit(*str))
	{
		return strtod(str, (char**)&str);
	}
	if (isalpha(*str) || *str=='_' || *str=='.' || *str=='?')
	{
		const char * start=str;
		while (isalnum(*str) || *str=='_') str++;
		int len=str-start;
		while (*str==' ') str++;
		if (*str=='(')
		{
			str++;
			while (*str==' ') str++;
			autoarray<long double> params;
			int numparams=0;
			if (*str!=')')
			{
				while (true)
				{
					while (*str==' ') str++;
					params[numparams++]=eval(0);
					while (*str==' ') str++;
					if (*str==',')
					{
						str++;
						continue;
					}
					if (*str==')')
					{
						str++;
						break;
					}
					error("Malformed function call.");
				}
			}
			long double rval;
			for (int i=0;i<numuserfunc;i++)
			{
				if ((int)strlen(userfunc[i].name)==len && !strncmp(start, userfunc[i].name, len))
				{
					if (userfunc[i].numargs!=numparams) error("Wrong number of parameters to function.");
					char ** oldfuncargnames=funcargnames;
					long double * oldfuncargvals=funcargvals;
					const char * oldstr=str;
					int oldnumuserfuncargs=numuserfuncargs;
					funcargnames=userfunc[i].arguments;
					funcargvals=params;
					str=userfunc[i].content;
					numuserfuncargs=numparams;
					rval=eval(0);
					funcargnames=oldfuncargnames;
					funcargvals=oldfuncargvals;
					str=oldstr;
					numuserfuncargs=oldnumuserfuncargs;
					return rval;
				}
			}
			if (*str=='_') str++;
#define func(name, numpar, code)                                   \
					if (!strncasecmp(start, name, len))                      \
					{                                                        \
						if (numparams==numpar) return (code);                  \
						else error("Wrong number of parameters to function."); \
					}
#define varfunc(name, code)                                   \
					if (!strncasecmp(start, name, len))                      \
					{                                                        \
						code; \
					}
			func("sqrt", 1, sqrt(params[0]));
			func("sin", 1, sin(params[0]));
			func("cos", 1, cos(params[0]));
			func("tan", 1, tan(params[0]));
			func("asin", 1, asin(params[0]));
			func("acos", 1, acos(params[0]));
			func("atan", 1, atan(params[0]));
			func("arcsin", 1, asin(params[0]));
			func("arccos", 1, acos(params[0]));
			func("arctan", 1, atan(params[0]));
			func("log", 1, log(params[0]));
			func("log10", 1, log10(params[0]));
			func("log2", 1, log(params[0])/log(2.0));
			func("read1", 1, read1(params[0]));
			func("read2", 1, read2(params[0]));
			func("read3", 1, read3(params[0]));
			func("read4", 1, read4(params[0]));
			func("read1", 2, read1s(params[0], params[1]));
			func("read2", 2, read2s(params[0], params[1]));
			func("read3", 2, read3s(params[0], params[1]));
			func("read4", 2, read4s(params[0], params[1]));
			func("canread1", 1, validaddr(params[0], 1));
			func("canread2", 1, validaddr(params[0], 2));
			func("canread3", 1, validaddr(params[0], 3));
			func("canread4", 1, validaddr(params[0], 4));
			func("canread", 2, validaddr(params[0], params[1]));
			//varfunc("min", {
			//		if (!numparams) error("Wrong number of parameters to function.");
			//		double minval=params[0];
			//		for (int i=1;i<numparams;i++)
			//		{
			//			if (params[i]<minval) minval=params[i];
			//		}
			//		return minval;
			//	});
			//varfunc("max", {
			//		if (!numparams) error("Wrong number of parameters to function.");
			//		double maxval=params[0];
			//		for (int i=1;i<numparams;i++)
			//		{
			//			if (params[i]>maxval) maxval=params[i];
			//		}
			//		return maxval;
			//	});
#undef func
#undef varfunc
			error("Unknown function.");
		}
		else
		{
			for (int i=0;i<numuserfuncargs;i++)
			{
				if (!strncmp(start, funcargnames[i], len)) return funcargvals[i];
			}
			foundlabel=true;
			int i=labelval(&start);
			str=start;
			//if (start!=str) error("Internal error. Send this patch to Alcaro.");//not gonna add sublabel/macrolabel support here
			if (i==-1) forwardlabel=true;
			return (int)i&0xFFFFFF;
//#define const(name, val) if (!strncasecmp(start, name, len)) return val
//			const("pi", 3.141592653589793238462);
//			const("\xCF\x80", 3.141592653589793238462);
//			const("\xCE\xA0", 3.141592653589793238462);//case insensitive pi, yay
//			const("e", 2.718281828459045235360);
//#undef const
//			error("Unknown constant.");
		}
	}
	error("Invalid number.");
}

static long double sanitize(long double val)
{
	if (val!=val) error("NaN encountered.");
	if (math_round) return (int)val;
	return val;
}

static long double getnum()
{
	while (*str==' ') str++;
#define prefix(name, func) if (!strncasecmp(str, name, strlen(name))) { str+=strlen(name); long double val=getnum(); return sanitize(func); }
	prefix("-", -val);
	prefix("~", ~(int)val);
	prefix("+", val);
	if (emulatexkas) prefix("#", val);
#undef prefix
	return sanitize(getnumcore());
}

extern autoarray<int> poslabels;
extern autoarray<int> neglabels;

static long double eval(int depth)
{
	if (str[0]=='+' || str[0]=='-')
	{
		int i;
		char top=str[0];
		for (i=0;str[i] && str[i]!=')';i++)
		{
			if (str[i]!=top) goto notposneglabel;
		}
		str+=i;
		foundlabel=true;
		if (top=='+') forwardlabel=true;
		if (top=='+') return labelval(S":pos_"+dec(i)+"_"+dec(poslabels[i]))&0xFFFFFF;
		else          return labelval(S":neg_"+dec(i)+"_"+dec(neglabels[i]))&0xFFFFFF;
	}
notposneglabel:
	recurseblock rec;
	long double left=getnum();
	long double right;
	while (*str==' ') str++;
	while (*str && *str!=')' && *str!=',')
	{
		while (*str==' ') str++;
		if (math_round) left=(int)left;
#define oper(name, thisdepth, contents)      \
			if (!strncmp(str, name, strlen(name))) \
			{                                      \
				if (math_pri)                        \
				{                                    \
					if (depth<=thisdepth)              \
					{                                  \
						str+=strlen(name);               \
						right=eval(thisdepth+1);         \
					}                                  \
					else return left;                  \
				}                                    \
				else                                 \
				{                                    \
					str+=strlen(name);                 \
					right=getnum();                    \
				}                                    \
				left=sanitize(contents);             \
				continue;                            \
			}
		oper("**", 4, pow(left, right));
		oper("*", 3, left*right);
		oper("/", 3, right?left/right:error("Division by zero."));
		oper("%", 3, right?fmod(left, right):error("Modulos by zero."));
		oper("+", 2, left+right);
		oper("-", 2, left-right);
		oper("<<", 1, (unsigned int)left<<(unsigned int)right);
		oper(">>", 1, (unsigned int)left>>(unsigned int)right);
		oper("&", 0, (unsigned int)left&(unsigned int)right);
		oper("|", 0, (unsigned int)left|(unsigned int)right);
		oper("^", 0, (unsigned int)left^(unsigned int)right);
		error("Unknown operator.");
#undef oper
	}
	return left;
}

//static autoptr<char*> freeme;
long double math(const char * s, const char ** e)
{
	//free(freeme);
	//freeme=NULL;
	foundlabel=false;
	forwardlabel=false;
	try
	{
		str=s;
		long double rval=eval(0);
		if (*str)
		{
			if (*str==',') error("Invalid input.");
			else error("Mismatched parentheses.");
		}
		*e=NULL;
		return rval;
	}
	catch (const char * error)
	{
		*e=error;
		return 0;
	}
	//catch (string& error)
	//{
	//	freeme=strdup(error);
	//	*e=error;
	//	return 0;
	//}
}

void initmathcore()
{
	//not needed
}

void deinitmathcore()
{
	userfunc.reset();
}
