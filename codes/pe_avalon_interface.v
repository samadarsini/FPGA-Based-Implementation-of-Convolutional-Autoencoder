module processing_element_avalon_interface (
    input csi_clockreset_clk,
    input csi_clockreset_reset_n,
    input [7:0] avs_s1_address,
    input avs_s1_read,
    input avs_s1_write,
    input [31:0] avs_s1_writedata,
    output reg [31:0] avs_s1_readdata
);

    // Image and filter inputs
    reg signed [7:0] img_0, img_1, img_2;
    reg signed [7:0] img_3, img_4, img_5;
    reg signed [7:0] img_6, img_7, img_8;

    reg signed [7:0] filt_0, filt_1, filt_2;
    reg signed [7:0] filt_3, filt_4, filt_5;
    reg signed [7:0] filt_6, filt_7, filt_8;

    reg start;
    wire signed [31:0] result;
    wire done;

    // Instantiate improved PE
    processing_element_fsm pe_inst (
        .clk(csi_clockreset_clk),
        .rst(~csi_clockreset_reset_n),
        .start(start),
        .image_sector_0(img_0), .image_sector_1(img_1), .image_sector_2(img_2),
        .image_sector_3(img_3), .image_sector_4(img_4), .image_sector_5(img_5),
        .image_sector_6(img_6), .image_sector_7(img_7), .image_sector_8(img_8),
        .filter_value_0(filt_0), .filter_value_1(filt_1), .filter_value_2(filt_2),
        .filter_value_3(filt_3), .filter_value_4(filt_4), .filter_value_5(filt_5),
        .filter_value_6(filt_6), .filter_value_7(filt_7), .filter_value_8(filt_8),
        .result(result),
        .done(done)
    );

    // Register write logic
    always @(posedge csi_clockreset_clk) begin
        if (!csi_clockreset_reset_n) begin
            img_0 <= 0; img_1 <= 0; img_2 <= 0;
            img_3 <= 0; img_4 <= 0; img_5 <= 0;
            img_6 <= 0; img_7 <= 0; img_8 <= 0;

            filt_0 <= 0; filt_1 <= 0; filt_2 <= 0;
            filt_3 <= 0; filt_4 <= 0; filt_5 <= 0;
            filt_6 <= 0; filt_7 <= 0; filt_8 <= 0;

            start <= 0;
        end else if (avs_s1_write) begin
            case (avs_s1_address)
                8'd0:  img_0 <= avs_s1_writedata;
                8'd1:  img_1 <= avs_s1_writedata;
                8'd2:  img_2 <= avs_s1_writedata;
                8'd3:  img_3 <= avs_s1_writedata;
                8'd4:  img_4 <= avs_s1_writedata;
                8'd5:  img_5 <= avs_s1_writedata;
                8'd6:  img_6 <= avs_s1_writedata;
                8'd7:  img_7 <= avs_s1_writedata;
                8'd8:  img_8 <= avs_s1_writedata;
                8'd9:  filt_0 <= avs_s1_writedata;
                8'd10: filt_1 <= avs_s1_writedata;
                8'd11: filt_2 <= avs_s1_writedata;
                8'd12: filt_3 <= avs_s1_writedata;
                8'd13: filt_4 <= avs_s1_writedata;
                8'd14: filt_5 <= avs_s1_writedata;
                8'd15: filt_6 <= avs_s1_writedata;
                8'd16: filt_7 <= avs_s1_writedata;
                8'd17: filt_8 <= avs_s1_writedata;
                8'd18: start  <= avs_s1_writedata[0];
            endcase
        end
    end

    // Read logic
    always @(*) begin
        case (avs_s1_address)
            8'd30: avs_s1_readdata = result;
            8'd31: avs_s1_readdata = done;
            default: avs_s1_readdata = 32'd0;
        endcase
    end

endmodule
