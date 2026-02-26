`timescale 1ns/1ns

module Adder(input wire signed[15:0] in1, input wire signed[15:0] in2, input wire cin, output reg signed[15:0] out, output reg cout);
    always @(*) begin
        { cout, out } <= #5 in1 + in2 + cin;
    end
endmodule