#pragma once

#include <string_view>
#include <array>
#include <iomanip>
#include <iostream>

namespace Atom {

namespace Tunables {

struct TunableParam {
    std::string_view name;
    int* value;
    int default_value;
    int min_value;
    int max_value;
    int step;
};

struct TunableFloatParam {
    std::string_view name;
    float* value;
    float default_value;
    float min_value;
    float max_value;
    float step;
};

#ifdef ENABLE_TUNING
    #define TUNABLE(name, value) inline int name = value
#else
    #define TUNABLE(name, value) constexpr int name = value
#endif

// Define all tunable parameters
TUNABLE(ASPIRATION_WINDOW_SIZE, 5);
TUNABLE(ASPIRATION_WINDOW_DIVISOR, 13424);
TUNABLE(DELTA_INCREMENT_DIV, 3);
TUNABLE(IIR_REDUCTION, 3);
TUNABLE(RFP_DEPTH, 4);
TUNABLE(RFP_DEPTH_MULTIPLIER, 100);
TUNABLE(RAZORING_DEPTH, 2);
TUNABLE(RAZORING_DEPTH_MULTIPLIER, 400);
TUNABLE(FUTILITY_PRUNING_DEPTH, 13);
TUNABLE(FUTILITY_MULT_BASE, 122);
TUNABLE(FUTILITY_TTCUT_IMPACT, 37);
TUNABLE(FUTILITY_IMPROVEMENT_SCALE, 2);
TUNABLE(FUTILITY_WORSENING_SCALE, 3);
TUNABLE(FUTILITY_STAT_SCALE, 260);
TUNABLE(NMR_EVAL_SCALE, 202);
TUNABLE(NMR_EVAL_MAX_DIFF, 6);
TUNABLE(NMR_DEPTH_SCALE, 3);
TUNABLE(NMR_MIN_REDUCTION, 5);
TUNABLE(NMP_VERIFICATION_MIN_DEPTH, 16);
TUNABLE(NMP_VERIFICATION_MAX_STATSCORE, 14389);
TUNABLE(NMP_VERIFICATION_MIN_STAT_EVAL_BASE, 390);
TUNABLE(NMP_VERIFICATION_MIN_STAT_EVAL_DEPTH_SCALE, 21);
TUNABLE(NMP_DEPTH_SCALE, 3);
TUNABLE(NMP_DEPTH_DIVISOR, 4);
TUNABLE(MOVEPICK_CAPTURE_MULTIPLIER, 7);
TUNABLE(MOVEPICK_KILLER_SCORE, 1 << 16);
TUNABLE(MOVEPICK_CHECK_SCORE, 16384);
TUNABLE(MOVEPICK_ESCAPE_QUEEN, 51700);
TUNABLE(MOVEPICK_ESCAPE_ROOK, 25600);
TUNABLE(MOVEPICK_ESCAPE_MINOR, 14450);
TUNABLE(MOVEPICKER_ENPRISE_QUEEN, 49000);
TUNABLE(MOVEPICKER_ENPRISE_ROOK, 24335);
TUNABLE(MOVEPICKER_ENPRISE_MINOR, 14900);
TUNABLE(MOVEPICKER_LOSING_CAP_THRESHOLD, 18);
TUNABLE(MOVEPICKER_QUIET_THRESHOLD, -3560);
TUNABLE(MOVEPICKER_GOOD_QUIET_THRESHOLD, -7998);
TUNABLE(CUTNODE_MIN_DEPTH, 7);
TUNABLE(SEE_PRUNING_MAX_DEPTH, 10);
TUNABLE(SEE_PRUNING_CAP_SCORE, 180);
TUNABLE(SEE_PRUNING_CHK_SCORE, 70);
TUNABLE(REDUCTION_BASE, 1274);
TUNABLE(REDUCTION_DELTA_SCALE, 746);
TUNABLE(REDUCTION_NORMALISER, 1024);
TUNABLE(REDUCTION_SCALE_THRESHOLD, 1293);
TUNABLE(STAT_SCORE_HISTORY_REDUCTION, 4664);
TUNABLE(REDUCTION_STAT_SCORE_NORMALIZER, 10898);
TUNABLE(SCORE_IMPROVEMENT_DEPTH_MIN, 2);
TUNABLE(SCORE_IMPROVEMENT_DEPTH_MAX, 14);
TUNABLE(PREVIOUS_POS_TTPV_MIN_DEPTH, 3);
TUNABLE(SEE_PRUNING_QSEARCH_SKIP_THRESHOLD, -83);
TUNABLE(NNUE_SMALL_NET_THRESHOLD, 962);
TUNABLE(NNUE_PSQT_WEIGHT, 125);
TUNABLE(NNUE_POSITIONAL_WEIGHT, 131);
TUNABLE(NNUE_BASE_EVAL, 77777);
TUNABLE(OPTIMISM_BASE_EVAL, 7777);
TUNABLE(OPTIMISM_DAMPING, 468);
TUNABLE(OPTIMISM_RATIO_NUMERATOR, 125);
TUNABLE(OPTIMISM_RATIO_DENOMINATOR, 89);
TUNABLE(EVALUATION_NORMALIZER, 77777);
TUNABLE(RULE50_DAMPING, 212);
TUNABLE(REDUCTION_HIGH_THRESHOLD, 5);
TUNABLE(STAT_BONUS_MULTIPLIER, 190);
TUNABLE(STAT_BONUS_BASE, -108);
TUNABLE(STAT_BONUS_MAX, 1596);
TUNABLE(STAT_MALUS_DEPTH_MULTIPLIER, 736);
TUNABLE(STAT_MALUS_DEPTH_BASE, -268);
TUNABLE(STAT_MALUS_MAX, 2044);
TUNABLE(FUTULITY_PRUNING_CAPTURE_MAX_DEPTH, 7);
TUNABLE(FUTILITY_PRUNING_CAPTURE_BASE, 285);
TUNABLE(FUTILITY_PRUNING_CAPTURE_LMPDEPTH_SCALE, 251);
TUNABLE(FUTILITY_PRUNING_CAPT_HIST_SCALE, 7);
TUNABLE(FUTILITY_PRUNING_SEE_HISTORY_NORMALIZER, 32);
TUNABLE(FUTILITY_PRUNING_SEE_DEPTH_SCALE_MIN, 182);
TUNABLE(FUTILITY_PRUNING_SEE_DEPTH_SCALE_MAX, 166);
TUNABLE(FUTILITY_PRUNING_SEE_DEPTH_SCALE_THRESHOLD, -168);
TUNABLE(FUTULITY_PRUNING_CHILD_NODE_MAX_DEPTH, 13);
TUNABLE(FUTILITY_BASE_INCREMENT, 299);
TUNABLE(FUTILITY_SEE_PRUNING_MULTIPLIER, 4);
TUNABLE(NNUE_RE_EVALUATE_THRESHOLD, 227);
TUNABLE(NNUE_COMPLEXITY_SMALL, 20233);
TUNABLE(NNUE_COMPLEXITY_BIG, 17879);
TUNABLE(PAWN_VALUE_SMALLNET, 553);
TUNABLE(PAWN_VALUE_BIGNET, 532);
TUNABLE(CONT_HIST_PRUNING_SCALE, -4165);
TUNABLE(LMP_DEPTH_HISTORY_SCALE, 3853);
TUNABLE(CONT_HIST_PRUNNING_THRESHOLD, 4653);
TUNABLE(CORRECTION_HIST_VAL_NUMERATOR, 66);
TUNABLE(CORRECTION_HIST_VAL_DENOMINATOR, 512);

// Special case for floating point value
#ifdef ENABLE_TUNING
inline float REDUCTION_AMOUNT = 19.43;
inline float CONT_HIST_BONUS_MULTIPLIER = (52.0 / 64.0);
#else
constexpr float REDUCTION_AMOUNT = 19.43;
constexpr float CONT_HIST_BONUS_MULTIPLIER = (52.0 / 64.0);
#endif

#ifdef ENABLE_TUNING
// List of all integer tunable parameters
inline std::array<TunableParam, 87> TUNABLE_PARAMS = {{
    {"ASPIRATION_WINDOW_SIZE", &ASPIRATION_WINDOW_SIZE, 5, 2, 8, 1},
    {"ASPIRATION_WINDOW_DIVISOR", &ASPIRATION_WINDOW_DIVISOR, 13424, 10000, 20000, 100},
    {"DELTA_INCREMENT_DIV", &DELTA_INCREMENT_DIV, 3, 1, 10, 1},
    {"IIR_REDUCTION", &IIR_REDUCTION, 3, 1, 10, 1},
    {"RFP_DEPTH", &RFP_DEPTH, 4, 1, 10, 1},
    {"RFP_DEPTH_MULTIPLIER", &RFP_DEPTH_MULTIPLIER, 100, 1, 1000, 10},
    {"RAZORING_DEPTH", &RAZORING_DEPTH, 2, 1, 10, 1},
    {"RAZORING_DEPTH_MULTIPLIER", &RAZORING_DEPTH_MULTIPLIER, 400, 1, 1000, 10},
    {"FUTILITY_PRUNING_DEPTH", &FUTILITY_PRUNING_DEPTH, 13, 8, 18, 1},
    {"FUTILITY_MULT_BASE", &FUTILITY_MULT_BASE, 122, 1, 1000, 10},
    {"FUTILITY_TTCUT_IMPACT", &FUTILITY_TTCUT_IMPACT, 37, 1, 1000, 10},
    {"FUTILITY_IMPROVEMENT_SCALE", &FUTILITY_IMPROVEMENT_SCALE, 2, 1, 10, 1},
    {"FUTILITY_WORSENING_SCALE", &FUTILITY_WORSENING_SCALE, 3, 1, 10, 1},
    {"FUTILITY_STAT_SCALE", &FUTILITY_STAT_SCALE, 260, 1, 1000, 10},
    {"NMR_EVAL_SCALE", &NMR_EVAL_SCALE, 202, 1, 1000, 10},
    {"NMR_EVAL_MAX_DIFF", &NMR_EVAL_MAX_DIFF, 6, 1, 10, 1},
    {"NMR_DEPTH_SCALE", &NMR_DEPTH_SCALE, 3, 1, 10, 1},
    {"NMR_MIN_REDUCTION", &NMR_MIN_REDUCTION, 5, 1, 10, 1},
    {"NMP_VERIFICATION_MIN_DEPTH", &NMP_VERIFICATION_MIN_DEPTH, 16, 1, 24, 1},
    {"NMP_VERIFICATION_MAX_STATSCORE", &NMP_VERIFICATION_MAX_STATSCORE, 14389, 1, 100000, 500},
    {"NMP_VERIFICATION_MIN_STAT_EVAL_BASE", &NMP_VERIFICATION_MIN_STAT_EVAL_BASE, 390, 1, 10000, 50},
    {"NMP_VERIFICATION_MIN_STAT_EVAL_DEPTH_SCALE", &NMP_VERIFICATION_MIN_STAT_EVAL_DEPTH_SCALE, 21, 1, 30, 1},
    {"NMP_DEPTH_SCALE", &NMP_DEPTH_SCALE, 3, 1, 10, 1},
    {"NMP_DEPTH_DIVISOR", &NMP_DEPTH_DIVISOR, 4, 1, 10, 1},
    {"MOVEPICK_CAPTURE_MULTIPLIER", &MOVEPICK_CAPTURE_MULTIPLIER, 7, 1, 10, 1},
    {"MOVEPICK_CHECK_SCORE", &MOVEPICK_CHECK_SCORE, 16384, 10000, 50000, 500},
    {"MOVEPICK_ESCAPE_QUEEN", &MOVEPICK_ESCAPE_QUEEN, 51700, 40000, 60000, 500},
    {"MOVEPICK_ESCAPE_ROOK", &MOVEPICK_ESCAPE_ROOK, 25600, 20000, 30000, 250},
    {"MOVEPICK_ESCAPE_MINOR", &MOVEPICK_ESCAPE_MINOR, 14450, 10000, 20000, 250},
    {"MOVEPICKER_ENPRISE_QUEEN", &MOVEPICKER_ENPRISE_QUEEN, 49000, 1, 80000, 500},
    {"MOVEPICKER_ENPRISE_ROOK", &MOVEPICKER_ENPRISE_ROOK, 24335, 1, 50000, 500},
    {"MOVEPICKER_ENPRISE_MINOR", &MOVEPICKER_ENPRISE_MINOR, 14900, 1, 50000, 500},
    {"MOVEPICKER_LOSING_CAP_THRESHOLD", &MOVEPICKER_LOSING_CAP_THRESHOLD, 18, 1, 10, 1},
    {"MOVEPICKER_QUIET_THRESHOLD", &MOVEPICKER_QUIET_THRESHOLD, -3560, -15000, 15000, 100},
    {"MOVEPICKER_GOOD_QUIET_THRESHOLD", &MOVEPICKER_GOOD_QUIET_THRESHOLD, -7998, -15000, 15000, 100},
    {"CUTNODE_MIN_DEPTH", &CUTNODE_MIN_DEPTH, 7, 1, 10, 1},
    {"SEE_PRUNING_MAX_DEPTH", &SEE_PRUNING_MAX_DEPTH, 10, 1, 15, 1},
    {"SEE_PRUNING_CAP_SCORE", &SEE_PRUNING_CAP_SCORE, 180, 1, 1000, 10},
    {"SEE_PRUNING_CHK_SCORE", &SEE_PRUNING_CHK_SCORE, 70, 1, 1000, 10},
    {"REDUCTION_BASE", &REDUCTION_BASE, 1274, 1, 10000, 10},
    {"REDUCTION_DELTA_SCALE", &REDUCTION_DELTA_SCALE, 746, 1, 10000, 10},
    {"REDUCTION_NORMALISER", &REDUCTION_NORMALISER, 1024, 512, 2048, 128},
    {"REDUCTION_SCALE_THRESHOLD", &REDUCTION_SCALE_THRESHOLD, 1293, 1, 10000, 10},
    {"STAT_SCORE_HISTORY_REDUCTION", &STAT_SCORE_HISTORY_REDUCTION, 4664, 1, 10000, 10},
    {"REDUCTION_STAT_SCORE_NORMALIZER", &REDUCTION_STAT_SCORE_NORMALIZER, 10898, 1, 10000, 10},
    {"SCORE_IMPROVEMENT_DEPTH_MIN", &SCORE_IMPROVEMENT_DEPTH_MIN, 2, 1, 15, 1},
    {"SCORE_IMPROVEMENT_DEPTH_MAX", &SCORE_IMPROVEMENT_DEPTH_MAX, 14, 1, 15, 1},
    {"PREVIOUS_POS_TTPV_MIN_DEPTH", &PREVIOUS_POS_TTPV_MIN_DEPTH, 3, 1, 15, 1},
    {"SEE_PRUNING_QSEARCH_SKIP_THRESHOLD", &SEE_PRUNING_QSEARCH_SKIP_THRESHOLD, -83, -500, 500, 10},
    {"NNUE_SMALL_NET_THRESHOLD", &NNUE_SMALL_NET_THRESHOLD, 962, 1, 10000, 10},
    {"NNUE_PSQT_WEIGHT", &NNUE_PSQT_WEIGHT, 125, 50, 200, 5},
    {"NNUE_POSITIONAL_WEIGHT", &NNUE_POSITIONAL_WEIGHT, 131, 50, 200, 5},
    {"NNUE_BASE_EVAL", &NNUE_BASE_EVAL, 77777, 10000, 100000, 500},
    {"OPTIMISM_BASE_EVAL", &OPTIMISM_BASE_EVAL, 7777, 1000, 10000, 50},
    {"OPTIMISM_DAMPING", &OPTIMISM_DAMPING, 468, 1, 1000, 10},
    {"OPTIMISM_RATIO_NUMERATOR", &OPTIMISM_RATIO_NUMERATOR, 125, 1, 10000, 10},
    {"OPTIMISM_RATIO_DENOMINATOR", &OPTIMISM_RATIO_DENOMINATOR, 89, 1, 10000, 10},
    {"EVALUATION_NORMALIZER", &EVALUATION_NORMALIZER, 77777, 1, 100000, 10},
    {"RULE50_DAMPING", &RULE50_DAMPING, 212, 1, 10000, 10},
    {"REDUCTION_HIGH_THRESHOLD", &REDUCTION_HIGH_THRESHOLD, 5, 1, 10, 1},
    {"STAT_BONUS_MULTIPLIER", &STAT_BONUS_MULTIPLIER, 190, 1, 10000, 10},
    {"STAT_BONUS_BASE", &STAT_BONUS_BASE, -108, -1000, 1000, 10},
    {"STAT_BONUS_MAX", &STAT_BONUS_MAX, 1596, 1, 10000, 10},
    {"STAT_MALUS_DEPTH_MULTIPLIER", &STAT_MALUS_DEPTH_MULTIPLIER, 736, 1, 10000, 10},
    {"STAT_MALUS_DEPTH_BASE", &STAT_MALUS_DEPTH_BASE, -268, -1000, 1000, 10},
    {"STAT_MALUS_MAX", &STAT_MALUS_MAX, 2044, 1, 10000, 10},
    {"FUTULITY_PRUNING_CAPTURE_MAX_DEPTH", &FUTULITY_PRUNING_CAPTURE_MAX_DEPTH, 7, 1, 10, 1},
    {"FUTILITY_PRUNING_CAPTURE_BASE", &FUTILITY_PRUNING_CAPTURE_BASE, 285, 1, 10000, 10},
    {"FUTILITY_PRUNING_CAPTURE_LMPDEPTH_SCALE", &FUTILITY_PRUNING_CAPTURE_LMPDEPTH_SCALE, 251, 1, 10000, 10},
    {"FUTILITY_PRUNING_CAPT_HIST_SCALE", &FUTILITY_PRUNING_CAPT_HIST_SCALE, 7, 1, 10000, 10},
    {"FUTILITY_PRUNING_SEE_HISTORY_NORMALIZER", &FUTILITY_PRUNING_SEE_HISTORY_NORMALIZER, 32, 1, 10000, 10},
    {"FUTILITY_PRUNING_SEE_DEPTH_SCALE_MIN", &FUTILITY_PRUNING_SEE_DEPTH_SCALE_MIN, 182, 1, 10000, 10},
    {"FUTILITY_PRUNING_SEE_DEPTH_SCALE_MAX", &FUTILITY_PRUNING_SEE_DEPTH_SCALE_MAX, 166, 1, 10000, 10},
    {"FUTILITY_PRUNING_SEE_DEPTH_SCALE_THRESHOLD", &FUTILITY_PRUNING_SEE_DEPTH_SCALE_THRESHOLD, -168, -1000, 1000, 10},
    {"FUTULITY_PRUNING_CHILD_NODE_MAX_DEPTH", &FUTULITY_PRUNING_CHILD_NODE_MAX_DEPTH, 13, 8, 18, 1},
    {"FUTILITY_BASE_INCREMENT", &FUTILITY_BASE_INCREMENT, 299, 1, 10000, 10},
    {"FUTILITY_SEE_PRUNING_MULTIPLIER", &FUTILITY_SEE_PRUNING_MULTIPLIER, 4, 1, 16, 1},
    {"NNUE_RE_EVALUATE_THRESHOLD", &NNUE_RE_EVALUATE_THRESHOLD, 227, 1, 10000, 10},
    {"NNUE_COMPLEXITY_SMALL", &NNUE_COMPLEXITY_SMALL, 20233, 1, 100000, 100},
    {"NNUE_COMPLEXITY_BIG", &NNUE_COMPLEXITY_BIG, 17879, 1, 100000, 100},
    {"PAWN_VALUE_SMALLNET", &PAWN_VALUE_SMALLNET, 553, 450, 650, 2},
    {"PAWN_VALUE_BIGNET", &PAWN_VALUE_BIGNET, 532, 450, 650, 2},
    {"CONT_HIST_PRUNING_SCALE", &CONT_HIST_PRUNING_SCALE, -4165, -10000, 10000, 100},
    {"LMP_DEPTH_HISTORY_SCALE", &LMP_DEPTH_HISTORY_SCALE, 3853, 1, 10000, 10},
    {"CONT_HIST_PRUNNING_THRESHOLD", &CONT_HIST_PRUNNING_THRESHOLD, 4653, -5000, 10000, 100},
    {"CORRECTION_HIST_VAL_NUMERATOR", &CORRECTION_HIST_VAL_NUMERATOR, 66, 1, 10000, 10},
    {"CORRECTION_HIST_VAL_DENOMINATOR", &CORRECTION_HIST_VAL_DENOMINATOR, 512, 1, 10000, 10}
}};

// List of all float tunable parameters
inline std::array<TunableFloatParam, 2> TUNABLE_FLOAT_PARAMS = {{
    {"REDUCTION_AMOUNT", &REDUCTION_AMOUNT, 19.43f, 0.0f, 100.0f, 1.0f},
    {"CONT_HIST_BONUS_MULTIPLIER", &CONT_HIST_BONUS_MULTIPLIER, 0.8125f, 0.0f, 1.5f, 0.01f}
}};

inline void outputTunablesJson() {
    std::cout << "{" << std::endl;
    bool first = true;
    for (const auto& param : TUNABLE_PARAMS) {
        if (!first) {
            std::cout << "," << std::endl;
        }
        first = false;
        
        std::cout << "    \"" << param.name << "\"" << ": {" << std::endl
                  << "        \"value\"" << ": " << param.default_value << "," << std::endl
                  << "        \"min_value\"" << ": " << param.min_value << "," << std::endl
                  << "        \"max_value\"" << ": " << param.max_value << "," << std::endl
                  << "        \"step\"" << ": " << param.step << std::endl
                  << "    }";
    }

    for (const auto& param : TUNABLE_FLOAT_PARAMS) {
        std::cout << "," << std::endl
                  << "    \"" << param.name << "\"" << ": {" << std::endl
                  << "        \"value\"" << ": " << std::fixed << std::setprecision(6) << param.default_value << "," << std::endl
                  << "        \"min_value\"" << ": " << param.min_value << "," << std::endl
                  << "        \"max_value\"" << ": " << param.max_value << "," << std::endl
                  << "        \"step\"" << ": " << param.step << std::endl
                  << "    }";
    }

    std::cout << std::endl << "}" << std::endl;
}

#endif

} // namespace Tunables

} // namespace Atom
