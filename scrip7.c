#define true
true /*
echo "Compiling binaries and extracting doc and test..."
cc $0 -Wall -Wextra -lm -fwrapv -o scrip7
cc -DNOS7MAIN $0 -c -lm -fwrapv -o scrip7.o
grep -A14 'B[E]GINHEADER' $0 >scrip7.h
cat >docs.txt <<END
Author: Oren Watson
Scrip7 hopefully getting stabler release 2015-05-12
You can extract and compile the full distribution simply by running this
file as a shell script like sh scrip7.c This produces the interpreter scrip7,
the library scrip7.o, the documentation docs.txt.
To include, include this file. If your platform does not support weak symbols, 
you will also need to define NOS7MAIN when including it.
Changes in this new version:
Ide removed for now, it sucked. Try rlwrap scrip7 instead.
g and h are now relatively normal registers, with VARIANT type adjusting
between double, int64, and pointer depending on the other operand. Their
use as code pointers was a dumb idea.
Instead we have K (label) and G (goto) instructions. K saves the current
place to a register. G goes to the place in a register.
Able to call C functions with simple signatures, using the C (call)
instruction. Changed W and R to allow access to other streams, and use
N register, the way S and L instructions do.
Changed p, r, x, ,, ., to allow other streams, while retaining default
streams when using _ register.
Added O instruction to open a file.

Language Description:
A scrip7 program or script consists of statements, jumps, and loop brackets.
Jump targets are written with #, Loops are written with {} and [] 
The {} brackets will jump  to the matching {} or [] bracket. [] brackets
are inert labels. The quickest way to see how these are supposed to be used, 
is to see the example code.
Statements consist of a register letter, an optional offset, an operator,
and either a literal, or another register and optional offset.
There are 6 registers which store memory addresses, and whose notation 
depends on the type of data they are being used to point to. The data types 
in scrip7 are: 8 bit integers, 16 bit integers, 32 bit integers, 64 bit 
integers, 32 bit floating point, 64 bit floating point, pointers, and the 
"address" type which means accessing the address the register stores.
The notation for registers is in the following table:
int8 int16 int32 int64 float double pointer addr
  a    A     i     I     u     U      o      O
  b    B     j     J     v     V      p      P
  c    C     k     K     w     W      q      Q
  d    D     l     L     x     X      r      R
  e    E     m     M     y     Y      s      S
  f    F     n     N     z     Z      t      T
                   g           g      g      G
                   h           h      h      H
A special register called _ is supported for the first register in a statement.
It is zero when read, and does nothing when written to.
The offsets are added to the address of the accessed data. An offset of
the form (1234 counts by bytes. An offset of the form )1234 counts by
the size of the addressed data type.
The literal syntax is as follows: 
1234    Decimal integer.
%34af   Hexadecimal integer. A space at the end is mandatory.
3%ff    Repeated hex. 3%ff is equivalent to %ffffffff .
12.34   Decimal floating point number.
12/34   Fractional floating point number.
1.23^4  Decimal floating point with decimal exponent.
{...}   Delimited string or code literal.
3"foo   Hollerith constant string. end " is not allowed or needed.
'a      Character constant. No ' after it.
The operators have a common semantics where usually only the first register's
data or address (hereafter called the destination) is modified, and the
second operand (hereafter called the source) is only read.
If the second operand is a literal, or the first operand is the _ register,
modifying it does nothing.
Command List:
END
grep '^\( \|	\)*\(c\|br\)ase' scrip7.c | sed -e "s/.*ase '/α/; s/\\\\//; s/': \\/\\//β /; s/src/β/g; s/dest/α/g" >>docs.txt
echo "Running test:"
./scrip7 >out <<END
I=260I%170I+8I*9I/2I-5_pI {checks arithmetic}
I=7%7f I&6%e0 I|3%01 IX7%0f _xI {checks bit operations}
I=5I^5_pII=3I^4_pI {checks exponentiation}
u=43/8_xi {basic check for ieee float}
U=35478/256_xI {basic check for ieee double}
U=355/113_pU {check real number output}
I=1J>1[J+II+JIl1000}#_pI {check basic looping}
I=%5253545556575859{insert newline}b='
 [_.aa>1O!P(1}# {check character output, char literals, and byte order}
X=4.5325_pX_xXJ\X_pJJ>1_pJ 
{check float literals and output some more and undivide}
I=-99_pI {check negative integer literals}
J<2J:23J:17J:27J:67J:44
{Now a test of insertion sort}
[O=RQ=R[IlKQ=O#I>1O!P}#KzLL>1R!P}#
O=SP=SQ=SR=S
_pI_pI)1_pI)2_pI)3_pI)4 {verify sort result and register offsets}
_z0 {modifying void register and constants should do nothing}
END
if diff out - <<END
436
0f6f6f6f6e6e6e6e
3125
81
40ac0000
406152c000000000
3.141593
1597
YXWVUTSR
4.532500
1813/400
1813
400
-99
17
23
27
44
67
END
then
echo "Success"
else
echo "Failure"
fi
rm out
exit 0
true */
//BEGINHEADER
#include "stdio.h"
extern int s7logflag,s7freeflag,s7errflag,s7memsafe;
/*Declarations for non-static*/
void scrip7(char *code);
/*execute string*/
void scrip7f(FILE *code);
/*execute whole file as a series of programs.*/
int scrip7rc(FILE *code);
/*execute a program, ending with .\n, returns 0 if the program was empty.*/
void scrip7cli(FILE *code);
/*execute a set of programs, ending with an empty program.*/
void scrip7mem(char *data,int size);
/*provide your own env instead of default one*/
//ENDHEADER

