// 生成一个在 [0, 2^24) 范围中的无符号整型随机值，并初始化之前的（prev）随机数生成器
uint lcg(inout uint prev) {
    uint LCG_A = 1664525u;
    uint LCG_C = 1013904223u;
    prev = (LCG_A * prev + LCG_C);
    return prev & 0x00FFFFFF;
}

// 生成一个在 [0, 1) 范围中的单精度浮点型随机，并初始化之前的（prev）随机数生成器
float rnd(inout uint prev) {
    return (float(lcg(prev)) / float(0x01000000));
}

// 高斯分布随机数
float uniform_rnd(inout uint state, float mean, float std) {
    return mean + std * (sqrt(-2 * log(rnd(state))) * cos(2 * 3.1415926535 * rnd(state)));
}