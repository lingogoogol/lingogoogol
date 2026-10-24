struct vertex_in_t
{
    float3 pos;
    uint32_t texture_index;
    float2 texcoord;
};

struct vertex_out_t
{
    float4 pos : SV_Position;
    uint32_t texture_index;
    float2 texcoord;
};

[[vk::binding(0, 0)]]
uniform Sampler2D texture;

[shader("vertex")]
vertex_out_t vertex_main(vertex_in_t input)
{
    vertex_out_t output;
    output.pos = float4(input.pos, 1.0);
    output.texture_index = input.texture_index;
    output.texcoord = input.texcoord;
    return output;
}

[shader("fragment")]
float4 fragment_main(vertex_out_t input) : SV_Target
{
    float alpha = texture.Sample(input.texcoord).r;
    //Discard fully transparent fragments so the surrounding empty space of a glyph cell
    //does not write to the depth buffer and occlude adjacent overhanging glyphs.
    if (alpha == 0.0)
    {
        discard;
    }
    return float4(1.0, 1.0, 1.0, alpha);
}