struct scrip7state{
	char *v[8];
	char *data;
	char *stack;
}globa7;

int s7logflag=0;
int s7freeflag=0;
int s7errflag=1;

#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#include "math.h"
#include "inttypes.h"
#include "stddef.h"
#include "unistd.h"
#define ei else if
#define un(x) if(!(x))
#define wh while
#define loop for(;;)
#define brase break;case
#define brault break;default
#define sut struct
#define let(x,y) typeof(y) x = y
#define new(T,y...) ({T *$$tmp=malloc(sizeof(T)); *$$tmp=(T){y}; $$tmp;})

struct va7 {int64_t c;double x;char *p;};

static char *s7getcmd(FILE *f){
	int n=80,l=0;
	char *s=NULL,*q;
	loop{
		s=realloc(s,n);
		q=fgets(s+l,n-l,f);
		if(!q)return s;
		l+=strlen(s+l);
		if(s[l-1]=='\n')
		if(s[l-2]=='.'){
			s[l-2]=0;
			return s;
		}
		if(l==n-1)n*=2;
	}
}

int undiv(double x,int64_t*np,int64_t*dp){
	double y;int i=0;
	int64_t p=0,q=1,r=1,s=0,n;
	do{i++;if(i>10)break;
	n = (int64_t)((p-x*q)/(x*s-r));
	p += n*r;q += n*s;y = (double)p/q;
	if(y==x){*np=p;*dp=q;return i;}
	n = (int64_t)((r-x*s)/(x*q-p));
	r += n*p;s += n*q;y = (double)r/s;
	}while(y!=x);
	*np=r;*dp=s;return i;
}

static void memsetl(char *r,char *s,int len,int num){
	memmove(r,s,len);
	int k=1;
	wh(k<num){
		memcpy(r+k*len,r,len*(k*2>num?num-k:k));
		k*=2;
	}
}

static char *s7getinput(int fn,char b){
	int n=80,l=0;
	char *s=malloc(n);
	wh(1){
		read(fn,s+l,1);
		if(s[l]==b)break;
		l++;
		if(l==n-1){
			n*=2;
			s=realloc(s,n);
		}
	}
	s[l]=0;
	return s;
}

#define TY_INT8 0
#define TY_INT16 1
#define TY_INT32 2
#define TY_INT64 3
#define TY_ADDR 4
#define TY_PTR 5
#define TY_FLOAT 6
#define TY_DOUBLE 7
#define TY_VARIANT 8
static char *tpnm[]={"int8","int16","int32","int64",
		"addr","void*","float","double","variant"};
static int tpsz[]={1,2,4,8,1,sizeof(void*),sizeof(float),sizeof(double),8};
static int isint[]={1,1,1,1,0,0,0,0,0};
static int isreal[]={0,0,0,0,0,0,1,1,0};
static int isptr[]={0,0,0,0,1,1,0,0,0};
static int isnum[]={1,1,1,1,0,0,1,1,0};
#define VR_AIOU 0
#define VR_BJPV 1
#define VR_CKQW 2
#define VR_DLRX 3
#define VR_EMSY 4
#define VR_FNTZ 5
#define VR_G 6
#define VR_H 7
#define VR_LIT 8
#define VR_VOID 9
static char *vrnm[]={"aiou","bjpv","ckqw","dlrx","emsy","fntz",
		"g","h","constant","voidvar"};

