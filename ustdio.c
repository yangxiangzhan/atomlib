/**
  ******************************************************************************
  * @file           ustdio.c
  * @author         古么宁
  * @brief          非标准化打印输出
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "ustdio.h"


/* Private types ------------------------------------------------------------*/

/* Private macro ------------------------------------------------------------*/

/* Private variables ------------------------------------------------------------*/

/* Public variables ------------------------------------------------------------*/

fmt_puts_t current_puts = NULL;
fmt_puts_t default_puts = NULL;


const char none        []= "\033[0m";  
const char black       []= "\033[0;30m";  
const char dark_gray   []= "\033[1;30m";  
const char blue        []= "\033[0;34m";  
const char light_blue  []= "\033[1;34m";  
const char green       []= "\033[0;32m";  
const char light_green []= "\033[1;32m";  
const char cyan        []= "\033[0;36m";  
const char light_cyan  []= "\033[1;36m";  
const char red         []= "\033[0;31m";  
const char light_red   []= "\033[1;31m";  
const char purple      []= "\033[0;35m";  
const char light_purple[]= "\033[1;35m";  
const char brown       []= "\033[0;33m";  
const char yellow      []= "\033[1;33m";  
const char light_gray  []= "\033[0;37m";  
const char white       []= "\033[1;37m"; 

char * default_color = (char *)none;


/* Gorgeous Split-line -----------------------------------------------*/


/**
	* @author   古么宁
	* @brief    重定义 printf 函数。本身来说 printf 方法是比较慢的，
	*           因为 printf 要做更多的格式判断，输出的格式更多一些。
	*           所以为了效率，在后面写了 printk 函数。
	* @return   NULL
*/
#ifdef __GNUC__ //for TrueStudio 

/*
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
PUTCHAR_PROTOTYPE
{
	char  cChar = (char)ch;
	if (current_puts)
		current_puts(&cChar,1);
	return ch;
}
*/
//C/C++ build->Settings->Tool Settings->C Linker->Miscellaneous->Other options 选项空中填写：-u_printf_float

int _write(int file, char *ptr, int len)
{
	if (current_puts)
		current_puts(ptr,len);
	return len;
}


#else // for keil5


#pragma import(__use_no_semihosting)
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
}

//重定义fputc函数 
int fputc(int ch, FILE *f)
{
	char  cChar = (char)ch;
	if (current_puts)
		current_puts(&cChar,1);
	return ch;
}


#endif 



#define ZEROPAD 1		/* pad with zero */
#define SIGN    2		/* unsigned/signed long */
#define PLUS    4		/* show plus */
#define SPACE   8		/* space if plus */
#define LEFT    16		/* left justified */
#define SMALL   32		/* Must be 32 == 0x20 */
#define SPECIAL	64		/* 0x */

static char *number(char *str, long num,int base, int size, int precision,
                    int type)
{
	/* we are called with base 8, 10 or 16, only, thus don't need "G..." */
	static const char digits[16] = "0123456789ABCDEF"; /* "GHIJKLMNOPQRSTUVWXYZ"; */
	unsigned long unum ;
	char tmp[66];
	char sign, locase,padding;
	int chgsize;

	if (base < 2 || base > 36)
		return NULL;

	/* locase = 0 or 0x20. ORing digits or letters with 'locase'
	 * produces same digits or (maybe lowercased) letters */
	locase = (type & SMALL);
	if (type & LEFT)
		type &= ~ZEROPAD;  

	padding = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & SIGN) {
		if (num < 0) {
			sign = '-';
			num = -num;
			size--;
		} 
		else 
		if (type & PLUS) {
			sign = '+';
			size--;
		} 
		else 
		if (type & SPACE) {
			sign = ' ';
			size--;
		}
	}

	if (type & SPECIAL) {
		if (base == 16)
			size -= 2;
		else
		if (base == 8)
			size--;
	}

	chgsize = 0;
	unum = (unsigned)num;
	do {
		tmp[chgsize] = (digits[unum % base] | locase) ;
		++chgsize;
		unum /= base;
	}while(unum);

	if (precision < chgsize)
		precision = chgsize;

	size -= precision;
	if (!(type & (ZEROPAD + LEFT))){
		for ( ; size > 0 ; --size )
			*str++ = ' ';
	}
	
	if (sign)
		*str++ = sign;
	
	if (type & SPECIAL) {
		if (base == 8)
			*str++ = '0';
		else 
		if (base == 16) {
			*str++ = '0';
			*str++ = ('X' | locase);
		}
	}

	if (!(type & LEFT)){
		for ( ; size > 0 ; --size)
			*str++ = padding ;
	}

	for ( ; chgsize < precision ; --precision ) 
		*str++ = '0';

	for ( ; chgsize > 0 ; *str++ = tmp[--chgsize]);

	for ( ; size > 0 ; --size)
		*str++ = ' ' ;

	return str;
}



