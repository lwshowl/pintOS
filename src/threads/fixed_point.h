#ifndef FIXED_POINT_H

#define fp_32_t int
#define fraction_bits 14 //小数的位数

#define fraction_base (int)16384 //2^14 小数的位移

#define FP_32_MAX (int)131072
#define FP32_MIN (int)262144U
// #define fp_32_add(a, b) ((a) + (b))
// #define fp_32_sub(a, b) ((a) - (b))

//把整数转换为 17.14 定点数
fp_32_t to_fp_32(int n);
//把17.14 定点数，转换为整数，丢掉小数部分
int fp_32_round_to_int_zero(fp_32_t f);
//把17.14 顶点数转换为整数，四舍五入
int fp_32_round_to_int_near(fp_32_t f);
//定点数加法
// inline fp_32_t fp_32_add(fp_32_t f1, fp_32_t f2);
//定点数减法 f1-f2
// inline fp_32_t fp_32_subtract(fp_32_t f1, fp_32_t f2);
//定点数乘法
fp_32_t fixed_point_32_mul(fp_32_t f1, fp_32_t f2);
//定点数除法  f1/f2
fp_32_t fixed_point_32_div(fp_32_t f1, fp_32_t f2);

#endif // !FIXED_POINT_H
