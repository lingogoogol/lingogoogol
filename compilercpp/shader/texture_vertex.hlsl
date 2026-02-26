struct in_stu {
    float4 pos: POS;
    float4 texcoord: Texcoord;
    uint texture_index: s_texture_index;
};

struct out_stu {
    float4 texcoord: Texcoord;
    uint texture_index: s_texture_index;
    float4 pos: SV_Position;
};

out_stu vertex(in_stu input) {
    out_stu output;
    output.pos = input.pos;
    output.texture_index = input.texture_index;
    output.texcoord = input.texcoord;
    return output;
}