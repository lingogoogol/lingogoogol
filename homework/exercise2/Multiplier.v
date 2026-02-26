`include "Adder.v"

module Multiplier(input wire clk, input wire signed[7:0] in1, input wire signed[7:0] in2, output reg signed[15:0] out);
	reg signed[15:0] intermediate[0:8];
	wire signed[15:0] added[0:7];
	reg signed[15:0] in1_copy[0:7];
	reg signed[15:0] in2_copy[0:7];
	integer i;

	genvar gen_i;
	generate
		for (gen_i = 0; gen_i < 8; gen_i = gen_i + 1) begin
			Adder adder(.in1(intermediate[gen_i]), .in2(in1_copy[gen_i] << gen_i), .cin(1'b0), .out(added[gen_i]), .cout());
		end
	endgenerate

	always @(*) begin
		out = intermediate[8];
	end

	always @(posedge clk) begin
		intermediate[0] <= 16'h0000;
		for (i = 0; i < 8; i = i + 1) begin
			if (in2_copy[i][i]) begin
				intermediate[i + 1] <= added[i];
			end else begin
				intermediate[i + 1] <= intermediate[i];
			end
		end
		in1_copy[0] <= in1;
		in2_copy[0] <= in2;
		for (i = 1; i < 8; i = i + 1) begin
			in1_copy[i] <= in1_copy[i - 1];
			in2_copy[i] <= in2_copy[i - 1];
		end
	end
endmodule