#define Log(mesg,args...) {if(s7logflag)fprintf(stderr,"%ld:"mesg"\n",g-code,##args);}wh(0)

#define Die(mesg) {if(s7errflag)fprintf(stderr,"%ld:"mesg"\n",g-code);goto hell;}wh(0)

static void gettype(char*g,int*d,int *dt){
	if(*g>='a'&&*g<='f')*d=*g-'a',*dt=TY_INT8;
	ei(*g>='A'&&*g<='F')*d=*g-'A',*dt=TY_INT16;
	ei(*g>='i'&&*g<='n')*d=*g-'i',*dt=TY_INT32;
	ei(*g>='I'&&*g<='N')*d=*g-'I',*dt=TY_INT64;
	ei(*g>='o'&&*g<='t')*d=*g-'o',*dt=TY_PTR;
	ei(*g>='O'&&*g<='T')*d=*g-'O',*dt=TY_ADDR;
	ei(*g>='u'&&*g<='z')*d=*g-'u',*dt=TY_FLOAT;
	ei(*g>='U'&&*g<='Z')*d=*g-'U',*dt=TY_DOUBLE;
	ei(*g=='G')*d=VR_G,*dt=TY_ADDR;
	ei(*g=='H')*d=VR_H,*dt=TY_ADDR;
	ei(*g=='g')*d=VR_G,*dt=TY_VARIANT;
	ei(*g=='h')*d=VR_H,*dt=TY_VARIANT;
	ei(*g=='_')*d=VR_VOID,*dt=-1;
}

static void setva7(int dt,char **dp, ptrdiff_t dx, struct va7*v){
	if(dp==0)return;
	if(dt==TY_INT8)(*(int8_t*)(*dp+dx)) = v->c;
	ei(dt==TY_INT16)(*(int16_t*)(*dp+dx)) = v->c;
	ei(dt==TY_INT32)(*(int32_t*)(*dp+dx)) = v->c;
	ei(dt==TY_INT64)(*(int64_t*)(*dp+dx)) = v->c;
	ei(dt==TY_ADDR)(*dp) = v->p-dx;
	ei(dt==TY_PTR)(*(void**)(*dp+dx)) = v->p;
	ei(dt==TY_FLOAT)(*(float*)(*dp+dx)) = v->x;
	ei(dt==TY_DOUBLE)*(double*)(*dp+dx) = v->x;
}

static void getva7(int st,char **sp, ptrdiff_t sx, struct va7 *v){
	if(!sp){
		v->c=0;
		v->p=NULL;
		v->x=0.0;
		return;
	}
	if(st==TY_INT8)   v->c=*(int8_t*)(*sp+sx);
	if(st==TY_INT16)  v->c=*(int16_t*)(*sp+sx);
	if(st==TY_INT32)  v->c=*(int32_t*)(*sp+sx);
	if(st==TY_INT64||st==TY_VARIANT)  v->c=*(int64_t*)(*sp+sx);
	if(st==TY_ADDR)   v->p=(*sp+sx);
	if(st==TY_PTR||st==TY_VARIANT)    v->p=*(void**)(*sp+sx);
	if(st==TY_FLOAT)  v->x=*(float*)(*sp+sx);
	if(st==TY_DOUBLE||st==TY_VARIANT) v->x=*(double*)(*sp+sx);
	if(st<=TY_INT64){ v->p=(*sp+sx);v->x=v->c;}
	ei(st<=TY_PTR){   v->c=(int64_t)v->p;v->x=v->c;}
	ei(st<=TY_DOUBLE){v->c=v->x;v->p=(*sp+sx);}
}

static int64_t memlen(char *d,struct va7 *s,int t){
	int64_t n=0;
	struct va7 v;
	loop{
		char *q=d+tpsz[t]*n;
		getva7(t,&q,0,&v);
		if(t<TY_ADDR&&v.c==s->c)break;
		ei(t<TY_FLOAT&&t>=TY_ADDR&&v.p==s->p)break;
		ei(t<8&&t>=TY_FLOAT&&v.x==s->x)break;
		n++;
	}
	return n;
}

#define usg uintptr_t

