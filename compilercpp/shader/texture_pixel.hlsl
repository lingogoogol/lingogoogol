struct in_stu {
    float2 texcoord: Texcoord;
    uint texture_index: s_texture_index;
};

struct out_stu {
    float4 color: SV_Target;
};

Texture2D<float> tex[]: register(t0);
sampler tex_sampler: register(s0);

out_stu pixel(in_stu input) {
    out_stu output;
    output.color = tex[input.texture_index].Sample(tex_sampler, input.texcoord);
    return output;
}