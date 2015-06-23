#ifndef FIXED_POINT_H
#define FIXED_POINT_H


static int f= 1<<14;

int con_int_fp(int n) 
{
    return (n*f);
}

int con_fp_int_zero(int x)
{
    return (x/f);
}

int con_fp_int_near(int x)
{
    if(x>=0)
	return ((x+f/2)/f);
    if(x<=0)
	return ((x-f/2)/f);
}

int fp_fp_add(int x,int y)
{
    return (x+y);
}

int fp_fp_sub(int x,int y)
{
    return (x-y);
}

int fp_int_add(int x,int n)
{
    return (x+n*f);
}

int fp_int_sub(int x,int n)
{
    return (x-n*f);
}

int fp_fp_mul(int x,int y)
{
    return (((int64_t) x)*y/f);
}

int fp_int_mul(int x,int n)
{
    return (x*n);
}

int fp_fp_div(int x,int y)
{
    return (((int64_t) x)*f/y);
}

int fp_int_div(int x,int n)
{
    return (x/n);
}

#endif  