static char *fwdjmptbl[256][2];
static uint8_t bakjmptbl[256];
static int pcache(char **s){
	if(**s=='{'){
		uint8_t k = ((usg)*s)%256;
		if(fwdjmptbl[k][0]==*s){
			*s = fwdjmptbl[k][1]+1;
			return 1;
		}else return 0;
	}
	if(**s=='}'){
		uint8_t k = bakjmptbl[((usg)*s)%256];
		if(fwdjmptbl[k][1]==*s){
			*s = fwdjmptbl[k][0]+1;
			return 1;
		}else return 0;
	}
	return 0;
}

static char *skipp(char *s){
	int l=0,nl=0;
	char *ss=s;
	if(*s=='['||*s==']')return s+1;
	un(*s=='{'||*s=='}')return s;
	if(pcache(&s))return s;
	do{
		if(*s=='['||*s=='{')nl=l+1;
		if(*s==']'||*s=='}')nl=l-1;
		if(nl==0){
			usg ssh = (usg)ss%256, sh = (usg)s%256;
			if(l>0){
				fwdjmptbl[ssh][0]=ss;
				fwdjmptbl[ssh][1]=s;
				bakjmptbl[sh]=ssh;
			}ei(l<0){
				fwdjmptbl[sh][0]=s;
				fwdjmptbl[sh][1]=ss;
				bakjmptbl[ssh]=sh;
			}
			return ++s;
		}
		l=nl;
		if(l<0)s--;
		if(l>0)s++;
	}wh(1);
}

static int64_t parsedec(char **gp){
	int64_t i=0;
	wh(**gp>='0'&&**gp<='9'){
		i=i*10+*(*gp)++-'0';
	}
	return i;
}

static void parsenum(char **gp,int *stp,struct va7 *sp){
	int64_t i=0,j;
	double x;
	char *p=0;
	int sgn=1;
	char *g=*gp;
	if(*g>='0'&&*g<='9'){
		i=parsedec(&g);
	}else if(*g=='-'){
		sgn = -1;
		g++;
		i=-parsedec(&g);
	}
	x = i;
	*stp = TY_INT64;
	if(*g=='.'){
		*stp = TY_DOUBLE;
		g++;
		j=0;
		while(*g>='0'&&*g<='9'){
			i=i*10+(*g++-'0')*sgn;
			j++;
		}
		i=x=(double)i/pow(10,j);
		if(*g=='^'){
			g++;
			j=parsedec(&g);
			x*=pow(10,j);
		}
	}ei(*g=='/'){
		*stp = TY_DOUBLE;
		g++;
		j=parsedec(&g);
		i=x=(double)i/j;
	}ei(*g=='%'){
		g++;
		j=0;
		int l=0;
		loop{
			int k=(*g+30)%39;
			if(k>=16)break;
			j=j*16+k;
			g++;l++;
		}
		int64_t t=j;
		wh(i>0){
			j=(j<<(l*4))+t;
			i--;
		}
		i=j;
		x=i;
	}ei(*g=='"'){
		p=(char*)malloc(i+1);
		p[i]=0;
		for(j=0;j<i;j++){
			p[j]=*++g;
		}
		g++;
		*stp=TY_ADDR;
	}else{
		x=i;
	}
	sp->p = p;
	sp->c = i;
	sp->x = x;
	*gp = g;
}

