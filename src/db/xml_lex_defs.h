/* Lexegen autogenerated definition file - do not edit! */
/* clang-format off */

enum {
    flag_has_more = 1,
    flag_at_beg_of_line = 2
};

enum {
    err_end_of_input = -1,
    predef_pat_default = 0,
    pat_null,
    pat_true,
    pat_false,
    pat_decimal,
    pat_neg_decimal,
    pat_real,
    pat_ws_with_nl,
    pat_other_value,
    pat_amp,
    pat_lt,
    pat_gt,
    pat_apos,
    pat_quot,
    pat_entity,
    pat_dcode,
    pat_hcode,
    pat_ent_invalid,
    pat_name,
    pat_start_element_open,
    pat_end_element_open,
    pat_end_element_close,
    pat_pi_open,
    pat_pi_close,
    pat_comment,
    total_pattern_count
};

enum {
    sc_initial = 0,
    sc_value
};
