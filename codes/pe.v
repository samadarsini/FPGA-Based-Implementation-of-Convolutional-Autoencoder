module processing_element_fsm (
    input wire clk,
    input wire rst,
    input wire start,

    // Image sectors
    input signed [7:0] image_sector_0,
    input signed [7:0] image_sector_1,
    input signed [7:0] image_sector_2,
    input signed [7:0] image_sector_3,
    input signed [7:0] image_sector_4,
    input signed [7:0] image_sector_5,
    input signed [7:0] image_sector_6,
    input signed [7:0] image_sector_7,
    input signed [7:0] image_sector_8,

    // Filter values
    input signed [7:0] filter_value_0,
    input signed [7:0] filter_value_1,
    input signed [7:0] filter_value_2,
    input signed [7:0] filter_value_3,
    input signed [7:0] filter_value_4,
    input signed [7:0] filter_value_5,
    input signed [7:0] filter_value_6,
    input signed [7:0] filter_value_7,
    input signed [7:0] filter_value_8,

    output reg signed [31:0] result,
    output reg done
);

    // Products
    wire signed [15:0] p0 = image_sector_0 * filter_value_0;
    wire signed [15:0] p1 = image_sector_1 * filter_value_1;
    wire signed [15:0] p2 = image_sector_2 * filter_value_2;
    wire signed [15:0] p3 = image_sector_3 * filter_value_3;
    wire signed [15:0] p4 = image_sector_4 * filter_value_4;
    wire signed [15:0] p5 = image_sector_5 * filter_value_5;
    wire signed [15:0] p6 = image_sector_6 * filter_value_6;
    wire signed [15:0] p7 = image_sector_7 * filter_value_7;
    wire signed [15:0] p8 = image_sector_8 * filter_value_8;

    // Adder Tree
    wire signed [19:0] sum1 = p0 + p1;
    wire signed [19:0] sum2 = p2 + p3;
    wire signed [19:0] sum3 = p4 + p5;
    wire signed [19:0] sum4 = p6 + p7;

    wire signed [19:0] sum5 = sum1 + sum2;
    wire signed [19:0] sum6 = sum3 + sum4;

    wire signed [31:0] final_sum = sum5 + sum6 + p8;

    // FSM
    reg [1:0] state;
    parameter IDLE = 2'd0, COMPUTE = 2'd1, DONE = 2'd2;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            state <= IDLE;
            result <= 0;
            done <= 0;
        end else begin
            case (state)
                IDLE: begin
                    done <= 0;
                    if (start)
                        state <= COMPUTE;
                end
                COMPUTE: begin
                    result <= final_sum;  
                    done <= 1;
                    state <= DONE;
                end
                DONE: begin
                    done <= 1;
                    state <= IDLE;
                end
            endcase
        end
    end

endmodule
