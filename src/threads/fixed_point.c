#include "fixed_point.h"
#include <debug.h>
#include <stdint.h>

//把整数转换为 17.14 定点数
fp_32_t to_fp_32(int n)
{
    ASSERT(n < FP_32_MAX);
    return n * fraction_base;
}

//把17.14 定点数，转换为整数，丢掉小数部分
int fp_32_round_to_int_zero(fp_32_t f)
{
    return f / fraction_base;
}
//把17.14 定点数转换为整数，四舍五入
int fp_32_round_to_int_near(fp_32_t f)
{
    if (f >= 0)
    {
        return (int)((f + fraction_base / 2) / fraction_base);
    }
    else
    {
        return ((f - (fraction_base / 2)) / fraction_base);
    }
}
//定点数加法
// inline fp_32_t fp_32_add(fp_32_t f1, fp_32_t f2)
// {
//     // ASSERT(f1 + f2 < FP_32_MAX);
//     // ASEERT(f1 + f2 > FP32_MIN);
//     return f1 + f2;
// }
// //定点数减法 f1-f2
// inline fp_32_t fp_32_subtract(fp_32_t f1, fp_32_t f2)
// {
//     return f1 - f2;
// }
//定点数乘法
fp_32_t fixed_point_32_mul(fp_32_t f1, fp_32_t f2)
{
    return ((int64_t)f1) * f2 / fraction_base;
}
//定点数除法  f1/f2
fp_32_t fixed_point_32_div(fp_32_t f1, fp_32_t f2)
{
    return (((int64_t)f1) * fraction_base) / f2;
}