void scrip7(char *code){
	char **v=globa7.v;
	int i;
	for(i=0;i<8;i++)v[i]=globa7.data;
	char *g=code;
	int d,dt,s,st;
	char op;
	intptr_t dx,sx;
	struct va7 dv,sv;
	wh(*g){
		char*j=skipp(g);
		if(j!=g)Log("skipped parens");
		g=j;
		if(*g=='`'){Log("backtick is early exit");goto hell;}
		if((*g>0&&*g<=' ')||*g=='#'){g++;continue;}
		un((*g<='Z'&&*g>='A')||(*g<='z'&&*g>='a')||*g=='_')
			Die("bad dest name");
		gettype(g,&d,&dt);
		dx=0;
		sx=0;
		g++;
		wh(*g>0&&*g<=' ')g++;
		if(*g==0)Die("unexpected zero");
		if(*g=='('){
			g++;
			dx=parsedec(&g);
		}ei(*g==')'){
			g++;
			dx=parsedec(&g)*tpsz[dt];
		}
		if(*g==0)Die("unexpected zero");
		op=*g;
		g++;
		if(*g==0)Die("unexpected zero");
		if((*g<='z'&&*g>='a')||(*g<='Z'&&*g>='A')){
			gettype(g,&s,&st);
			g++;
			if(*g=='('){
				g++;
				sx=parsedec(&g);
			}ei(*g==')'){
				g++;
				sx=parsedec(&g)*tpsz[st];
			}
		}ei((*g<='9'&&*g>='0')||*g=='.'||*g=='-'||*g=='%'){
			s=8;
			parsenum(&g,&st,&sv);
		}ei(*g=='\''){
			s=8;
			st=TY_INT8;
			g++;
			sv.x=sv.c=*g;
			g++;
		}ei(*g=='{'){
			s=8;
			st=TY_ADDR;
			sv.p=g+1;
			sv.x=sv.c=0;
			g=skipp(g)+1;
		}else Die("bad arg");
		if(dt==-1)dt=st;
		Log("%s %"PRIuPTR" %s %c %s %"PRIuPTR" %s",
			vrnm[d],dx,tpnm[dt],op,vrnm[s],sx,tpnm[st]);
		char **sp=0;
		if(s<8){
			sp=v+s;
			getva7(st,sp,sx,&sv);
		}
		char **dp=(d<8)?&(v[d]):0;
		getva7(dt,dp,dx,&dv);
		if(dt==TY_VARIANT)dt=st;
		if(st==TY_VARIANT)st=dt;
		if(dt==TY_VARIANT)Die("Cannot perform operation on two variants.");
		int skip=0;
		char *r;
		int dvn=d;
		int fdes;
		int64_t n,d;
		struct va7 nv;
		char pbuf[100];
		switch(op){
		case '=': //set dest to src
			setva7(dt,dp,dx,&sv);
		brase 'z': //swap dest and src
			setva7(st,sp,sx,&dv);
			setva7(dt,dp,dx,&sv);
		brase 'p': //print src to stream given by dest. _ is stdout
			if(dvn!=VR_VOID&&!isint[dt])Die("dest must be a stream number, as an integer.");
			if(dvn==VR_VOID)fdes=1;
			else fdes=dv.c;
			if(isint[st])sprintf(pbuf,"%"PRId64"\n",sv.c);
			ei(isptr[st])sprintf(pbuf,"%p\n",sv.p);
			ei(isreal[st])sprintf(pbuf,"%f\n",sv.x);
			write(fdes,pbuf,strlen(pbuf));
		brase 'x': //print src in hex to stream given by dest. _ is stdout
			if(dvn!=VR_VOID&&!isint[dt])Die("dest must be a stream number, as an integer.");
			if(dvn==VR_VOID)fdes=1;
			else fdes=dv.c;
			if(dt==TY_INT8)sprintf(pbuf,"%02"PRIx8"\n",(uint8_t)sv.c);
			ei(dt==TY_INT16)
				sprintf(pbuf,"%04"PRIx16"\n",(uint16_t)sv.c);
			ei(dt==TY_INT32)
				sprintf(pbuf,"%08"PRIx32"\n",(uint32_t)sv.c);
			ei(dt==TY_INT64)sprintf(pbuf,"%016"PRIx64"\n",sv.c);
			ei(dt<=TY_PTR)sprintf(pbuf,"%p\n",sv.p);
			else{
				int64_t n,d;
				undiv(sv.x,&n,&d);
				sprintf(pbuf,"%"PRId64"/%"PRId64"\n",n,d);
			}
			write(fdes,pbuf,strlen(pbuf));
		brase 'r': //read in a line from stream given by src and put value in dest
			if(!isint[st])Die("src must be a stream number, as an integer.");
			r=s7getinput(sv.c,'\n');
			if(dt<=TY_INT64)dv.c=strtol(r,0,0);
			ei(dt==TY_FLOAT||dt==TY_DOUBLE)dv.x=strtod(r,0);
			else dv.p = r;
			if(!isptr[dt])free(r);
			setva7(dt,dp,dx,&dv);
		brase 'S': //copy N objects from src to dest, if src is an address. (N being the register N of course) otherwise, set N objects in dest to the value of src.
			r=*dp;
			getva7(TY_INT64,&v[5],0,&nv);
			if(isptr[st]){
				memmove(r,*sp,nv.c*tpsz[dt]);
			}else{
				memsetl(r,*sp,tpsz[st],nv.c);
			}
		brase 'L': //get number of objects in dest until the value of src is reached. set N to it.
			nv.c = memlen(*dp,&sv,dt);
			setva7(TY_INT64,v+5,0,&nv);
		brase '\\': //undivide src into two consecutive values in dest.
			if(!isnum[dt])Die("Undivide result must be placed in numeric type.");
			undiv(sv.x,&n,&d);
			dv.x=dv.c = n;
			setva7(dt,dp,dx,&dv);
			dv.x=dv.c = d;
			*dp = *dp + tpsz[dt];
			setva7(dt,dp,dx,&dv);
			*dp = *dp - tpsz[dt];
		brase 'R': //read N objects of dest's type into dest, from steem number src.
			if(!isint[st])Die("Source must be a stream number, as an integer.");
			getva7(TY_INT64,&v[5],0,&nv);
			read(sv.c,*dp,tpsz[dt]*nv.c);
		brase 'W': //write N objects of dest's type from dest, to stream number src..
			if(!isint[st])Die("Source must be a stream number, as an integer.");
			getva7(TY_INT64,&v[5],0,&nv);
			write(sv.c,*dp,tpsz[dt]*nv.c);
		brase '.': //put src as char, to stream indicated by dest. _ is stdout.
			if(dvn!=VR_VOID&&!isint[dt])Die("dest must be a stream number, as an integer.");
			if(dvn==VR_VOID)fdes=1;
			else fdes = dv.c;
			write(fdes,&sv.c,1);
		brase ',': //get char into dest, from stream indicated by src.
			if(!isint[st])Die("src must be a stream number, as an integer.");
			{char c;
			read(sv.c,&c,1);
			dv.x=dv.c=c;}
			setva7(dt,dp,dx,&dv);
		brase '+': //add src to dest.
			dv.c+=sv.c;
			dv.x+=sv.x;
			dv.p+=sv.c;
			setva7(dt,dp,dx,&dv);
		brase '-': //subtract src from dest.
			dv.c-=sv.c;
			dv.x-=sv.x;
			dv.p-=sv.c;
			setva7(dt,dp,dx,&dv);
		brase '/': //divide dest by src
			if(sv.c)dv.c/=sv.c;
			dv.x/=sv.x;
			if(isptr[st]||isptr[dt])Die("Can't divide pointers.");
			setva7(dt,dp,dx,&dv);
		brase '%': //modulo dest by src
			dv.c%=sv.c;
			dv.x=fmod(dv.x,sv.x);
			if(isptr[st]||isptr[dt])Die("Can't modulo pointers.");
			setva7(dt,dp,dx,&dv);
		brase '*': //multiply dest by src
			dv.c*=sv.c;
			dv.x*=dv.x;
			if(isptr[st]||isptr[dt])
				Die("Can't multiply pointers.");
			setva7(dt,dp,dx,&dv);
		brase '|': //bitor src into dest
			dv.c|=sv.c;
			if(!isint[st]||!isint[dt])
				Die("Can only bitor integers.");
			setva7(dt,dp,dx,&dv);
		brase '&': //bitand src with dest
			dv.c&=sv.c;
			if(!isint[st]||!isint[dt])
				Die("Can only bitand integers.");
			setva7(dt,dp,dx,&dv);
		brase '^': //raise dest to src power
			dv.c=pow(dv.c,sv.x);
			dv.x=pow(dv.x,sv.x);
			if(isptr[st]||isptr[dt])
				Die("Can only exponent numbers.");
			setva7(dt,dp,dx,&dv);
		brase 'c': //multiply dest by cos src
			dv.c=dv.x=dv.x*cos(sv.x);
			if(isptr[st]||isptr[dt])
				Die("Can only cosine numbers.");
			setva7(dt,dp,dx,&dv);
		brase 's': //multiply dest by sin src
			dv.c=dv.x=dv.x*sin(sv.x);
			if(isptr[st]||isptr[dt])
				Die("Can only sine numbers.");
			setva7(dt,dp,dx,&dv);
		brase '_': //multiply dest by log src
			dv.c=dv.x=dv.x*log(sv.x);
			if(isptr[st]||isptr[dt])
				Die("Can only logarithm numbers.");
			setva7(dt,dp,dx,&dv);
		brase 't': //multiply dest by tan src
			dv.c=dv.x=dv.x*tan(sv.x);
			if(isptr[st]||isptr[dt])
				Die("Can only tangent numbers.");
			setva7(dt,dp,dx,&dv);
		brase 'a': //set dest to atan(dest/src)
			dv.c=dv.x=atan2(dv.x,sv.x);
			if(isptr[st]||isptr[dt])
				Die("Can only arctangent numbers.");
			setva7(dt,dp,dx,&dv);
		brase 'e': //multiply dest by e^src
			dv.c=dv.x=dv.x*exp(sv.x);
			if(isptr[st]||isptr[dt])
				Die("Can only exponent numbers.");
			setva7(dt,dp,dx,&dv);
		brase 'X': //xor dest with src
			dv.c^=sv.c;
			if(!isint[st]||!isint[dt])
				Die("Can only bitxor integers.");
			setva7(dt,dp,dx,&dv);
		brase 'M': //set dest to point to src newly allocated bytes
			if(isptr[st])
				Die("Non number malloc size.");
			*dp=malloc(sv.c*tpsz[dt]);
		brase 'N': //resize allocated memory at dest to src bytes.
			if(isptr[st])
				Die("Non number realloc size.");
			*dp=realloc(*dp,sv.c*tpsz[dt]);
		brase '>': //move dest src objects forward
			if(isptr[st])
				Die("Non number move size.");
			*dp=*dp+(sv.c*tpsz[dt]);
		brase ':': //set dest to src and move dest to next object
			setva7(dt,dp,dx,&sv);
			*dp=*dp+tpsz[dt];
		brase ';': // move to previous object and set dest to src
			*dp=*dp-tpsz[dt];
			setva7(dt,dp,dx,&sv);
		brase '<': //move dest src objects backward
			if(isptr[st])
				Die("Non number move size.");
			*dp=*dp-(sv.c*tpsz[dt]);
		brase 'F': //free memory at dest
			free(*dp);
			*dp=globa7.data;
		brase '~': //skip to # if dest != src
			if(dt<=TY_INT64)skip=(dv.c!=sv.c);
			ei(dt<=TY_PTR)skip=(dv.p!=sv.p);
			else skip=(dv.x!=sv.x);
		brase 'g': //skip to # if dest <= src
			if(dt<=TY_INT64)skip=(dv.c<=sv.c);
			ei(dt<=TY_PTR)skip=(dv.p<=sv.p);
			else skip=(dv.x<=sv.x);
		brase 'l': //skip to # if dest > src
			if(dt<=TY_INT64)skip=(dv.c>sv.c);
			ei(dt<=TY_PTR)skip=(dv.p>sv.p);
			else skip=(dv.x>=sv.x);
		brase '!': //skip to # if dest == src
			if(dt<=TY_INT64)skip=(dv.c==sv.c);
			ei(dt<=TY_PTR)skip=(dv.p==sv.p);
			else skip=(dv.x==sv.x);
		brase 'K': //save current place into dest
			if(dt==TY_PTR||dt==TY_ADDR)dv.p=g;
			else Die("Can only save label to a pointer.");
		brase 'G': //goto place saved in src
			if(st==TY_PTR||st==TY_ADDR)g=sv.p;
			else Die("Can only jump to a pointer.");
		brase 'C': //call dest as a void(*)(char**) with src as parameter.
			un(dt==TY_PTR||dt==TY_ADDR)Die("Cam only call a pointer.");
			((void(*)(char**))dv.p)(sp);
		brault: Die("Bad Operator");
		}
		if(skip){
			Log("condition false, skipping");
			wh(*g&&*g!='#')g++;
			if(*g)g++;
		}
	}
	hell:
	Log("end");
}

int scrip7rc(FILE *in){
	char *cmd=s7getcmd(in);
	if(*cmd==0){free(cmd);return 0;}
	scrip7(cmd);
	if(s7freeflag)free(cmd);
	return 1;
}


void scrip7f(FILE *f){
	wh(!feof(f))scrip7rc(f);
}

void scrip7cli(FILE *in){
	wh(!feof(in)&&scrip7rc(in));
}

#if !defined(NOS7MAIN)
__attribute__((weak)) int main(int argc,char **argv){
	globa7.data=malloc(1000);
	int i=argc,cli=1;
	wh(i>1){
		i--;
		FILE *f=fopen(argv[i],"r");
		if(!f){
			if(strchr(argv[i],'d'))s7logflag=1;
			if(strchr(argv[i],'f'))s7freeflag=1;
			if(strchr(argv[i],'c'))cli=1;
			continue;
		}
		cli=0;
		scrip7f(f);
		fclose(f);
	}
	if(cli)scrip7cli(stdin);
	return 0;
}
#endif