static char * float2string(char *str,float num, int size, int precision,int type)
{
	char tmp[66];
	char sign,padding;
	int chgsize;
	unsigned int ipart ;

	if (type & LEFT)
		type &= ~ZEROPAD;

	padding = (type & ZEROPAD) ? '0' : ' ';

	if (precision < 0 || precision > 6) // 精度，此处精度限制为最多 6 位小数
		precision = 6;

	if (num < 0.0f) {  // 如果是负数，则先转换为正数，并占用一个字节存放负号
		sign = '-';
		num  = -num ;
		size--;
	}
	else 
		sign = 0;

	chgsize = 0;
	ipart = (unsigned int)num; // 整数部分

	if (precision) {           // 如果有小数转换，则提取小数部分
		static const float mulf[7] = {1,10,100,1000,10000,100000,1000000};
		static const unsigned int muli[7] = {1,10,100,1000,10000,100000,1000000};
		unsigned int fpart = (unsigned int)(num * mulf[precision]) - ipart * muli[precision];
		for(int i = 0 ; i < precision ; ++i) {		
			tmp[chgsize++] = (char)(fpart % 10 + '0');
			fpart /= 10;
		}
		tmp[chgsize++] = '.';
	}

	do {
		tmp[chgsize++] = (char)(ipart % 10 + '0');
		ipart /= 10;
	}while(ipart);

	size -= chgsize;                 // 剩余需要填充的大小

	if (!(type & LEFT)){             // 右对齐
		if ('0' == padding && sign) {// 如果是填充 0 且为负数，先放置负号
			*str++ = sign;
			sign = 0;
		}
		for ( ; size > 0 ; --size)   // 填充
			*str++ = padding ;
	}

	if (sign)
		*str++ = sign;

	for ( ; chgsize > 0 ; *str++ = tmp[--chgsize]);

	for ( ; size > 0 ; --size)   // 左对齐时，填充右边的空格
		*str++ = ' ' ;

	return str;
}


#include <ctype.h>

void printk(const char* fmt, ...)
//void newprintk(const char* fmt, ...)
//int vsprintf(char *buf, const char *fmt, va_list args)
{
	int len , base;
	unsigned long num;
	const char * substr;
	int flags;          /* flags to number() */
	int field_width;    /* width of output field */
	int precision;      /* min. # of digits for integers; max
                           number of chars for from string */
	int qualifier;      /* 'h', 'l', or 'L' for integer fields */

	char * fmthead = (char *)fmt;
	char * fmtout = fmthead;

	va_list args;
	va_start(args, fmt);

	for ( ; *fmtout; ++fmtout) {
		if (*fmtout == '%') {
			char tmp[128] ;
			char * str = tmp ;

			if (fmthead != fmtout)   { //先把 % 前面的部分输出
				current_puts(fmthead,fmtout - fmthead);
				fmthead = fmtout;
			}

			/* process flags */
			flags = 0;
			base  = 0;
			do {
				++fmtout; /* this also skips first '%' */
				switch (*fmtout) {
					case '-': flags |= LEFT;    break;
					case '+': flags |= PLUS;    break;
					case ' ': flags |= SPACE;   break;
					case '#': flags |= SPECIAL; break;
					case '0': flags |= ZEROPAD; break;
					default : base = 1;
				}
			}while(!base);

			/* get field width */
			for (field_width = 0 ; isdigit(*fmtout) ; ++fmtout) 
				field_width = field_width * 10 + *fmtout - '0';

			/* get the precision */
			if (*fmtout == '.') {
				precision = 0;
				for (++fmt ; isdigit(*fmtout) ; ++fmtout) 
					precision = precision * 10 + *fmtout - '0';
			}
			else
				precision = -1;

			/* get the conversion qualifier *fmt == 'h' ||  || *fmt == 'L'*/
			qualifier = -1;
			if (*fmtout == 'l') {
				qualifier = *fmtout;
				++fmtout;
			}

			/* default base */
			base = 10;

			switch (*fmtout) {
				case 'c':
					if (!(flags & LEFT))
						for ( ; --field_width > 0 ; *str++ = ' ') ;
					*str++ = (unsigned char)va_arg(args, int);
					for ( ; --field_width > 0 ; *str++ = ' ') ;
					current_puts(tmp,str-tmp);
					fmthead = fmtout + 1;
					continue;

				case 's':
					substr = va_arg(args, char *); 
					if (!substr)
						substr = "(NULL)";
					len = strnlen(substr, precision);
					if ((!(flags & LEFT)) && (len < field_width)){
						for ( ; len < field_width ; --field_width)
							*str++ = ' ';
						current_puts(tmp,str-tmp);
					} 
					current_puts(substr,len);
					if (len < field_width) {
						for ( ; len < field_width ; --field_width)
							*str++ = ' ';
						current_puts(tmp,str-tmp);
					}
					fmthead = fmtout + 1;
					continue;

				case 'p':
					if (field_width == 0) {
						field_width = 2 * sizeof(void *);
						flags |= ZEROPAD;
					}
					str = number(tmp,
						(unsigned long)va_arg(args, void *), 16,
						field_width, precision, flags);
					current_puts(tmp,str-tmp);
					fmthead = fmtout + 1;
					continue;

				case 'f':
					str = float2string(tmp,va_arg(args, double),field_width, precision, flags);
					current_puts(tmp,str-tmp);
					fmthead = fmtout + 1;
					continue;

				/* case '%':
					*str++ = '%';
					continue;*/

				/* integer number formats - set up the flags and "break" */
				case 'o':
					base = 8;
					break;

				case 'x':
					flags |= SMALL;
				case 'X':
					base = 16;
					break;

				case 'd':
				case 'i':
					flags |= SIGN;
				case 'u':
					break;

				default:
					*str++ = '%';
					if (*fmt)
						*str++ = *fmt;
					else
						--fmt;
					continue;
			}

			if (qualifier == 'l')
				num = va_arg(args, unsigned long);
			else 
				num = va_arg(args, int);

			str = number(tmp, num, base, field_width, precision, flags);
			current_puts(tmp,str-tmp);
			fmthead = fmtout + 1;
		}
	}
			
	if (fmthead != fmtout)   {
		current_puts(fmthead,fmtout - fmthead);
		fmthead = fmtout;
	}
}

